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
 1.980059421531693e+01, // 0
-1.335498419822311e+02, // 1
 1.847637929920124e+01, // 2
 9.220543731123132e+01, // 3
 7.302451283841117e+01, // 4
-1.067343350815857e+02, // 5
 5.583463850614897e+02, // 6
-4.304961166367229e+02, // 7
 1.957164649046066e+02, // 8
-3.558631282396130e+00, // 9
-5.984645803463384e+01, // 10
 1.932724162273972e+02, // 11
 2.016014594068888e+02, // 12
-7.602920083244154e+02, // 13
-3.911710592236085e+02, // 14
-1.797777647329929e+02, // 15
-4.886341248587692e+02, // 16
 2.346145236913489e+02, // 17
 8.054512180323455e+02, // 18
 9.878788272531996e+01, // 19
 2.976617083089569e+02, // 20
 1.147959751176199e+02, // 21
 1.403757893061886e+01, // 22
 2.722076393783170e+01, // 23
-2.139487357130525e+02, // 24
 1.011293413943580e+03, // 25
 7.276637546573882e+01, // 26
-3.472801632183696e+01, // 27
 1.968022066536098e+02, // 28
-8.783359645181351e+01, // 29
-4.763689976543444e+01, // 30
-2.199875139241119e+00, // 31
 9.628939891065446e+00, // 32
 2.432428777733653e+02, // 33
-6.597383894734662e+02, // 34
 2.257709537175536e+01, // 35
-2.247901292960740e+01, // 36
-1.748532190919984e+02, // 37
 7.511483607230767e+01, // 38
-2.078200833671894e+02, // 39
 7.685529563490256e+01, // 40
 1.017881275775760e+00, // 41
-7.350681602831406e-01, // 42
-1.869940528178316e+01, // 43
-1.507307818653975e+02, // 44
-8.338613974790901e+01, // 45
-7.263120518970372e+00, // 46
 9.136332905233170e+02, // 47
-7.556039321825431e+02, // 48
 5.963532269756106e+01, // 49
-1.034377060972858e+01, // 50
-1.945590306335095e+01, // 51
 1.181565046437024e+02, // 52
 7.884947925544004e+01, // 53
-1.045411331797217e+02, // 54
-1.273309326460350e+02, // 55
 7.861691685841791e-01, // 56
 1.053033323908384e+02, // 57
-2.240836072719207e+02, // 58
-1.956457045702422e+02, // 59
-9.642612203845819e+01, // 60
-1.037778701445948e+01, // 61
 7.267425717297104e+00, // 62
-7.654256198034189e+01, // 63
 2.808183632924786e+00, // 64
-4.837957190017528e+02, // 65
 1.695433992280225e+01, // 66
 1.735293303245865e+00, // 67
-4.338922157888211e+01, // 68
 1.621707467402586e+00, // 69
-4.411721985569323e-01, // 70
 8.131548818857900e-02, // 71
 1.797439491487942e+01, // 72
-8.907663331601800e+01, // 73
 9.369170187189007e+01, // 74
-8.813489245432478e+01, // 75
-1.061469300996965e+01, // 76
 1.521451183930107e+02, // 77
-2.987409893769933e+02, // 78
 2.107913772036339e+02, // 79
-3.949268495252012e+00, // 80
-3.142399678690226e+00, // 81
 2.863472435954939e+01, // 82
-3.723518282468374e-02, // 83
-1.513477475902004e+00, // 84
-1.873029036510124e+00, // 85
 7.473604874259247e+01, // 86
 4.866902536809487e+01, // 87
 1.712395775593357e+02, // 88
 1.267737627072832e+01, // 89
-4.352888658015883e+00, // 90
-1.092424338821057e+01, // 91
 1.983700854734736e+00, // 92
-4.138791553526112e+02, // 93
 5.603778204770716e+01, // 94
 9.675715943922256e+01, // 95
 1.396091714295578e+01, // 96
-8.808370190158391e+01, // 97
 9.601737319680516e-01, // 98
-4.354458932868860e+02, // 99
 2.072674365733874e+01, // 100
 1.365778789404558e+00, // 101
-1.143353719267456e+01, // 102
 1.531435938850802e+00, // 103
-3.775681574335045e+02, // 104
-6.495121878809964e+01, // 105
 2.073468784037226e+01, // 106
-5.972378603433835e+01, // 107
-6.818219795832036e+01, // 108
 6.760450580016310e+01, // 109
 3.251707633772684e-01, // 110
-1.348653418015042e+00, // 111
-2.301998954301339e+01, // 112
-2.726038488905109e+00, // 113
-1.746651889889902e+01, // 114
-8.381424914831847e+01, // 115
 3.736948852497962e-01, // 116
 4.873500466857298e+01, // 117
-4.341947853861146e+01, // 118
-1.833309860967315e+01, // 119
-6.165335864657094e-01, // 120
 7.804862189630535e+00, // 121
 8.527857178054191e-02, // 122
-2.853355859609290e+00, // 123
-9.656305333879341e+01, // 124
 1.186509180883746e-01, // 125
-1.364383277851844e-01, // 126
-1.377334067851904e+02, // 127
 6.547456213311180e+00, // 128
-1.895291031533184e+02, // 129
 1.753842344397056e+02, // 130
 2.199928984450503e+01, // 131
-1.784005338761391e+00, // 132
-4.652047912100946e+02, // 133
 1.430633479894355e+01, // 134
 2.438456681714269e+00, // 135
-3.142553089069667e-01, // 136
 1.102996989526857e+02, // 137
-2.833112445022263e+02, // 138
 7.010110156893987e+01, // 139
 4.199954512245218e-01, // 140
-7.308011923524858e+00, // 141
-3.531278989244809e+01, // 142
-8.818422068412629e+01, // 143
 8.584828694433077e+01, // 144
-2.136135204985244e-01, // 145
 3.644341539938940e-01, // 146
-1.107023640970981e+01, // 147
 3.202655766868452e+02, // 148
-2.552279018102712e+00, // 149
-3.530050871787222e+00, // 150
 3.250206029998530e-02, // 151
 3.967383157509303e-01, // 152
-1.994407229490107e-01, // 153
-5.201446916571689e-02, // 154
 1.075000969116850e-01, // 155
 1.732308308154709e+02, // 156
 2.059510033538488e+02, // 157
 1.141834421828348e-01, // 158
 8.375690839693362e+01, // 159
-3.118020394649576e-02, // 160
 1.954904938426249e+00, // 161
 1.717126336344230e+02, // 162
 3.010090115583989e+02, // 163
-4.473372091961258e+01, // 164
 5.606038068243684e+00, // 165
-8.800591623849033e+01, // 166
 1.668794083839759e-01, // 167
-2.768209869418800e+00, // 168
-3.834318155944404e+02, // 169
 6.165549077470199e+00, // 170
-4.323465802460606e+01, // 171
 4.246603969903940e+02, // 172
 5.831451134103917e+01, // 173
-1.693907443769479e-01, // 174
 9.335053593260674e-01, // 175
-3.802408284294820e-01, // 176
-5.439292687608318e-01, // 177
-4.849861438335725e+01, // 178
 1.699271879091120e-01, // 179
 1.216999655412023e+00, // 180
 2.072658033116701e+02, // 181
 2.346060684866349e-01, // 182
 2.506704906845009e+00, // 183
 1.546063049997708e+02, // 184
-1.418480178091446e+00, // 185
 4.553120712464626e-03, // 186
 1.342137497049742e+02, // 187
-2.064239881279332e-01, // 188
-7.209118756605127e-03, // 189
 9.621452620056742e+00, // 190
-6.379848372616468e+01, // 191
 2.785868746731064e-01, // 192
 4.106510985055601e-02, // 193
-1.854705281345932e+02, // 194
-2.808456003780750e+01, // 195
 5.067000622901705e-03, // 196
 3.165963412429242e+01, // 197
 4.499026168546438e+01, // 198
 1.106257959015740e-01, // 199
-8.433395726947190e+01, // 200
-3.105822680910005e-01, // 201
-3.051802514267071e-01, // 202
 5.901999845632899e-02, // 203
 5.944547254638071e+00, // 204
 5.441059049261467e+00, // 205
-2.160400540795592e+02, // 206
-7.935965681722600e-03, // 207
 5.582965694742883e+01, // 208
-3.184691401940586e+01, // 209
 2.177832710560740e-02, // 210
-9.648665397326096e-03, // 211
 9.782827010332256e-01, // 212
-3.563632463739964e-01, // 213
-2.075692838196640e+00, // 214
 2.696542159198379e-04, // 215
 4.561050599928094e+00, // 216
-4.346362779096489e+01, // 217
 2.348112037947100e-01, // 218
-3.284669871457272e-01, // 219
 5.831789141446134e-01, // 220
 3.848011856241563e-01, // 221
 2.926366568137216e+00, // 222
 4.322736867560500e+00, // 223
-8.110660259174611e-01, // 224
 1.806870002016827e+02, // 225
 1.152956320892057e-02, // 226
 2.827658269363072e-02, // 227
 1.900845780199303e-01, // 228
-6.623697347780103e-02, // 229
 2.807406282154622e+00, // 230
 1.935146087764766e+01, // 231
 9.607566419457472e+00, // 232
-2.226629111285688e-02, // 233
 2.580281390096265e+01, // 234
-1.018013744715790e-01, // 235
-3.711964516350474e-01, // 236
-1.741554720391442e-02, // 237
 1.028299754896591e+01, // 238
 2.406437461121485e+00, // 239
-1.286787845722923e+01, // 240
-1.646822976955974e-01, // 241
 8.941552762308791e-03, // 242
 6.994799185475577e+00, // 243
-2.369266195814206e-03, // 244
-1.360801421901297e-02, // 245
 2.424725131465122e-01, // 246
 3.205721069036435e-01, // 247
-2.105484459064627e+01, // 248
-5.344261608412906e-03, // 249
 4.067528910873541e-03, // 250
-1.152673310315664e-02, // 251
 6.052207077961856e-01, // 252
 7.730883441100590e+00, // 253
 7.286102246042051e+01, // 254
 5.350208382710735e-02, // 255
 1.353697349462219e+00, // 256
 8.169826176472807e+00, // 257
-3.071127297826783e-03, // 258
 2.819175803557217e+01, // 259
 5.746224067047675e+01, // 260
 5.088429858289650e+00, // 261
-2.996273314298618e-01, // 262
 5.128244775877389e-02, // 263
 2.445218879463541e-02, // 264
-2.055433227608843e+01, // 265
 5.507578896602160e-01, // 266
-1.690626773811515e+01, // 267
-1.022464440754965e-01, // 268
 2.375919872680917e-03, // 269
 1.325681005505889e+01, // 270
 5.981428939119415e-03, // 271
-3.493760594495422e+00, // 272
-1.072310281547183e-02, // 273
-2.828825250472447e-02, // 274
 1.167133560616738e-02, // 275
-1.172494314778376e+02, // 276
-1.704322623052994e-03, // 277
-3.379100523560781e+01, // 278
 7.683467937692585e+01, // 279
 2.893269404077742e+00, // 280
 8.510379270160226e+01, // 281
-1.807094441906752e+01, // 282
-3.405687643753511e+00, // 283
-2.957316684659342e-01, // 284
 2.101984158581279e-03, // 285
 2.173847154011593e+00, // 286
 3.142742351454616e-01, // 287
 7.101420765702669e-02, // 288
-1.790586474755296e+00, // 289
-6.097423621144401e-02, // 290
 1.023797228866959e-01, // 291
-1.610066666308936e+00, // 292
-3.616992293705847e-03, // 293
-5.370536580664615e+01, // 294
 4.346480073706008e+00, // 295
 3.574000607525206e+00, // 296
 8.388430721081228e+01, // 297
 6.803543530741865e-01, // 298
 1.717291461857037e-01, // 299
 1.235891664007598e+00, // 300
-8.168157606182805e+01, // 301
-1.654755245387035e-04, // 302
-6.226175580810562e+00, // 303
 2.708221284116472e-01, // 304
-3.731319128325307e+00, // 305
-9.707403241548553e+00, // 306
 3.942962048419984e-02, // 307
 2.750950071587226e+02, // 308
-1.961880024960117e-02, // 309
 2.797694438950586e+00, // 310
 7.677053625360525e-05, // 311
-2.490602947779096e-02, // 312
-8.038288926433392e+00, // 313
-2.479578955452606e-03, // 314
 6.485109623460472e-02, // 315
-1.967989897663437e+00, // 316
-3.629778948089279e-01, // 317
 8.881126487231763e-01, // 318
 1.202412326547582e+00, // 319
-5.892858466939290e-01, // 320
 3.848733102705656e-01, // 321
 7.449553896856018e-01, // 322
-2.152432875114137e-03, // 323
-2.093736059426137e+01, // 324
 1.175091622821439e+00, // 325
-2.923603359174393e-01, // 326
 2.067783923764265e+00, // 327
-3.237896660909327e+01, // 328
 5.009593229529358e-04, // 329
 7.803506103817403e-05, // 330
 1.008636223821969e+00, // 331
-9.938817448949179e+00, // 332
-7.744790918766838e-02, // 333
 2.044040959211102e+01, // 334
-1.142184393785743e-01, // 335
 1.428344900522842e-01, // 336
-3.183736114447971e-02, // 337
-9.453047677304173e-01, // 338
-2.586595858473860e+00, // 339
-2.317580481736019e-02, // 340
-6.302695765587535e-01, // 341
-2.023941437405230e-01, // 342
-9.313201072806313e-01, // 343
 2.646042364033770e-01, // 344
-4.609438712713012e-01, // 345
-2.475947404306181e+00, // 346
-9.534463262729058e-02, // 347
 5.183965030867636e+00, // 348
 5.963187428459596e-03, // 349
 7.148089834718338e-02, // 350
-2.019590340524677e-01, // 351
 2.559995149346920e+00, // 352
 2.191108135258058e+00, // 353
 5.709165043940408e-03, // 354
 5.284279757492509e+01, // 355
 1.865661601504362e+01, // 356
 1.050460676301363e-01, // 357
 1.946339111696552e+01, // 358
 2.494730560687518e-03, // 359
 7.354798695508933e+00, // 360
 7.856061753684589e-02, // 361
-1.271383443726341e+01, // 362
 1.310539590097947e-01, // 363
-2.447471152545285e+00, // 364
 5.529698821539287e-01, // 365
-1.130206624302438e-01, // 366
-3.654357888330041e+01, // 367
 2.776160956659139e+00, // 368
 3.989671708848009e+00, // 369
-8.510403875762943e-03, // 370
 7.321424125443300e+02, // 371
-2.400201418027549e-01, // 372
-5.822211551240140e-02, // 373
-1.807320542208281e+00, // 374
 5.554699578588745e+00, // 375
-1.164552317328342e+01, // 376
 3.987483315777185e+01, // 377
-2.497467991795655e-01, // 378
-1.191151353388779e+00, // 379
 3.436186835818029e-01, // 380
-4.488508068951773e+00, // 381
 6.116515500107889e-01, // 382
 4.392265299936516e-02, // 383
-1.716464811313613e+02, // 384
 3.630124289536317e+01, // 385
 8.788570199314592e-02, // 386
-4.300245568791327e+02, // 387
-1.400751395722789e+00, // 388
 2.050651903136763e-01, // 389
-8.174797695327864e+01, // 390
-3.633423184845060e+01, // 391
-7.970644156994799e+00, // 392
-2.368155432904140e-02, // 393
-4.447630928480837e-02, // 394
 2.370718879278548e-02, // 395
-1.860575636610765e+00, // 396
 2.496179729591361e-01, // 397
-3.171768674166855e-01, // 398
-2.830994790127602e+00, // 399
-1.362958562960494e-01, // 400
 9.522885224651072e-01, // 401
-1.129447390806947e-02, // 402
-2.328250486166383e-01, // 403
 8.239429906620959e-04, // 404
 1.176418694511150e-02, // 405
 2.527207656952961e-01, // 406
-3.424269197346751e+00, // 407
-1.414084256181953e-01, // 408
 2.261613550849727e-03, // 409
-4.768282116967812e-01, // 410
 3.320249981492121e+00, // 411
-6.450892737233001e-04, // 412
-8.172452015929962e+01, // 413
-1.788211764125082e+00, // 414
 1.233071656922576e+00, // 415
-9.750717401839945e-01, // 416
-5.880473899164952e+00, // 417
 2.849120399023876e+02, // 418
-2.431094655185551e+00, // 419
 2.125434475437620e-01, // 420
-2.315486960389526e+01, // 421
 8.306416250069555e-04, // 422
-4.304128035636471e-01, // 423
-1.297166781286814e-01, // 424
-1.230479344751477e-03, // 425
 3.196549690096002e-06, // 426
-5.350507717184359e+00, // 427
-8.290239835635084e+00  // 428
};

//----------------------------------------------------------------------------//

x2b_h2o_ion_v1x_p::x2b_h2o_ion_v1x_p()
{
    m_k_HH_intra =         1.975327500640361e-01; // A^(-1)
    m_k_OH_intra =         2.617953180072867e-01; // A^(-1)
                           
    m_k_XH_coul =          4.677981536517541e-01; // A^(-1)
    m_k_XO_coul =          8.328591854489560e-01; // A^(-1)
                           
    m_k_XLp_main =         7.425459819690536e-01; // A^(-1)
                           
    m_d_HH_intra =         7.395636139500104e-01; // A^(-1)
    m_d_OH_intra =         1.999982215501664e+00; // A^(-1)
                           
    m_d_XH_coul =          5.786908831939904e+00; // A^(-1)
    m_d_XO_coul =          6.999937722802333e+00; // A^(-1)
                           
    m_d_XLp_main =         6.999670438271242e+00; // A^(-1)
                           
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
