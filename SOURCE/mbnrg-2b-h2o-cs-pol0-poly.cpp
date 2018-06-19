#include <cmath>
#include <algorithm>

#include "poly-2b-h2o-ion-v1x.h"

////////////////////////////////////////////////////////////////////////////////

namespace {

//----------------------------------------------------------------------------//

struct variable {
    double v_exp(const double& r0, const double& k,
                 const double* xcrd, int o, int x );

    double v_coul(const double& r0, const double& k,
                  const double* xcrd, int o, int x );

    void grads(const double& gg, double* xgrd, int o, int x ) const;

    double g[3]; // diff(value, p1 - p2)
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - //

double variable::v_exp(const double& r0, const double& k,
                       const double* xcrd, int o, int x )
{
    g[0] = xcrd[o++] - xcrd[x++];
    g[1] = xcrd[o++] - xcrd[x++];
    g[2] = xcrd[o]   - xcrd[x];

    const double r = std::sqrt(g[0]*g[0] + g[1]*g[1] + g[2]*g[2]);

    const double exp1 = std::exp(k*(r0 - r));
    const double gg = - k*exp1/r;

    g[0] *= gg;
    g[1] *= gg;
    g[2] *= gg;

    return exp1;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - //

double variable::v_coul(const double& r0, const double& k,
                        const double* xcrd, int o, int x)
{
    g[0] = xcrd[o++] - xcrd[x++];
    g[1] = xcrd[o++] - xcrd[x++];
    g[2] = xcrd[o]   - xcrd[x];

    const double rsq = g[0]*g[0] + g[1]*g[1] + g[2]*g[2];
    const double r = std::sqrt(rsq);

    const double exp1 = std::exp(k*(r0 - r));
    const double rinv = 1.0/r;
    const double val = exp1*rinv;

    const double gg = - (k + rinv)*val*rinv;

    g[0] *= gg;
    g[1] *= gg;
    g[2] *= gg;

    return val;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - //

void variable::grads(const double& gg, double* xgrd, int o, int x) const
{
    for (int i = 0; i < 3; ++i) {
        const double d = gg*g[i];

        xgrd[o++] += d;
        xgrd[x++] -= d;
    }
}

//----------------------------------------------------------------------------//

struct monomer {
    double oh1[3];
    double oh2[3];

    void setup(const double* ohh,
               const double& in_plane_g, const double& out_of_plane_g,
               double x1[3], double x2[3]);

    void grads(const double* g1, const double* g2,
               const double& in_plane_g, const double& out_of_plane_g,
               double* grd) const;
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - //

void monomer::setup(const double* ohh,
                    const double& in_plane_g, const double& out_of_plane_g,
                    double* x1, double* x2)
{
    for (int i = 0; i < 3; ++i) {
        oh1[i] = ohh[i + 3] - ohh[i];
        oh2[i] = ohh[i + 6] - ohh[i];
    }

    const double v[3] = {
        oh1[1]*oh2[2] - oh1[2]*oh2[1],
        oh1[2]*oh2[0] - oh1[0]*oh2[2],
        oh1[0]*oh2[1] - oh1[1]*oh2[0]
    };

    for (int i = 0; i < 3; ++i) {
        const double in_plane = ohh[i] + 0.5*in_plane_g*(oh1[i] + oh2[i]);
        const double out_of_plane = out_of_plane_g*v[i];

        x1[i] = in_plane + out_of_plane;
        x2[i] = in_plane - out_of_plane;
    }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - //

void monomer::grads(const double* g1, const double* g2,
                    const double& in_plane_g, const double& out_of_plane_g,
                    double* grd) const
{
    const double gm[3] = {
        g1[0] - g2[0],
        g1[1] - g2[1],
        g1[2] - g2[2]
    };

    const double t1[3] = {
        oh2[1]*gm[2] - oh2[2]*gm[1],
        oh2[2]*gm[0] - oh2[0]*gm[2],
        oh2[0]*gm[1] - oh2[1]*gm[0]
    };

    const double t2[3] = {
        oh1[1]*gm[2] - oh1[2]*gm[1],
        oh1[2]*gm[0] - oh1[0]*gm[2],
        oh1[0]*gm[1] - oh1[1]*gm[0]
    };

    for (int i = 0; i < 3; ++i) {
        const double gsum = g1[i] + g2[i];
        const double in_plane = 0.5*in_plane_g*gsum;

        const double gh1 = in_plane + out_of_plane_g*t1[i];
        const double gh2 = in_plane - out_of_plane_g*t2[i];

        grd[i + 0] += gsum - (gh1 + gh2); // O
        grd[i + 3] += gh1; // H1
        grd[i + 6] += gh2; // H2
    }
}

////////////////////////////////////////////////////////////////////////////////

struct x2b_h2o_ion_v1x_p {
    x2b_h2o_ion_v1x_p();

    typedef h2o_ion::poly_2b_h2o_ion_v1x poly_type;

    double operator()(const double* w, const double* x,
                      double* g1, double* g2) const;

protected:
    double m_k_HH_intra;
    double m_k_OH_intra;

    double m_k_XH_coul;
    double m_k_XO_coul;

    double m_k_XLp_main;

    double m_d_HH_intra;
    double m_d_OH_intra;

    double m_d_XH_coul;
    double m_d_XO_coul;

    double m_d_XLp_main;

    double m_in_plane_gamma;
    double m_out_of_plane_gamma;

public:
    double m_r2i;
    double m_r2f;

    double f_switch(const double&, double&) const; // X-O separation

protected:
    double m_poly[poly_type::size];
};

////////////////////////////////////////////////////////////////////////////////

double x2b_h2o_ion_v1x_p::f_switch(const double& r, double& g) const
{
    if (r > m_r2f) {
        g = 0.0;
        return 0.0;
    } else if (r > m_r2i) {
        const double t1 = M_PI/(m_r2f - m_r2i);
        const double x = (r - m_r2i)*t1;
        g = - std::sin(x)*t1/2.0;
        return (1.0 + std::cos(x))/2.0;
    } else {
        g = 0.0;
        return 1.0;
    }
}

//----------------------------------------------------------------------------//

double x2b_h2o_ion_v1x_p::operator()
    (const double* w, const double* x, double* g1, double* g2) const
{
    // the switch

    const double dXO[3] = {w[0] - x[0],
                           w[1] - x[1],
                           w[2] - x[2]};

    const double rXOsq = dXO[0]*dXO[0] + dXO[1]*dXO[1] + dXO[2]*dXO[2];
    const double rXO = std::sqrt(rXOsq);

    if (rXO > m_r2f)
        return 0.0;

    // offsets

    const int O  = 0;
    const int H1 = 3;
    const int H2 = 6;

    const int X   = 9;
    
    const int Lp1 = 12;
    const int Lp2 = 15;

    double xcrd[18]; // coordinates including extra-points

    std::copy(w, w + 9, xcrd);
    std::copy(x , x  + 9, xcrd + 9);

    // the extra-points

    monomer ma;

    ma.setup(xcrd + O,
             m_in_plane_gamma, m_out_of_plane_gamma,
             xcrd + Lp1, xcrd + Lp2);

    // variables

//    const double d0_intra = 1.0;  //TODO MBpol values 
//    const double d0_inter = 4.0;

    double v[ 8]; // stored separately (gets passed to poly::eval)

    variable ctxt[8 ];

    v[0] = ctxt[0].v_exp(m_d_HH_intra, m_k_HH_intra, xcrd, H1, H2);

    v[1] = ctxt[1].v_exp(m_d_OH_intra, m_k_OH_intra, xcrd, O, H1);
    v[2] = ctxt[2].v_exp(m_d_OH_intra, m_k_OH_intra, xcrd, O, H2);

    v[3] = ctxt[3].v_coul(m_d_XH_coul, m_k_XH_coul, xcrd, X, H1);
    v[4] = ctxt[4].v_coul(m_d_XH_coul, m_k_XH_coul, xcrd, X, H2);
                            
    v[5] = ctxt[5].v_coul(m_d_XO_coul, m_k_XO_coul, xcrd, X, O);

    v[6] = ctxt[6].v_exp(m_d_XLp_main, m_k_XLp_main, xcrd, X, Lp1);
    v[7] = ctxt[7].v_exp(m_d_XLp_main, m_k_XLp_main, xcrd, X, Lp2);

    double g[8 ];
    const double E_poly = h2o_ion::poly_2b_h2o_ion_v1x::eval(m_poly, v, g);

    double xgrd[18];
    std::fill(xgrd, xgrd + 18, 0.0);

   ctxt[0].grads(g[0], xgrd, H1, H2);
                       
    ctxt[1].grads(g[1], xgrd, O, H1);
    ctxt[2].grads(g[2], xgrd, O, H2);
                       
    ctxt[3].grads(g[3], xgrd, X, H1);
    ctxt[4].grads(g[4], xgrd, X, H2);

    ctxt[5].grads(g[5], xgrd, X, O);

    ctxt[6].grads(g[6], xgrd, X, Lp1);
    ctxt[7].grads(g[7], xgrd, X, Lp2);

    // distribute gradients w.r.t. the X-points

    ma.grads(xgrd + Lp1, xgrd + Lp2,
             m_in_plane_gamma, m_out_of_plane_gamma,
             xgrd + O);

    // the switch

    double gsw;
    const double sw = f_switch(rXO, gsw);

    for (int i = 0; i < 9; ++i) {
        g1[i] = sw*xgrd[i];
    }

    for (int i = 0; i < 3; ++i) {
        g2[i] += sw*xgrd[i + 9];
    }

    // gradient of the switch

    gsw *= E_poly/rXO;
    for (int i = 0; i < 3; ++i) {
        const double d = gsw*dXO[i];
        g1[i] += d;
        g2[i] -= d;
    }

    return sw*E_poly;
}

//----------------------------------------------------------------------------//

//fit-fullpolargrid-fixedwaterparams  12/20/16
static const double the_poly[] = {
-1.105614770018504e+02, // 0
-9.825030328753317e+01, // 1
 2.469761906977259e+01, // 2
 2.788317993155662e+02, // 3
-6.062664083643927e+00, // 4
-3.741665956957034e+02, // 5
-4.740936688380801e+02, // 6
-8.877142771836617e+01, // 7
 1.180234852991351e+02, // 8
 3.682076035469317e+02, // 9
 3.108023044224163e+02, // 10
 1.287659355270088e+01, // 11
 1.592439915706151e+02, // 12
-2.775432788771330e+02, // 13
 2.064799167184454e+02, // 14
-3.331547423558126e+02, // 15
 4.743755550859896e+02, // 16
-1.864004393107394e+02, // 17
 3.716829939023386e+01, // 18
-3.348361135011235e+01, // 19
-9.831744507349990e-01, // 20
-1.156267953848431e+02, // 21
 2.284601361404158e+01, // 22
-4.891595642211557e+01, // 23
-2.989363384962454e+01, // 24
 4.482940253627178e+02, // 25
-2.285085233875727e+02, // 26
 4.776061422971304e+01, // 27
 3.834149540960037e+02, // 28
 2.222894882566486e+02, // 29
 6.671354177580045e+01, // 30
-1.700302146214687e+02, // 31
 6.207552951749027e+01, // 32
-1.025197106746872e+02, // 33
-1.410160374462876e+02, // 34
-3.660835808472705e+02, // 35
-3.981302735601237e+01, // 36
-5.270893224772454e+02, // 37
-1.233211720143845e+02, // 38
 5.412683207532332e+02, // 39
 2.034818465064892e+02, // 40
-6.826083382797242e+00, // 41
 5.895031322817691e+01, // 42
-9.583486958377181e-01, // 43
-2.445242022297921e+02, // 44
 1.580020204296660e+01, // 45
-3.646607183233935e+02, // 46
 1.447151540875708e+02, // 47
 7.137695447957800e+01, // 48
 9.594785702077712e+01, // 49
-1.777849775629444e+02, // 50
 1.238721276075003e+02, // 51
-1.040047241176219e+02, // 52
 1.823861643866948e+02, // 53
-4.459530138568504e+02, // 54
-5.243305510933263e+02, // 55
-1.104748527918317e+02, // 56
-9.073615878777875e+01, // 57
-1.051256457308386e+02, // 58
 4.091935756403835e+02, // 59
-1.002076824685990e+02, // 60
 2.088038168184983e+01, // 61
-7.231478228551527e+00, // 62
 6.297382545811266e+00, // 63
 1.852906221571916e+02, // 64
 7.451754219090476e+01, // 65
 4.840101021692220e+02, // 66
-3.754231533931470e+01, // 67
-2.125771807700588e+02, // 68
 2.353905317691336e+00, // 69
 1.370412862407883e+01, // 70
-5.139401677985342e+00, // 71
 6.523241767805041e+01, // 72
 2.942002472678039e+02, // 73
 7.086819849184143e+01, // 74
 3.433820934265406e+01, // 75
-6.216825967116808e+00, // 76
 2.033804337158872e+01, // 77
-8.456966011825597e+01, // 78
 1.569611998264279e+01, // 79
-9.508083201325652e+00, // 80
-6.507536208863169e+01, // 81
 7.145358398408574e+00, // 82
-4.557266584352331e-01, // 83
-2.117096207864242e+00, // 84
-3.841691901567331e-01, // 85
-5.378386028960161e+00, // 86
-5.244414063557453e+01, // 87
-4.495330051017976e+02, // 88
 2.396633829063228e+01, // 89
-2.730288629634323e+00, // 90
-2.072089447659238e+02, // 91
 2.108351475739068e+01, // 92
-9.469754213728872e+01, // 93
 4.051287698068053e+01, // 94
 6.832562458541442e+01, // 95
 7.401540348939028e+01, // 96
 2.666883179221836e+02, // 97
-2.651569493400016e+02, // 98
-3.131334844459640e+02, // 99
 5.825916085612047e+01, // 100
 1.407580697233732e+00, // 101
-9.101323653614169e+01, // 102
 1.102728253643473e+02, // 103
-8.726723661296101e+01, // 104
-3.730434776391323e+01, // 105
-1.601830935550024e+01, // 106
 2.328052055412077e+02, // 107
-1.904190150381032e+01, // 108
-6.521051547201651e+01, // 109
 2.084811574221049e+01, // 110
-5.064662956826123e+01, // 111
-3.859445540885740e+01, // 112
-1.639726898913287e+00, // 113
 9.674353695855485e+00, // 114
-2.788910605667716e+01, // 115
 1.621553547673256e+02, // 116
 6.003936278621079e+00, // 117
-2.730388717602010e+01, // 118
 1.069497841468938e+02, // 119
-1.671660653792345e+02, // 120
-1.354559658489634e+02, // 121
 4.957559096213681e+01, // 122
 1.512809401300308e+02, // 123
-1.860144539355574e+02, // 124
 8.161114066538198e+01, // 125
-2.833614536372128e+01, // 126
-3.134958922960918e+01, // 127
 1.225644510971418e+02, // 128
-4.452114310460258e+00, // 129
 7.311496335262363e+01, // 130
-1.130530952134716e+02, // 131
-1.111651433294307e+02, // 132
-4.013283697811922e+01, // 133
-2.263969721130914e+02, // 134
 1.154180813904560e+00, // 135
-1.222614372217950e+01, // 136
 1.076751186026539e+01, // 137
 1.018314284507953e+02, // 138
-2.512161847959364e+01, // 139
 4.167599969898210e+00, // 140
-5.434040843325138e+00, // 141
 4.114834347267835e+00, // 142
 2.446038214482556e+02, // 143
 2.909026819598851e+02, // 144
-9.899290781861922e+01, // 145
-3.108354337495328e+01, // 146
-4.457964018136954e+01, // 147
-4.070578453460354e+02, // 148
-5.435823345632069e+01, // 149
 4.955430343179063e+02, // 150
 2.585043478855473e+01, // 151
 1.137549462179738e+02, // 152
 7.824978109927986e+00, // 153
-1.119566133543612e+01, // 154
 6.331892348627441e+01, // 155
 1.369566609250454e+02, // 156
 1.552271295217863e+01, // 157
-1.858097232215445e+00, // 158
-5.787204027192023e-01, // 159
 2.158761600685430e+02, // 160
 3.998159759513873e-01, // 161
 4.249854737025240e+01, // 162
-8.620024659771502e+01, // 163
 1.918261803769699e+02, // 164
 2.536915433259739e+01, // 165
 2.248226390793862e+02, // 166
-1.572041510331164e+02, // 167
 6.644897424121524e+01, // 168
-1.110343716605393e+02, // 169
 1.480936314786786e+02, // 170
-1.153938076706837e+00, // 171
-3.796762897176976e+01, // 172
 2.914429390865374e+01, // 173
-2.354313902702882e+02, // 174
-1.378337688446476e+02, // 175
-9.684136931518290e+01, // 176
-1.149462978592395e+01, // 177
-3.076753036999822e+01, // 178
-1.874250937563757e+01, // 179
-1.475847026927634e+01, // 180
 2.014076146401490e+02, // 181
 4.848957435914275e+01, // 182
 2.544869889456467e+01, // 183
 2.614721346697518e+02, // 184
-4.070326642027972e-01, // 185
 1.364686949725344e+00, // 186
-8.556746853449104e+00, // 187
 6.128344886524192e-02, // 188
-1.218116279063094e+00, // 189
 1.434920310956369e+02, // 190
-1.073359725944321e+02, // 191
-9.111729290962156e+00, // 192
-1.819495060473891e+01, // 193
-2.907308072824336e+01, // 194
 1.942947189399996e-01, // 195
 9.660103549314716e-01, // 196
-7.516056366880602e-01, // 197
-9.722814902002726e+01, // 198
-8.936860643138607e+00, // 199
-6.086822447289836e+00, // 200
 2.637399797573739e+00, // 201
-1.181859039629708e+00, // 202
 1.170168193509044e+02, // 203
 3.415846718420897e+01, // 204
 1.608852846294307e+01, // 205
 5.686109222246864e+00, // 206
-6.854382886872917e-01, // 207
 2.236536974447503e+02, // 208
-8.749637288363395e+01, // 209
-9.345530201001659e-03, // 210
 1.193394952158277e-01, // 211
-6.314942278051090e-01, // 212
-7.068477347383371e-02, // 213
 8.705770563053491e+01, // 214
 1.039483307466623e+01, // 215
-1.478806882457309e+01, // 216
 5.302637461982467e+01, // 217
 9.377677680553811e+00, // 218
-3.146619415867709e+00, // 219
 1.642955509318426e+00, // 220
-1.855057641271880e+00, // 221
 1.081458319080681e+01, // 222
-4.649946125268850e+01, // 223
 2.336579118764239e-01, // 224
 1.752528792249326e+01, // 225
-6.492043525483640e-02, // 226
-8.209273140539071e-04, // 227
 1.109375364210087e+01, // 228
 5.092414848008199e-01, // 229
 2.301567893100067e+01, // 230
-1.628945317020414e+01, // 231
-8.907295919783651e+00, // 232
 4.697422003791315e-01, // 233
-8.423409395706429e+00, // 234
 1.681083401379382e+00, // 235
 4.074224865842680e+01, // 236
-1.565733400470429e+01, // 237
-4.207172874699131e-02, // 238
-3.542602337503396e+02, // 239
-4.195702598147291e+01, // 240
 4.457398994346142e+00, // 241
 2.174566448997237e+00, // 242
 1.608996056900799e+01, // 243
-1.705791694619859e-02, // 244
-4.052321824113289e-02, // 245
 2.491504102402621e+01, // 246
 8.787663197162603e-01, // 247
-1.304101930991467e+00, // 248
-8.627986877293894e-01, // 249
-2.479263400799576e+00, // 250
-3.525376519201321e+01, // 251
 2.362953756574752e+00, // 252
-2.752780942294413e+01, // 253
 1.979522004637356e+01, // 254
-5.362256164131713e+00, // 255
-5.452839073300055e+00, // 256
 2.368662816219302e+01, // 257
-7.472554481279775e-02, // 258
-1.503233622408940e+02, // 259
 7.766977770411194e+01, // 260
 1.753017446221560e+01, // 261
 2.315030233572208e+00, // 262
 1.052094783874922e+01, // 263
-4.646933500343461e+01, // 264
 2.295923887521127e-01, // 265
 5.529535183375861e-02, // 266
-2.018668518606739e+00, // 267
-2.167513945253709e+01, // 268
-3.944717495315816e-03, // 269
 1.349091914864801e+01, // 270
-8.268036888713294e-01, // 271
 6.510702187168299e+01, // 272
 1.945045367904351e+01, // 273
 1.773430498597573e+00, // 274
 3.113081592982548e-01, // 275
-1.766911482155688e+01, // 276
-6.002917430218244e-05, // 277
-1.563631926932225e+01, // 278
 4.134949960426065e+00, // 279
-2.329062064031548e-01, // 280
 6.381524957113400e+01, // 281
-2.549667403845790e+01, // 282
 4.403238114076532e+01, // 283
-1.936227270962694e+00, // 284
-9.705217709968339e-02, // 285
 9.171975461539960e+00, // 286
-1.464332016189016e-01, // 287
 9.997840935634372e+01, // 288
-6.497026209563885e-01, // 289
-1.390785011405785e+00, // 290
 1.149782047445961e-01, // 291
-9.521463054003470e+01, // 292
-3.831565915376657e+00, // 293
-6.045944049407567e+01, // 294
 9.003289771673762e+00, // 295
-1.274777332416889e+01, // 296
 2.635073759743661e+01, // 297
-1.372995063503555e-01, // 298
-1.649473185859130e+00, // 299
-6.141154080574189e+00, // 300
-7.346064810363689e+00, // 301
-3.615274244950342e-03, // 302
 1.415876141030011e+01, // 303
 4.954110876938050e-01, // 304
 2.536650610906116e+01, // 305
-6.102641036142714e+01, // 306
 1.506266027495302e-02, // 307
 6.031585519663399e+01, // 308
-2.693138435423308e-04, // 309
 2.627669617344733e+02, // 310
-3.008387037670570e+00, // 311
-1.254758308340180e+02, // 312
 7.649523230011636e+01, // 313
 7.849585263888429e-01, // 314
 7.675870968047101e+00, // 315
 6.035866755397181e-01, // 316
 1.592586654042987e+01, // 317
 4.308446728656245e-02, // 318
 2.591959404905246e+00, // 319
-6.925650020610672e+01, // 320
 6.990381937656812e+01, // 321
-1.011059939123210e+00, // 322
-7.974397778206370e-01, // 323
 2.159596544762071e+00, // 324
 1.567347338717625e+01, // 325
-4.516382991868071e+00, // 326
 1.300938986499199e+01, // 327
-1.222431367637576e+02, // 328
-1.221675713091563e+00, // 329
 1.392006143991481e-02, // 330
-1.775742293001351e+02, // 331
-2.048774199775675e+01, // 332
-1.636388888975205e+01, // 333
-4.276406658027285e-02, // 334
 9.960820941689584e-03, // 335
-7.607958982500798e-01, // 336
 1.354041537061910e-01, // 337
 1.969483002377140e+01, // 338
-2.230574086607968e+01, // 339
 3.815481758710325e+00, // 340
-2.023377428851041e-02, // 341
 1.142568530388813e+01, // 342
-5.976653891221181e+01, // 343
-9.610812722416156e+00, // 344
-1.492199978183326e+02, // 345
-2.778803360480031e+00, // 346
-2.504918776582176e+00, // 347
 5.956432338679324e+02, // 348
-3.441131551008656e+00, // 349
 2.769001255655415e+01, // 350
 4.304163109934117e+00, // 351
 3.556592057262577e+00, // 352
 1.695598234716563e+00, // 353
 3.439125506010050e-02, // 354
-1.882633198897570e+02, // 355
 2.821783584935865e+01, // 356
-7.272227183538491e-02, // 357
-8.076798186403286e+01, // 358
-4.765125713481653e+00, // 359
-5.857660600349405e+00, // 360
 4.731487817265911e+00, // 361
-2.457682310445076e+00, // 362
-4.752244684459252e+01, // 363
-4.476640476228796e+00, // 364
 5.079237742641035e+01, // 365
 1.657217747874428e-01, // 366
-2.941198268921083e+00, // 367
 5.553523367279293e+00, // 368
-4.914791534680772e+00, // 369
-2.242575529520054e-02, // 370
 1.029344735841188e+02, // 371
 1.658516098726170e+00, // 372
-4.085099333542323e-01, // 373
 4.623230516744044e+01, // 374
-4.735085857657388e+00, // 375
 1.673061160501111e+01, // 376
 4.055537387522292e-01, // 377
 1.293138244273198e+00, // 378
-3.028924778398887e+01, // 379
 9.550688719021658e-01, // 380
 5.178673135077571e+00, // 381
 3.025999569588632e-01, // 382
-2.597120950891387e-01, // 383
-4.500132188597743e+01, // 384
-1.459182537070393e-01, // 385
 2.094893030538693e-02, // 386
-7.442731447905632e+01, // 387
 6.398936074733645e+00, // 388
 2.063688783108456e+01, // 389
 8.493451755999496e+01, // 390
-1.121723101441336e+01, // 391
-2.090444982988485e+01, // 392
-4.932291626928944e+00, // 393
 1.145113261659942e-01, // 394
 5.889951288548299e-01, // 395
 3.057841690682318e+01, // 396
-1.492278701223528e+00, // 397
-1.291218281531360e+00, // 398
-2.118658907127782e+01, // 399
 5.645319926282220e+01, // 400
-7.913769274187072e+00, // 401
-2.895871239184272e-02, // 402
 3.999689279508760e+00, // 403
 3.570795085352875e+00, // 404
 6.040483537232118e+00, // 405
 1.961145192444221e+01, // 406
-1.681418437881552e+00, // 407
-1.457061215906534e+01, // 408
 1.292421784360771e+00, // 409
 5.055793012285910e+00, // 410
 1.021653225310645e+00, // 411
-1.238844416982697e+00, // 412
 9.136542983073189e+00, // 413
 3.224201202772262e+01, // 414
 1.337064526736697e+01, // 415
 1.376458483424400e+00, // 416
-8.836088713761910e+00, // 417
 1.292481179950134e+02, // 418
-1.130620105268816e+00, // 419
 2.780500648948220e+00, // 420
 1.186712246202522e+00, // 421
-2.407643988967958e-02, // 422
 1.154539703064144e+00, // 423
 3.173975945739663e-01, // 424
 1.722326704167514e-03, // 425
 7.823922992400654e+00, // 426
-2.997093696628805e+01, // 427
 6.078832499225732e-02 // 428
};

//----------------------------------------------------------------------------//

x2b_h2o_ion_v1x_p::x2b_h2o_ion_v1x_p()
{
    m_k_HH_intra =         4.028053520238458e-01; // A^(-1)
    m_k_OH_intra =         4.509806860358335e-01; // A^(-1)
                           
    m_k_XH_coul =          4.864870134382200e-01; // A^(-1)
    m_k_XO_coul =          6.288413402784591e-01; // A^(-1)
                           
    m_k_XLp_main =         8.069975251987423e-01; // A^(-1)
                           
    m_d_HH_intra =         1.999837886240069e+00; // A^(-1)
    m_d_OH_intra =         1.162051362716558e+00; // A^(-1)
                           
    m_d_XH_coul =          6.309550239856037e+00; // A^(-1)
    m_d_XO_coul =          6.999999948206465e+00; // A^(-1)
                           
    m_d_XLp_main =         3.971458511009841e+00; // A^(-1)
                           
    m_in_plane_gamma =     -9.721486914088159e-02;
    m_out_of_plane_gamma=  9.859272078406150e-02;

    m_r2i =  6.000000000000000e+00; // A
    m_r2f =  7.000000000000000e+00; // A

    std::copy(the_poly, the_poly + poly_type::size, m_poly);
};

//----------------------------------------------------------------------------//

static const x2b_h2o_ion_v1x_p the_model;

//----------------------------------------------------------------------------//

} // namespace

////////////////////////////////////////////////////////////////////////////////

extern "C" {

//----------------------------------------------------------------------------//

#ifdef BGQ
void mbnrg_2b_h2o_cs_poly(const double* w, const double* x ,
                    double* E, double* g1, double* g2)
#else
void mbnrg_2b_h2o_cs_poly_(const double* w, const double* x ,
                    double* E, double* g1, double* g2)
#endif
{
    *E = the_model(w , x , g1, g2);
}

//----------------------------------------------------------------------------//

#ifdef BGQ
void mbnrg_2b_h2o_cs_cutoff(double* r)
#else
void mbnrg_2b_h2o_cs_cutoff_(double* r)
#endif
{
    *r = the_model.m_r2f;
}

//----------------------------------------------------------------------------//

} // extern "C"

////////////////////////////////////////////////////////////////////////////////
