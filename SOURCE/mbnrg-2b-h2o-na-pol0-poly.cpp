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
 1.114874625194155e+01, // 0
-1.392280802439622e+02, // 1
 1.698728586651959e+02, // 2
 7.792479223513266e+02, // 3
 8.210533520573966e+02, // 4
 1.648602207613349e+02, // 5
 2.121834767027918e+01, // 6
-2.120423050951069e+02, // 7
-3.159637230623922e+01, // 8
-2.264790645745899e+01, // 9
-3.770191951465837e+02, // 10
 1.452052776355698e+02, // 11
-3.883628131686191e+01, // 12
-4.653464925958957e+02, // 13
 1.706703927801058e+01, // 14
-4.692717466463515e+02, // 15
 3.157031674278255e+02, // 16
 4.923847100840660e+02, // 17
 3.720342216227721e+01, // 18
 6.750026732290128e+01, // 19
 1.424471266739745e-01, // 20
-9.324068222018752e+01, // 21
 3.483397853061628e+02, // 22
 9.002891153640316e+01, // 23
 1.129116870418761e+01, // 24
 3.549517604815480e+01, // 25
-4.419048754132601e+01, // 26
-2.649207632786891e+00, // 27
 3.413317398592998e+02, // 28
 7.520493446183839e+02, // 29
-1.671957363701054e+01, // 30
-1.576104691800795e+02, // 31
 2.788828636974372e+01, // 32
-1.754596202298942e+02, // 33
 1.961405180628919e+00, // 34
-4.837929775533633e-01, // 35
-7.637194585526713e+01, // 36
-7.514864418167376e+01, // 37
 5.274996781786719e+01, // 38
 1.306067195280435e-01, // 39
 4.345539927266763e+01, // 40
-8.709706025748503e+01, // 41
-2.040959775211554e+01, // 42
 3.796458107782410e+02, // 43
-5.768316788502573e+02, // 44
-4.492836884055338e+01, // 45
-1.765803002777456e+01, // 46
 5.167230026317952e+01, // 47
-3.561071672781663e+01, // 48
-4.294350415270305e+02, // 49
-3.878568623216025e+02, // 50
-1.762620944199057e+02, // 51
-6.241744145308080e-01, // 52
-4.695981684618813e+01, // 53
-3.356606592726620e+01, // 54
-2.159166263812912e+02, // 55
 7.142593436641353e+01, // 56
-9.120596960133701e+02, // 57
 1.999656358505969e+00, // 58
 6.718477236352274e+01, // 59
 3.758696274021242e+01, // 60
 7.138547064482158e+01, // 61
 7.428778413049815e+01, // 62
 2.621329500399539e+02, // 63
 4.475096469293125e+02, // 64
-9.610533997230696e+02, // 65
 3.298441417950938e+02, // 66
-1.365261353254778e+00, // 67
 8.679905500602804e-01, // 68
 2.039247557003460e+02, // 69
 3.080504619966198e+00, // 70
-1.051847089833117e+03, // 71
-2.860073515548039e+01, // 72
 6.214948359376247e+02, // 73
 2.338315490279646e+00, // 74
 2.331823266655635e+02, // 75
 3.732912191939026e+01, // 76
 4.172175844944661e+02, // 77
-5.365926183772510e+00, // 78
 3.221080820181662e+02, // 79
 1.368590679044756e+02, // 80
 3.638036334464650e+01, // 81
-7.463328341386346e-01, // 82
-9.248309113241677e+01, // 83
 3.668539579821718e+02, // 84
 5.659912675152029e+01, // 85
 3.751405009184210e+00, // 86
 9.568573495597603e+01, // 87
-8.363292890177509e+01, // 88
 1.632213744624065e+02, // 89
-4.269761048111224e+02, // 90
-1.332563785455355e+01, // 91
 7.756175748783615e-01, // 92
-4.105443703605666e+01, // 93
 4.889531037949684e+01, // 94
-5.042045724352604e+00, // 95
-1.355667632640264e+02, // 96
-3.154649605477731e+01, // 97
-3.709325523564285e+00, // 98
-7.446623116421144e-01, // 99
 3.171045519301057e+02, // 100
 7.062296990676846e+01, // 101
-1.646055632433750e+01, // 102
 1.867555965005234e+01, // 103
-3.268487717268563e+01, // 104
-1.157665820389149e+01, // 105
 1.438280176905843e+02, // 106
 2.913427885685006e+00, // 107
-4.537165467659422e+02, // 108
 2.345917034093169e+00, // 109
 1.228828053916307e+02, // 110
 4.327690200614029e+02, // 111
 5.930081482214706e+00, // 112
-1.681426370255053e+02, // 113
-1.155686028463594e+01, // 114
-4.880916859335315e-01, // 115
 2.674975199565409e+02, // 116
-7.001831259771795e-01, // 117
-6.372781919547267e-03, // 118
 2.677734181336534e+00, // 119
-4.596819629531301e+02, // 120
 2.395801213762175e+00, // 121
 1.444414169191474e+00, // 122
-2.217483755546223e+00, // 123
-4.412249893129190e+01, // 124
 2.725585338817936e+01, // 125
 6.154462290394183e+01, // 126
-1.497459068182148e+00, // 127
-5.666857133238220e+00, // 128
-7.743174494224741e+02, // 129
 1.260662586987349e-01, // 130
 9.874702363813597e+02, // 131
-1.167916290316971e+00, // 132
-2.984456561142025e+00, // 133
-2.264408457230843e+01, // 134
 1.002547507848218e+02, // 135
 9.321806967304135e+02, // 136
 1.758592451464721e+01, // 137
 2.707399659976510e-01, // 138
-8.339170429987173e+02, // 139
 4.014237214371485e+02, // 140
 1.798738385875854e+01, // 141
-7.259270683193208e+00, // 142
 3.840012660226182e+01, // 143
 2.176711273196029e+00, // 144
-2.148042809190680e+00, // 145
 2.860481416691185e+01, // 146
-3.664516428195104e+01, // 147
-1.828679854744971e+00, // 148
-1.461106125312986e+01, // 149
 9.573563218568795e+01, // 150
-6.397033827386170e+01, // 151
 2.221676375965800e+02, // 152
-2.594651838570510e+01, // 153
-5.248959857450266e-03, // 154
-1.621309011513743e+02, // 155
-2.317538783924186e+00, // 156
-1.973224764602754e+02, // 157
 4.415137777607158e+00, // 158
-1.945064807830734e+01, // 159
-1.717942099516946e+01, // 160
 1.354260783049413e+02, // 161
 1.976019196907990e+01, // 162
 1.318919964032347e+02, // 163
 8.238048999329536e+01, // 164
 1.789168947639984e+01, // 165
 2.646359982937143e+00, // 166
-8.325509228323078e+00, // 167
 1.055942806916749e+01, // 168
-3.228979124497556e+01, // 169
-6.157539905059797e-01, // 170
-1.753517362055635e+02, // 171
-4.029962434349189e+01, // 172
 3.832101063628935e+00, // 173
 1.254615606667002e+02, // 174
-3.548682572589763e+02, // 175
-8.396057223650439e+01, // 176
 4.807491751564310e+02, // 177
 2.924332226805577e-01, // 178
 7.175175551768259e+02, // 179
-5.350072037437975e-01, // 180
 4.151499756311924e+00, // 181
-1.139475318517649e+02, // 182
-7.530524276372911e+02, // 183
-9.001634503829442e-01, // 184
-1.161755096133525e+03, // 185
 1.393762654764850e+02, // 186
 2.593169725272149e-01, // 187
-1.564298188060763e+00, // 188
-1.003134413976246e+00, // 189
 2.548999240794092e+00, // 190
-9.049993668571316e-01, // 191
-3.366321248170882e-01, // 192
 4.684290559099798e+00, // 193
 1.275880259043551e+00, // 194
 1.732180659081209e+02, // 195
 7.608957151002667e+00, // 196
 1.758779899759130e+00, // 197
-8.072943619526204e-01, // 198
-2.848213875215229e+01, // 199
 3.510961935311513e+01, // 200
-5.559464062339888e-01, // 201
-2.320857495933878e+01, // 202
 6.938807405983015e+00, // 203
 2.266643688194902e+00, // 204
-1.532928005600155e+01, // 205
 1.696019703499272e-01, // 206
 1.003416991117861e+00, // 207
 6.376867816701262e-01, // 208
-1.108527319086367e+00, // 209
 1.858183324416514e-01, // 210
 1.238642600132655e-01, // 211
 1.412739173990157e+03, // 212
-1.261151827747909e-01, // 213
 1.242941002382577e+00, // 214
 6.750728774896426e+01, // 215
-5.014546578887016e-01, // 216
-1.031225233889713e+00, // 217
-9.412806368226628e+00, // 218
 4.316901634017863e+00, // 219
 4.338443209823068e+00, // 220
 9.050113194526581e-01, // 221
 1.460752084239953e-01, // 222
-2.194923752597870e-01, // 223
 1.893626614375002e+00, // 224
 2.871054195994188e+00, // 225
 7.068544601395620e+00, // 226
 2.755205296485640e+00, // 227
-2.038446925255958e+00, // 228
 6.587031258636750e+02, // 229
-1.066920835854303e+00, // 230
 1.201708319004876e-02, // 231
 2.471438157658401e+00, // 232
 1.426005795042605e+02, // 233
 1.836710077355070e+00, // 234
 4.597808802309272e+02, // 235
-1.405100717535467e-02, // 236
 5.062608610847888e+01, // 237
-7.370434083173607e+02, // 238
-8.128920814288042e-01, // 239
 1.763062314161384e+03, // 240
 9.449016577546536e-01, // 241
-4.681029951089443e+01, // 242
-2.848831130812976e+02, // 243
 8.246353410036340e+00, // 244
 1.083423205494992e+00, // 245
 3.816890862029663e-01, // 246
 1.534376343797636e+00, // 247
-1.830416267135622e-01, // 248
 1.912873671816679e+01, // 249
-5.331719557937982e+02, // 250
 4.461766519770465e-01, // 251
 4.352632277877231e+02, // 252
 8.031985704295463e+00, // 253
 1.827763387795260e+00, // 254
 9.175545565818110e+00, // 255
-1.023267607567533e+00, // 256
 3.230805858512095e+01, // 257
-2.400385093294114e+00, // 258
 2.740431672937712e+01, // 259
-8.364676369356239e-01, // 260
-7.989994152014193e-02, // 261
-2.168004641906124e+01, // 262
 3.645541307226071e+00, // 263
-5.245575872708362e+00, // 264
 3.083067858140677e+02, // 265
-3.877454799643797e+01, // 266
 8.674144978069325e+00, // 267
-4.559623456274434e-02, // 268
-1.440206743787074e-02, // 269
 1.248559560245471e+00, // 270
-4.818463406433245e-01, // 271
 7.574395160185048e-01, // 272
-2.219727630116625e+02, // 273
-1.688870475764388e+00, // 274
 3.112722238416392e+00, // 275
-8.568610310367704e+00, // 276
-1.497673064864493e-02, // 277
 1.334291713580965e+00, // 278
 1.686424261244025e+02, // 279
-1.640703079156454e+02, // 280
-1.103363739569851e+00, // 281
 6.511323054725859e+02, // 282
-1.036005020002490e+01, // 283
-1.805113814615374e+02, // 284
-5.222892194277994e+00, // 285
 1.214435076917152e+02, // 286
 3.699115754744986e+00, // 287
-2.330936958707273e-01, // 288
 5.582427385071080e+00, // 289
-5.293675917815178e+00, // 290
 5.569128679167905e+01, // 291
 2.195049950818408e-01, // 292
 2.459019211232702e+01, // 293
-3.702175671934383e-01, // 294
-3.231548381503636e+02, // 295
 4.302268335557559e-01, // 296
 1.414731762138796e-01, // 297
-1.202639009625953e+00, // 298
-2.456203979092671e-01, // 299
-9.765179925036106e-01, // 300
-8.355794185057215e+01, // 301
-1.202117985965339e+02, // 302
-5.500253371614224e-01, // 303
-9.255740570636809e-01, // 304
-1.511698676309417e+00, // 305
-1.347896814017533e+01, // 306
-2.593562575016558e+00, // 307
 2.904392107272920e+00, // 308
 3.674266559055233e+01, // 309
 6.544184739492037e-02, // 310
 3.343446589094379e+01, // 311
-1.021052615813464e-02, // 312
-5.247000275512235e-02, // 313
-6.826856720827581e-01, // 314
-1.491459699929801e+01, // 315
 1.583396193582754e+01, // 316
 8.545991294784802e-02, // 317
-5.023607577172719e+01, // 318
-9.898672675074471e+02, // 319
 4.148362152565124e-02, // 320
-3.557241436678092e-01, // 321
-4.545516409646611e+00, // 322
-1.140566084395559e+01, // 323
-8.635086613311303e+00, // 324
-1.972541380306790e-01, // 325
 1.174379790929116e+01, // 326
 8.453691803144608e+01, // 327
-6.015995368226539e-01, // 328
 5.913022749089081e+01, // 329
 6.342954291678604e-02, // 330
-7.085063499330451e+00, // 331
-1.266629822653154e+00, // 332
-7.548103952263501e+00, // 333
-4.079422470277747e+00, // 334
-8.660033985848555e-01, // 335
-9.682883721836092e+00, // 336
-5.813774725329626e-01, // 337
-2.919845795444085e-01, // 338
 3.837206372385237e+00, // 339
 1.365219808999826e+01, // 340
 1.390121033543334e+02, // 341
-3.264894189724884e+00, // 342
 6.863338788647349e+00, // 343
 3.505640733308688e+00, // 344
-7.048605298987248e-02, // 345
-1.960414530961892e+01, // 346
-1.826299921418051e+01, // 347
 2.631401394943892e+00, // 348
-3.768936444009984e+02, // 349
 4.688951363027688e-01, // 350
-4.606255464950427e+00, // 351
-2.221803035206337e+01, // 352
 1.399043899105245e+00, // 353
-8.689231867949811e+00, // 354
 1.691124622333338e-01, // 355
-3.323213696060329e+00, // 356
 1.530510875166253e+01, // 357
 2.441647702808222e+00, // 358
-4.023952752927858e+01, // 359
 1.043557137573055e-01, // 360
 6.015104948433586e-03, // 361
 6.682948039457230e-01, // 362
-9.456553434007308e+00, // 363
-2.953678415924416e-03, // 364
 6.481223055528095e+00, // 365
-8.504234773606814e+00, // 366
 1.427827528368575e+01, // 367
 2.815069812236429e+00, // 368
 3.714633521075089e-01, // 369
 4.098597725628894e+00, // 370
 2.918715875570372e-01, // 371
 5.142192954824663e-02, // 372
-2.972219593179631e+02, // 373
-1.118995580109641e+01, // 374
-3.858495105221124e+00, // 375
-6.200245111547895e-01, // 376
-3.207275554160645e+00, // 377
-8.206938582442487e+01, // 378
-2.048344491375887e+00, // 379
 2.937520658097248e+01, // 380
-7.067014172460794e-01, // 381
 5.547348686432350e+00, // 382
 4.668768419760863e+02, // 383
-1.082396653321835e+00, // 384
-5.850534251061474e+00, // 385
-1.060033718980315e-01, // 386
 2.207097116676641e-01, // 387
-2.301796424557802e+00, // 388
-6.736704582403734e+00, // 389
-5.369664423158307e-02, // 390
-1.432045091196551e+00, // 391
-9.502517587015149e-01, // 392
-6.266773161051472e+01, // 393
-1.473946229015096e+02, // 394
 1.024056982946147e+02, // 395
-7.210222984331021e-01, // 396
-1.019130133650579e+02, // 397
 6.972734308568870e+01, // 398
 6.404966591596535e-01, // 399
 1.634301503459131e+00, // 400
 5.347608561335524e+01, // 401
-5.795323750197419e+00, // 402
-8.541614221537742e+00, // 403
-5.881822784601862e+01, // 404
 5.107103347720154e+01, // 405
-1.003863161235391e-03, // 406
 1.888986347185558e+02, // 407
-4.352765140012620e-04, // 408
-9.341870476031352e+01, // 409
 1.413019033731703e+01, // 410
-1.688370049437318e-01, // 411
 2.048761612108636e+01, // 412
 1.427176291627693e+00, // 413
 2.789062740014767e+00, // 414
 8.483566576635144e+01, // 415
-1.919455805733226e-01, // 416
 3.503155401514109e+01, // 417
 4.272765200328918e+00, // 418
-2.627961963918309e+01, // 419
-4.158672960643414e+02, // 420
-1.136051892025480e-01, // 421
 8.286306391593100e-02, // 422
-4.028399286500393e+00, // 423
 2.489151027697777e-02, // 424
 7.500433430865876e-04, // 425
-3.134736722614716e+01, // 426
 3.920527931823975e+02, // 427
 7.553232746626447e-01 // 428
};

//----------------------------------------------------------------------------//

x2b_h2o_ion_v1x_p::x2b_h2o_ion_v1x_p()
{
    m_k_HH_intra =         4.486597767562190e-01; // A^(-1)
    m_k_OH_intra =         1.999999985087912e+00; // A^(-1)
                           
    m_k_XH_coul =          1.137553081822990e-01; // A^(-1)
    m_k_XO_coul =          6.464154361224240e-01; // A^(-1)
                           
    m_k_XLp_main =         8.519110931821103e-01; // A^(-1)
                           
    m_d_HH_intra =         9.527622551741199e-01; // A^(-1)
    m_d_OH_intra =         1.999985130842382e+00; // A^(-1)
                           
    m_d_XH_coul =          6.718134294113021e+00; // A^(-1)
    m_d_XO_coul =          6.880638895624118e+00; // A^(-1)
                           
    m_d_XLp_main =         3.165068379361477e+00; // A^(-1)
                           
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
void mbnrg_2b_h2o_na_poly(const double* w, const double* x ,
                    double* E, double* g1, double* g2)
#else
void mbnrg_2b_h2o_na_poly_(const double* w, const double* x ,
                    double* E, double* g1, double* g2)
#endif
{
    *E = the_model(w , x , g1, g2);
}

//----------------------------------------------------------------------------//

#ifdef BGQ
void mbnrg_2b_h2o_na_cutoff(double* r)
#else
void mbnrg_2b_h2o_na_cutoff_(double* r)
#endif
{
    *r = the_model.m_r2f;
}

//----------------------------------------------------------------------------//

} // extern "C"

////////////////////////////////////////////////////////////////////////////////