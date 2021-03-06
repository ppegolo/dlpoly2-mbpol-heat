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

//fit-fullpolargrid-fixedwaterparams 100%polfac 03/15/17
static const double the_poly[] = {
 3.328235414306171e+01, // 0
-2.445161690197339e+02, // 1
-1.723616297153360e+01, // 2
 3.088195964174597e+01, // 3
 1.292297336573403e+01, // 4
-2.244372357533834e+01, // 5
 6.533810451377436e+02, // 6
-2.618769177131760e+02, // 7
 2.396830679052968e+02, // 8
 1.384296685689339e+02, // 9
 1.007346331812710e+01, // 10
 5.012433377019573e+00, // 11
 5.167965656474346e+01, // 12
-1.939327728104440e+02, // 13
-3.180787390117308e+02, // 14
-1.861374147480916e+02, // 15
-3.542916834101724e+02, // 16
 1.027568388792026e+02, // 17
 4.309820308120030e+02, // 18
 1.163488224289240e+02, // 19
 1.181719075076786e+02, // 20
 1.144621486054318e+02, // 21
 1.605700540589699e+01, // 22
 8.033523511559293e-01, // 23
-6.435169094861140e+00, // 24
 5.072993060071077e+02, // 25
-1.866869571038190e+01, // 26
-6.720789652668044e+01, // 27
 1.862326940416779e+02, // 28
-5.323262887191718e+00, // 29
-3.128998064418820e+02, // 30
-8.226788411300820e+00, // 31
 1.025963149416750e+01, // 32
 1.118899416011454e+01, // 33
-9.051696440761605e+02, // 34
-1.004642455879279e+02, // 35
-5.864186213918679e+00, // 36
-1.483292832599617e+02, // 37
-3.743325930785792e+01, // 38
-3.816174920464491e+02, // 39
-3.514219038827239e+01, // 40
-1.750392106072302e+02, // 41
 2.645827896592643e-02, // 42
-3.583565856761929e+01, // 43
-4.719767184743825e+01, // 44
-8.767337549560980e+01, // 45
-1.537540629554698e+00, // 46
 7.852897957601699e+02, // 47
-3.339324055431832e+02, // 48
 5.495226393329747e+01, // 49
-2.549793360197792e+00, // 50
-2.867629402874784e+00, // 51
 7.833766939618680e+01, // 52
 4.224296260535560e+02, // 53
-9.625221929027978e+01, // 54
-1.798568692522924e+02, // 55
-3.207243048635257e-01, // 56
 7.655435177874907e+01, // 57
-1.590603436882650e+02, // 58
-9.527305499202123e+01, // 59
 4.653218255292521e+00, // 60
-1.046533914481788e+02, // 61
 6.469174122543440e+00, // 62
 1.041915327432917e+02, // 63
 5.980019023584839e+00, // 64
 1.507267312146688e+02, // 65
 5.696815558603389e+00, // 66
-6.539531748701400e+01, // 67
-8.248042748557141e+01, // 68
 2.021784851940937e+00, // 69
 3.204743499498806e-01, // 70
 1.843189889825329e+00, // 71
 1.166965064110522e+00, // 72
-5.064275086472949e+01, // 73
 5.531884435255108e+02, // 74
-1.149575618275514e+01, // 75
 8.008853840627217e-01, // 76
-1.594045054462613e+02, // 77
-6.095825100260884e+02, // 78
 2.095426497154324e+02, // 79
-1.497598578445013e-01, // 80
-3.566370306628239e+01, // 81
 2.351655269429706e+01, // 82
-1.490899947334889e-02, // 83
-6.423934552831240e-01, // 84
-1.692594492794068e+00, // 85
 3.468432579120423e+01, // 86
 1.273029651583161e+02, // 87
 8.022759181767239e+01, // 88
 5.335294732545164e+00, // 89
 1.785983960950850e+00, // 90
-6.181175549295900e+00, // 91
 1.999799439355768e-02, // 92
-2.824022450453774e+02, // 93
 1.472422926108176e+02, // 94
-7.207939182274224e+00, // 95
 1.019355883388374e+02, // 96
-5.961655534201045e+01, // 97
-1.992363302969528e+02, // 98
-2.679736951305131e+02, // 99
-4.750026598573231e+00, // 100
 6.034821912872976e-01, // 101
-2.126319739931299e+01, // 102
 8.508678993753959e+00, // 103
-2.973058927871054e+02, // 104
-1.548524504029952e+02, // 105
 1.799818877202566e+01, // 106
 1.217232945642461e+02, // 107
-5.705176670740070e+01, // 108
 3.938980220556177e+02, // 109
 7.721763190204864e-02, // 110
-2.284278244155553e+01, // 111
-3.450858421203424e-01, // 112
-1.528339856013008e-01, // 113
-4.000858555504816e+01, // 114
 1.830222943170503e+01, // 115
-6.084408742618992e-01, // 116
 1.703104315307452e+02, // 117
 1.304268442927757e+02, // 118
-1.002029779075090e-01, // 119
-3.543309925720083e+00, // 120
 2.769963117489027e+01, // 121
-8.594943453061779e-04, // 122
 1.113999011930677e+02, // 123
-8.227980929815716e+01, // 124
 6.991783608892566e-02, // 125
-5.565538111055108e-01, // 126
-5.139793715115899e+01, // 127
 2.822633224915939e+00, // 128
-2.285529418550970e+02, // 129
 1.438372427982243e+00, // 130
 1.198909914276824e+01, // 131
-4.078081071537218e+00, // 132
-8.691074857975950e+02, // 133
-5.738318987354275e-02, // 134
-5.238564977005973e-01, // 135
 2.962049761411509e-01, // 136
 1.593283600043940e+02, // 137
-1.313496768791616e+02, // 138
 3.677430460279775e+01, // 139
 1.036609768149679e-01, // 140
-1.132529012599359e+00, // 141
 7.749980993671573e+00, // 142
-1.344230583606974e+02, // 143
 1.705325591894819e+02, // 144
-1.176887733616356e-02, // 145
-6.744659093522952e+01, // 146
-1.939390395280690e+01, // 147
 2.852085637503777e+02, // 148
-1.859542291049455e-01, // 149
-6.952305397555860e+01, // 150
 9.014091605135033e-02, // 151
 2.090452914813847e+00, // 152
-9.601069854098226e-03, // 153
 1.122648963468695e-04, // 154
 7.236087783181647e-01, // 155
-1.219626584087989e+02, // 156
-2.401843600346155e+02, // 157
 5.004469833889289e-04, // 158
 2.529081987654369e+02, // 159
-5.227168526874219e-02, // 160
-3.995434620391005e+00, // 161
 9.473239414674892e+01, // 162
 4.268561216658744e+01, // 163
-2.317664899452661e+02, // 164
-6.882645464909726e+00, // 165
 2.425428180930703e+02, // 166
 3.383097759469614e-02, // 167
-9.350480289199218e+00, // 168
 2.184992967976506e+02, // 169
-5.488976900269952e+00, // 170
-7.393026949115038e-01, // 171
 6.331028694630935e+01, // 172
 4.853851950863070e+00, // 173
-5.089587833257195e-01, // 174
 7.340031321519069e-01, // 175
-1.050020386881979e-01, // 176
-2.769197804400043e+00, // 177
-1.344290878568555e+01, // 178
-1.392994399808315e+00, // 179
-3.352174108997562e-02, // 180
 9.309996252373402e+01, // 181
 7.561175810525057e-01, // 182
 2.467183573634047e-01, // 183
 4.863883222694594e+02, // 184
-1.437989981753425e+00, // 185
-2.805963311052766e-02, // 186
 5.904180641540469e+01, // 187
-2.014953786355942e-01, // 188
-5.878356678650661e-03, // 189
 7.505524343085490e+01, // 190
 2.124198759205388e+01, // 191
 1.760610549243840e-03, // 192
-6.173293541235062e-03, // 193
-5.628068459889748e+02, // 194
-7.022868005407361e+01, // 195
 2.837448441077472e-03, // 196
 3.383298402529046e+01, // 197
-1.783060654405141e+02, // 198
-2.276991598062443e-02, // 199
 2.665880891809331e+02, // 200
-7.105107329405330e-03, // 201
 2.632295350108074e-01, // 202
 3.414739111831985e-01, // 203
 2.944368502203097e+01, // 204
 6.999942968794683e+01, // 205
-1.072974074592071e+02, // 206
 8.626103293591679e-01, // 207
 2.137646947843202e+02, // 208
-5.728150664830042e+01, // 209
 1.995025667948458e-04, // 210
-1.356091782873596e-04, // 211
 1.384923978953582e+00, // 212
-1.084610743061221e-03, // 213
 1.577787357483005e+00, // 214
 3.312679107001226e-02, // 215
 7.807179488990220e+00, // 216
-8.067287547048666e+00, // 217
-7.636398563365814e+00, // 218
 2.011865194177411e-02, // 219
-3.225006293156824e-01, // 220
 6.763641810541347e-03, // 221
-3.018096631407269e-02, // 222
 1.117836550275886e-01, // 223
 1.074565771488582e-03, // 224
 2.148968058168648e+02, // 225
 1.076724354972741e-02, // 226
 2.450406903744500e-02, // 227
-2.325128667224221e-01, // 228
-1.734964420600775e+00, // 229
 9.614072388897029e+00, // 230
 9.548939417245561e-01, // 231
 3.661977129613391e+01, // 232
-8.689853947981722e-02, // 233
 2.505484307456556e-01, // 234
-2.319211745243558e-01, // 235
-9.503988412784843e-02, // 236
-9.762430964654634e-02, // 237
 3.252314095058579e+01, // 238
-1.150030099205926e+02, // 239
-9.486717485044539e+01, // 240
-7.050812537243227e-02, // 241
 4.664009426197411e-03, // 242
 2.013658863380010e+00, // 243
-4.856647950270352e-03, // 244
-6.604087967356099e-04, // 245
 1.809561116162714e+00, // 246
-4.042079941857146e-02, // 247
-4.761433780525480e+01, // 248
-4.260288250513409e-03, // 249
-9.142758563156135e-02, // 250
-5.182843548385144e-02, // 251
 4.275860578964878e+00, // 252
-5.621382949926767e+00, // 253
 1.894330890234537e+02, // 254
 1.584673538875140e-01, // 255
 3.677654663740523e-01, // 256
 6.985998133517215e+01, // 257
-5.426353868415988e-05, // 258
 2.680237297893418e+01, // 259
-2.589922628328453e+01, // 260
 2.489470019916599e-01, // 261
-7.094464894436708e-03, // 262
 1.475367275751049e-01, // 263
 1.022943625761789e-01, // 264
-8.813772464744861e+00, // 265
 1.632128425286037e-01, // 266
-4.105984565197807e+01, // 267
-6.465958254055699e-04, // 268
-2.960381112599121e-06, // 269
 8.474457713458081e+00, // 270
 6.571920990296497e-04, // 271
 5.917741699649988e+00, // 272
-6.108219656553278e-02, // 273
-4.382988204592767e-02, // 274
 1.519987740322311e-03, // 275
-1.343828089550206e+02, // 276
 1.016361711691187e-06, // 277
-7.313861779696953e+00, // 278
 1.562335290417145e+02, // 279
 7.450015459157741e+00, // 280
 3.278026076068886e+01, // 281
-9.500521151652019e+01, // 282
-2.327154902773039e+01, // 283
-1.069855511473522e-01, // 284
 2.497623246670957e-03, // 285
 8.363681086352607e+00, // 286
-2.409619390502991e-02, // 287
-7.244363923448665e-03, // 288
-2.027323035474896e+00, // 289
 8.039064836137108e-01, // 290
 1.644271696768232e-02, // 291
 1.279195534525243e+00, // 292
-1.335533447919865e-03, // 293
-1.793267896764539e+02, // 294
 7.065685028447358e+00, // 295
 1.559499984540122e+00, // 296
 6.339547415606330e+01, // 297
 6.350546244076574e-04, // 298
 4.649354935225189e-04, // 299
-3.093258052225476e+00, // 300
 1.111833478158043e+02, // 301
 1.113394867749934e-01, // 302
-4.796567690873883e-01, // 303
 2.763893868020264e-03, // 304
-2.198253480809245e+02, // 305
-7.222937380042052e+01, // 306
 4.755217453416128e-03, // 307
 2.776940644395106e+02, // 308
 6.071109963155369e-02, // 309
 3.126258748318150e+00, // 310
 7.275657864057067e-03, // 311
 1.531985254748205e-02, // 312
 1.091788846076583e+00, // 313
-6.449608712077173e-04, // 314
 4.039960262189708e-01, // 315
-3.294976520722815e+00, // 316
 3.143000159681665e+00, // 317
 1.509180956829632e+00, // 318
-2.095854029314125e+00, // 319
-5.092281284116512e-02, // 320
-6.317509860085363e+00, // 321
 1.512676974009626e+00, // 322
-2.660820973575233e-03, // 323
-1.514029405516586e+00, // 324
 9.138662086655916e+00, // 325
-2.560073449543568e+00, // 326
 9.891546727069410e-01, // 327
-3.846753984890051e+00, // 328
-1.524617853632414e-02, // 329
-5.503809262494674e-05, // 330
-5.328394738168300e+00, // 331
-4.043440231650934e+01, // 332
-5.472886019316644e-01, // 333
 8.246966103932465e+00, // 334
-3.435141605646787e-03, // 335
 7.484154762967350e-01, // 336
-6.814953576787188e-01, // 337
-1.184481648464447e+00, // 338
-1.780738395526458e+00, // 339
-6.722403412124316e-04, // 340
-1.851311106148573e+00, // 341
-8.018448385292258e-02, // 342
-2.325795743139898e+00, // 343
 1.645696241318401e-01, // 344
-6.286118166694905e+00, // 345
 7.741701450731026e-02, // 346
 2.749122853715247e+00, // 347
 2.654247838855907e+02, // 348
 1.627237936555241e-02, // 349
 7.897890522704410e-03, // 350
 4.984089024229726e-02, // 351
 2.364284149829793e+01, // 352
-2.526789182975553e-04, // 353
-4.054417488247577e-02, // 354
-5.776569404026253e+00, // 355
 3.880073972100020e+01, // 356
 2.566726196021383e-02, // 357
 2.130568185206688e+01, // 358
-4.785536831502481e-03, // 359
 7.614326262244596e-02, // 360
 8.443310311081844e-05, // 361
-5.630949466977446e+01, // 362
 6.775694120006787e-01, // 363
-4.305320953014244e-03, // 364
 4.656327590578354e+00, // 365
 1.392867320495234e-03, // 366
-1.873211951384931e+02, // 367
 3.843587917377590e-01, // 368
-4.556029316896095e-02, // 369
-9.903909741387860e-04, // 370
 6.607545272397598e+02, // 371
-4.766298486885994e-04, // 372
-7.987083369375117e-02, // 373
 6.106882443249910e+01, // 374
-2.284250468769152e+02, // 375
-6.148838097320591e+00, // 376
 6.019980408609470e+01, // 377
-1.357248315259191e+00, // 378
-6.579788014375160e-01, // 379
-5.150510645410470e-05, // 380
-4.549982213247120e-01, // 381
 4.236126269868614e-01, // 382
 7.774621746546354e-02, // 383
-6.674974823085169e+02, // 384
 1.119071795633172e+02, // 385
 2.286153305155112e-05, // 386
-2.829099442276282e+02, // 387
-3.206398036468251e+00, // 388
 1.862291105257610e+00, // 389
-6.045959065867330e-01, // 390
-9.271360118714425e+01, // 391
-4.922704542732896e+00, // 392
 7.946522453181287e-03, // 393
-1.807243048825412e-02, // 394
-4.965119156172035e-03, // 395
 1.269628597174779e-01, // 396
 3.524080296923881e-02, // 397
-1.044535979764166e+00, // 398
-2.598731829520767e-01, // 399
 7.241847550008365e-03, // 400
-2.703359829198348e-01, // 401
-1.188759830343957e-02, // 402
-1.243958373428192e+00, // 403
-6.762122337358464e-04, // 404
 1.170031469474551e-01, // 405
 1.048619143336960e-02, // 406
-1.271037868872907e-01, // 407
 1.252313574132073e-02, // 408
 2.217093180947701e-02, // 409
-2.426374003620405e+00, // 410
-1.861413270896985e+00, // 411
-5.180553500360096e-03, // 412
 1.459351898229490e+01, // 413
 7.436088420527437e-01, // 414
 1.824793574135581e-01, // 415
-5.321707818814363e-02, // 416
-1.873184628962536e-01, // 417
 6.655049620413054e+02, // 418
-4.423779790813189e-02, // 419
 2.041554378062008e+00, // 420
-4.092464971459847e+01, // 421
 8.920256117738579e-05, // 422
-1.850661652801404e+00, // 423
-6.077090158327547e-05, // 424
 1.706147739456462e-07, // 425
-3.574012033194768e-03, // 426
-1.170070286942761e+01, // 427
-1.342403106690647e+01  // 428
};

//----------------------------------------------------------------------------//

x2b_h2o_ion_v1x_p::x2b_h2o_ion_v1x_p()
{
    m_k_HH_intra =         1.287860253987811e-01; // A^(-1)
    m_k_OH_intra =         2.521312733790787e-01; // A^(-1)
                           
    m_k_XH_coul =          3.950180440069139e-01; // A^(-1)
    m_k_XO_coul =          1.163343409452539e+00; // A^(-1)
                           
    m_k_XLp_main =         7.102580751813152e-01; // A^(-1)
                           
    m_d_HH_intra =         1.793655232739329e+00; // A^(-1)
    m_d_OH_intra =         5.676853531148358e-01; // A^(-1)
                           
    m_d_XH_coul =          6.865462207536646e+00; // A^(-1)
    m_d_XO_coul =          6.998510996519467e+00; // A^(-1)
                           
    m_d_XLp_main =         6.539602724733552e+00; // A^(-1)
                           
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
void mbnrg_2b_h2o_i_poly(const double* w, const double* x ,
                    double* E, double* g1, double* g2)
#else
void mbnrg_2b_h2o_i_poly_(const double* w, const double* x ,
                    double* E, double* g1, double* g2)
#endif
{
    *E = the_model(w , x , g1, g2);
}

//----------------------------------------------------------------------------//

#ifdef BGQ
void mbnrg_2b_h2o_i_cutoff(double* r)
#else
void mbnrg_2b_h2o_i_cutoff_(double* r)
#endif
{
    *r = the_model.m_r2f;
}

//----------------------------------------------------------------------------//

} // extern "C"

////////////////////////////////////////////////////////////////////////////////
