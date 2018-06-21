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
 2.099911490487774e+02, // 0
-4.270474050856164e+02, // 1
 1.141169325279117e+02, // 2
 1.435313431772710e+02, // 3
 4.532695933942335e+02, // 4
-1.895637848313392e+01, // 5
 7.306551700694378e+02, // 6
-5.191961481854613e+02, // 7
-1.346024576330687e+02, // 8
-1.769826336552325e+02, // 9
-8.544401404675277e+01, // 10
 1.626998302398061e+02, // 11
 3.763968535503512e+02, // 12
-4.780005237711242e+02, // 13
-5.709808666178494e+02, // 14
 5.496141897795825e+01, // 15
-5.195739254204595e+02, // 16
 8.790794812012484e+01, // 17
 2.785104578795243e+02, // 18
 1.133084457909239e+02, // 19
 4.791637503796567e+02, // 20
 3.569940198524319e+02, // 21
 7.021838459984995e+00, // 22
 5.842352725980977e+01, // 23
-1.960897443508604e+02, // 24
 1.093676900562188e+03, // 25
 1.084543490023325e+02, // 26
 2.190600834286962e+02, // 27
 1.020105930216965e+02, // 28
-1.258570457029468e+02, // 29
 1.138613903513463e+02, // 30
 2.965377263488955e+00, // 31
 3.437759387216269e+01, // 32
 2.367203117823786e+02, // 33
-9.113665292082636e+02, // 34
 2.621389242452431e+01, // 35
-6.455165233209998e+01, // 36
-3.875142894701284e+02, // 37
 2.069419442364964e+00, // 38
-1.174879561021920e+02, // 39
 1.303821512956870e+01, // 40
-3.836318363322674e+02, // 41
-1.350900899223512e+01, // 42
 4.582055301967380e+01, // 43
-6.512725424224483e+01, // 44
-6.614417038815917e+01, // 45
 1.555221800248965e+01, // 46
 1.062471550154020e+03, // 47
-9.979192120546137e+02, // 48
 1.587517307334635e+02, // 49
-4.001052371199406e+00, // 50
-4.240643451623033e+01, // 51
 1.325064378849300e+02, // 52
 1.570043898867152e+02, // 53
-2.617667533042608e+02, // 54
-1.847134558322151e+02, // 55
 1.411772868170387e+01, // 56
 6.884976801749602e+01, // 57
-8.746878951110283e+01, // 58
-9.530407531320060e+01, // 59
-3.771221936517504e+02, // 60
-1.162252118125729e+02, // 61
 1.595609132261478e+01, // 62
-2.384700954759631e+02, // 63
-2.942543429801270e-01, // 64
-1.393722214854140e+02, // 65
-1.312262983816558e+01, // 66
-1.185581831471331e+02, // 67
-1.483389033300795e+01, // 68
 2.764301914251456e+00, // 69
-1.156173849299114e+01, // 70
 1.227956464088171e+00, // 71
 4.749278416253991e+01, // 72
-7.394208618155757e+01, // 73
 1.880073061400250e+02, // 74
-1.945084835335760e+02, // 75
-7.067174380985779e+00, // 76
-7.567861165386779e+01, // 77
-1.837641960247569e+02, // 78
 3.649996269774474e+02, // 79
 9.078532855899399e-02, // 80
-5.776853680679815e+01, // 81
 4.341519185964950e+01, // 82
 6.234962046905657e+00, // 83
-5.444532242523682e+00, // 84
-2.358453317062497e+00, // 85
 4.130313402969500e+01, // 86
 7.541836209428268e+01, // 87
 1.342655448911121e+02, // 88
 3.183616567903636e+01, // 89
-5.824797498575155e+00, // 90
 8.322552900043963e+00, // 91
 4.419102893968038e+00, // 92
-4.558015722142159e+02, // 93
-2.713186126391845e+01, // 94
 7.209116044211752e+00, // 95
-5.858318247155521e-01, // 96
-4.381851454947152e+01, // 97
-1.414651156320693e+01, // 98
-5.279195282375459e+02, // 99
-1.015884691448636e+02, // 100
 7.316548373022822e+00, // 101
-4.947604832460581e+01, // 102
-1.930430968756242e+00, // 103
-4.776520106676981e+01, // 104
-1.766614212681481e+02, // 105
-3.969814742908662e+01, // 106
-9.138692993437024e+01, // 107
-1.478345954352384e+02, // 108
-1.868441343707985e+02, // 109
-1.192804328680626e+00, // 110
-1.062439288405511e+00, // 111
-7.269561716780825e+01, // 112
-7.192082503533149e+00, // 113
-1.978168222587016e+01, // 114
-8.496547860715442e+01, // 115
 1.897332000275427e+00, // 116
 1.678284612794055e+02, // 117
-5.829651179050371e+01, // 118
-5.650827516187386e+01, // 119
-1.751581570196698e-01, // 120
 2.326907908148371e+01, // 121
 1.312693378407518e+00, // 122
-8.173454229144862e-01, // 123
-6.022601058071406e+01, // 124
 8.685085198535410e-02, // 125
 8.114845019594462e-02, // 126
-1.131677442614252e+02, // 127
 3.560442784997675e+00, // 128
-1.287441100476528e+02, // 129
 1.400584408812770e+02, // 130
-1.467082523362699e+01, // 131
 1.198822383267651e-01, // 132
-4.507719442018283e+02, // 133
 3.799428001188679e+01, // 134
 1.790456367944976e+01, // 135
 2.956677399533482e-01, // 136
 7.535952010877480e+01, // 137
-4.152104683341776e+02, // 138
 1.433365615844368e+02, // 139
-3.836007902034920e+00, // 140
-4.479619516197937e+00, // 141
 5.461062932819713e+01, // 142
 9.340406440626631e+01, // 143
 1.790561271448809e+02, // 144
-6.900076319395512e-01, // 145
-1.511016408931318e+01, // 146
-4.365130917682951e+01, // 147
 3.195808339621516e+02, // 148
-6.456300696485497e+00, // 149
-1.386470389611271e+01, // 150
 3.283002233114014e-02, // 151
-8.379989809738214e-01, // 152
 3.834537179211644e+00, // 153
-7.136490483672443e-01, // 154
 3.660527706160326e-01, // 155
-1.359595274302189e+02, // 156
 1.682718777151732e+02, // 157
-4.062312304998691e+00, // 158
-1.566057770507613e+01, // 159
 9.466156064885345e-01, // 160
 6.719166104330558e+00, // 161
 2.875875562138967e+02, // 162
 2.353916782012868e+02, // 163
-1.558231417492624e+02, // 164
 6.714763281782955e+01, // 165
 1.562697678059000e+01, // 166
-2.158550332260337e+00, // 167
 9.237976319800140e+01, // 168
-2.485255244824515e+02, // 169
-3.310610156190215e+01, // 170
-4.245106374364018e+01, // 171
 5.403374681264453e+02, // 172
 3.908128137937545e+02, // 173
-1.992515811312689e-01, // 174
 5.777301897498623e+00, // 175
-2.296093841555224e+00, // 176
-5.845601644100179e-03, // 177
 3.415204343263814e+02, // 178
-6.430137481372845e-01, // 179
 1.292689013091602e+01, // 180
 3.278333673326489e+02, // 181
-1.101311043955697e-01, // 182
 3.371140212164546e-01, // 183
 9.560728128280621e+01, // 184
 3.377908735000561e+00, // 185
-1.546806381219859e-01, // 186
 3.418986637149223e+02, // 187
-1.050432011243438e+00, // 188
-2.372966734896786e-02, // 189
 1.348411156009297e+02, // 190
-1.913918332724182e+02, // 191
 3.080543723240360e+00, // 192
 1.975096184038506e-01, // 193
-1.037539369650309e+02, // 194
-1.188402843751364e+01, // 195
 9.413321668970999e-02, // 196
-7.350035362347258e+01, // 197
-3.462096569810819e+01, // 198
 1.377738634903702e+00, // 199
-1.601960416502958e+02, // 200
-8.178245792083345e+00, // 201
 4.860396337242611e+00, // 202
 3.169589563520466e-02, // 203
 1.807361100119689e+01, // 204
 1.535646665178944e+01, // 205
-3.627702254716894e+02, // 206
 6.039370026719256e-01, // 207
 2.768596146360074e+01, // 208
-2.282825751379875e+01, // 209
 4.894627134772379e-02, // 210
-2.452955950456037e-01, // 211
 7.400363839160001e+00, // 212
-1.144901167063000e+00, // 213
 1.172906228681931e+01, // 214
 1.825646690162628e-02, // 215
 2.045950658928032e+00, // 216
-2.243626649860275e+02, // 217
 7.773437380343922e+00, // 218
-3.425954340366878e-01, // 219
 1.406052215811781e+01, // 220
 1.279947622211064e+00, // 221
 4.833405603145427e+00, // 222
-1.268019336362187e+01, // 223
-3.686609932188503e+00, // 224
 2.785510090680650e+02, // 225
 8.111735642790169e-01, // 226
 8.031869800996348e-02, // 227
 5.328030819036798e-01, // 228
 1.262218691788634e+00, // 229
 3.235372652203307e+01, // 230
 4.277255533742018e+01, // 231
 2.957657348554453e+01, // 232
-2.213541789700124e-01, // 233
 3.565319794986003e+01, // 234
-2.975731519671103e-02, // 235
 1.282715010090675e+01, // 236
 4.479419820476143e-02, // 237
 1.663033578624453e+01, // 238
 5.168404531910389e+00, // 239
-2.144283696554017e+01, // 240
-1.866210351783745e+00, // 241
-2.679158435716525e-01, // 242
 3.880836702884823e+01, // 243
 1.803890019197273e+00, // 244
 8.813385131281200e-02, // 245
-6.058488910080873e-02, // 246
-2.271562856607223e-01, // 247
-3.121254461515198e+01, // 248
-5.301922964459535e-02, // 249
-2.090584787394552e-01, // 250
-3.672310527531050e-02, // 251
-9.274351510938810e-01, // 252
-4.313583245648621e+00, // 253
 1.384774646335883e+02, // 254
 8.919862812451815e-02, // 255
 8.561378481985525e-01, // 256
 2.687167267210741e+01, // 257
-1.511322666393536e-01, // 258
 1.891218383429045e+01, // 259
 1.322136101704825e+02, // 260
 7.034833704567392e+00, // 261
 7.340525296849610e-01, // 262
 4.359223956781642e-01, // 263
 3.684637448418731e-01, // 264
-4.460519462602413e+01, // 265
 5.256206428662368e+00, // 266
-5.391988642419738e+01, // 267
 2.974671734804614e-01, // 268
 1.368839593342983e-01, // 269
 4.076257085191294e+01, // 270
 1.490219441277873e-01, // 271
 3.642834414933573e+00, // 272
 5.512939566386096e-02, // 273
 1.531680673186566e-01, // 274
 5.479765495943983e-02, // 275
-4.409001471783682e+01, // 276
-5.532624786091031e-02, // 277
-1.202861528991457e+02, // 278
 1.184221599455443e+02, // 279
 4.954293202368984e+00, // 280
 1.308782646050902e+02, // 281
-6.947475947208311e+01, // 282
 4.467884044197839e+00, // 283
-4.578071782137823e+00, // 284
-5.061163066839329e-03, // 285
 5.656981408042793e-01, // 286
-5.231145472470043e+00, // 287
-1.257352584316893e-02, // 288
-9.329626504790894e-01, // 289
 7.064320778663900e-01, // 290
-3.051602953364907e-01, // 291
-4.528907746172434e+00, // 292
-3.802713997985290e-02, // 293
-1.461073983041659e+02, // 294
 1.245542875805877e+02, // 295
-6.945583614443692e+01, // 296
 4.176255323448269e+01, // 297
 2.923725405136930e+00, // 298
-8.529729901130630e-01, // 299
-6.210877409502734e-01, // 300
-4.536998400976087e+00, // 301
-6.833944136170961e-01, // 302
-6.544746154250835e+00, // 303
 6.715902902712131e-01, // 304
 8.960883431970518e+01, // 305
-2.535014562551085e+01, // 306
 2.149556177334678e-01, // 307
 1.710265104300842e+02, // 308
-1.950982602924346e-01, // 309
-2.377671473003886e-01, // 310
 8.347668788375121e-03, // 311
-1.663292628336924e-02, // 312
-1.841249616436209e+01, // 313
 3.507618533730073e-02, // 314
-1.804574104826236e-01, // 315
-8.147995555195701e+00, // 316
 1.554507787514702e+00, // 317
 2.455741675399241e+00, // 318
 6.985615665369050e-01, // 319
 6.280402476554923e-01, // 320
 3.043894730126510e-01, // 321
 1.115199851999792e+01, // 322
-1.880144563578880e-02, // 323
-3.091136533016201e+01, // 324
-4.557970285081498e+00, // 325
-1.826264879005222e+00, // 326
 3.947229206138796e+00, // 327
-3.794367029742251e+02, // 328
 7.763797341823478e-02, // 329
 9.577899092081761e-02, // 330
-1.433523666170377e+00, // 331
-1.636539986487833e+01, // 332
-3.012184615479464e-02, // 333
 1.209398859685624e+02, // 334
-1.265185427757567e+00, // 335
-1.303320652185977e-01, // 336
-2.379713362437142e+00, // 337
-4.516183126504590e-01, // 338
-4.828382836933658e+00, // 339
 6.759077810588998e-01, // 340
-2.819323254259203e+00, // 341
-4.758712589849846e-01, // 342
-2.622766970331986e+01, // 343
 6.545826253877678e+00, // 344
 2.322115194672490e-01, // 345
-6.123557636363235e+00, // 346
-5.095840652960875e+00, // 347
-1.256346489504103e+00, // 348
 3.874760774391241e-02, // 349
-3.076453025319454e-01, // 350
-1.084538382233440e+00, // 351
-5.066557011683367e+00, // 352
 6.842893183901609e+00, // 353
-6.842650671370973e-01, // 354
 9.292748535286690e+01, // 355
 4.987488844143191e+01, // 356
 1.010352043163217e+00, // 357
 7.659979507194102e+01, // 358
-4.934713737337814e-03, // 359
 1.715245691252154e+01, // 360
-2.007916800933747e-01, // 361
-7.388178262222538e+00, // 362
 3.640431610729479e-01, // 363
-5.463926151340109e+00, // 364
 1.261240383698944e-01, // 365
 2.809240356881678e-01, // 366
-8.726877344881487e+01, // 367
 1.480118032201930e+01, // 368
 5.931710618938319e+01, // 369
-1.275248168797422e+00, // 370
 7.532406179772709e+02, // 371
-2.120962612428252e+00, // 372
-3.829386848882054e-01, // 373
 1.450470367547163e+01, // 374
 1.128041201499419e+00, // 375
-5.643220510412437e+01, // 376
 3.907370230038466e+01, // 377
-4.741558897028948e-01, // 378
-8.454462268195957e-01, // 379
 4.757310943022652e+00, // 380
-4.860471879244251e+01, // 381
 1.505129553655873e+00, // 382
 2.017781898705338e-01, // 383
-1.289428470982153e+02, // 384
 1.631473821338350e+01, // 385
-4.252686439688716e-02, // 386
-4.786733765271741e+02, // 387
-4.304350016363268e-01, // 388
-1.914399591997088e+00, // 389
-1.330248902273394e+02, // 390
-6.915924585908759e+00, // 391
-4.523125715475144e+01, // 392
-2.529303806405293e-01, // 393
 3.399306209020225e-01, // 394
-1.821153149483920e-01, // 395
 2.545293209049488e+01, // 396
-1.594468551576019e-01, // 397
-6.812375371321203e-01, // 398
-2.483021965828419e+00, // 399
-1.853429747391041e+00, // 400
 1.711342667877688e+00, // 401
-7.157472772364147e-02, // 402
-1.681469664876086e+00, // 403
 3.495705625770166e-03, // 404
 3.725151372689905e-02, // 405
-6.883215515866039e-01, // 406
-2.643394932071886e+00, // 407
-1.313311174019055e+01, // 408
 2.195492847130801e-03, // 409
-3.061431211338337e-01, // 410
-7.938503355526551e+00, // 411
 1.434784119720624e-04, // 412
-1.543242237946420e+02, // 413
-3.671786156919332e+00, // 414
 3.419692714110547e+00, // 415
-2.385485712119160e+01, // 416
-4.713920933393961e+01, // 417
 9.974873831510145e+01, // 418
-1.702439163980928e+01, // 419
 1.562219237566780e+00, // 420
 7.213238551406292e+01, // 421
-1.027793083772419e-01, // 422
-6.512868448537750e+00, // 423
 5.235563144741348e-01, // 424
-6.595660556386451e-02, // 425
-6.292996483411406e-02, // 426
-1.274949323985538e+01, // 427
 5.210684213014147e+00  // 428
};

//----------------------------------------------------------------------------//

x2b_h2o_ion_v1x_p::x2b_h2o_ion_v1x_p()
{
    m_k_HH_intra =         2.951167833464670e-01; // A^(-1)
    m_k_OH_intra =         3.331141943760614e-01; // A^(-1)
                           
    m_k_XH_coul =          5.661597529227129e-01; // A^(-1)
    m_k_XO_coul =          8.076624979285920e-01; // A^(-1)
                           
    m_k_XLp_main =         1.009430529406921e+00; // A^(-1)
                           
    m_d_HH_intra =         1.153140899745128e+00; // A^(-1)
    m_d_OH_intra =         1.427296477192937e+00; // A^(-1)
                           
    m_d_XH_coul =          4.309935170051643e+00; // A^(-1)
    m_d_XO_coul =          6.984688951176864e+00; // A^(-1)
                           
    m_d_XLp_main =         5.515897876336050e+00; // A^(-1)
                           
    m_in_plane_gamma =     -9.721486914088159e-02;
    m_out_of_plane_gamma=  9.859272078406150e-02;

    m_r2i =  5.500000000000000e+00; // A
    m_r2f =  6.500000000000000e+00; // A

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
void mbnrg_2b_h2o_br_poly(const double* w, const double* x ,
                    double* E, double* g1, double* g2)
#else
void mbnrg_2b_h2o_br_poly_(const double* w, const double* x ,
                    double* E, double* g1, double* g2)
#endif
{
    *E = the_model(w , x , g1, g2);
}

//----------------------------------------------------------------------------//

#ifdef BGQ
void mbnrg_2b_h2o_br_cutoff(double* r)
#else
void mbnrg_2b_h2o_br_cutoff_(double* r)
#endif
{
    *r = the_model.m_r2f;
}

//----------------------------------------------------------------------------//

} // extern "C"

////////////////////////////////////////////////////////////////////////////////