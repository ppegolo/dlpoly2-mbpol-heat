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
 1.320257448835526e+02, // 0
-3.628061758145657e+02, // 1
 8.376240442636178e+01, // 2
 1.736653242640375e+02, // 3
 5.674098676790977e+01, // 4
-1.406249428327392e+01, // 5
 5.752782978022306e+02, // 6
-2.191385943531075e+01, // 7
-1.537158100134117e+02, // 8
-1.470240651835454e+02, // 9
-1.635709278326090e+02, // 10
 1.500666044933216e+02, // 11
 1.538498850810483e+02, // 12
-3.437553273649442e+02, // 13
-2.480697412339525e+02, // 14
 1.246049174448417e+02, // 15
-4.827840794539125e+02, // 16
 2.219168706797919e+02, // 17
 2.205861976276959e+02, // 18
-8.113629283051594e+01, // 19
 1.919228407496882e+02, // 20
 2.186741734552524e+02, // 21
 2.452489647388732e+00, // 22
 3.596556180416191e+01, // 23
-9.549177273576967e+01, // 24
 8.004964826477293e+02, // 25
 1.187582651080821e+02, // 26
 3.959354400805157e+02, // 27
 2.391498846426312e+02, // 28
-3.376523909277149e+02, // 29
 7.975107646145198e+00, // 30
 2.631646924632971e+01, // 31
 5.125372474331561e+00, // 32
 4.842480808186613e+02, // 33
-5.711367528479975e+02, // 34
-1.261773216905417e+02, // 35
-8.519081507617424e+00, // 36
-4.121046829274271e+02, // 37
-5.519622524064960e+01, // 38
-3.867413824675504e+01, // 39
 3.500700943619233e+01, // 40
-7.091872597836046e+01, // 41
-6.584190656201216e+01, // 42
 7.419807196503234e-01, // 43
-2.209285097132480e+02, // 44
-1.024869712893227e+02, // 45
 2.033836499777318e+01, // 46
 3.707441817995902e+01, // 47
-4.708703184559772e+02, // 48
 2.346465083781241e+01, // 49
-4.530990029442059e+00, // 50
-3.175165554773904e+01, // 51
 4.177900354725102e+01, // 52
 1.486960320051836e+02, // 53
-6.710483650331381e+01, // 54
-2.107146330479472e+02, // 55
 7.737471590109050e+01, // 56
 2.182565564781289e+02, // 57
 2.710429798789387e+01, // 58
-2.310261727277644e+02, // 59
-4.507975181074769e+01, // 60
 4.531966811797606e+01, // 61
 1.361446921042147e-01, // 62
 9.696770547741263e+00, // 63
-1.225363399233612e+01, // 64
 1.902559770276166e+01, // 65
-1.091023030266575e+02, // 66
-6.772928949785521e+01, // 67
-2.124240304516011e+02, // 68
 4.149313798152256e-02, // 69
-1.660557700788023e+02, // 70
 2.330053429929685e-01, // 71
 3.835856676314239e+01, // 72
-8.828296999494691e+01, // 73
 3.970979660521172e+01, // 74
 2.729334518329446e+01, // 75
-3.276264359315122e+00, // 76
-4.939938975500372e+01, // 77
-9.713462835292795e-01, // 78
 5.310768872993382e+02, // 79
 1.589450740843693e+00, // 80
-1.460709695486813e+02, // 81
 2.131539918392347e-01, // 82
 9.674285657640206e-01, // 83
 7.928622716122101e-02, // 84
 3.944571622190483e-04, // 85
-4.595552948296001e+02, // 86
 1.581778411703954e+01, // 87
 5.457012933071347e+02, // 88
 6.634125147338725e+00, // 89
-8.883367664114768e+01, // 90
 1.536624669652580e+02, // 91
-8.794290287201531e+01, // 92
-1.912971041685276e+02, // 93
 7.086847046600104e+01, // 94
-5.641312286915623e+01, // 95
 3.643429419674335e+01, // 96
-1.613533315666290e+02, // 97
-2.558616043483545e+02, // 98
-4.461379975127199e+02, // 99
-2.767841607428375e+01, // 100
-1.527133846917684e-01, // 101
-7.092538720274401e+00, // 102
-3.417409026531350e+01, // 103
-1.411029031007950e+02, // 104
-3.109563220646026e+01, // 105
-1.042175759418272e+01, // 106
 2.932933225460688e+02, // 107
-2.908355503160265e+02, // 108
 8.915629066360731e+00, // 109
-1.698498333075126e+00, // 110
 2.501143123595670e+01, // 111
-4.829547070354830e+01, // 112
-8.881602819185853e-02, // 113
-1.536974972926518e-01, // 114
 1.184121449359411e+01, // 115
-7.292380341264838e+01, // 116
 1.398758533770615e+02, // 117
-1.404258527435186e+01, // 118
-8.123056624456946e+01, // 119
 2.286544595421048e+01, // 120
 2.044549699045493e+02, // 121
-1.741319645835035e+01, // 122
 3.506999138653814e+01, // 123
-1.517448429715801e+02, // 124
-3.481575943783407e+00, // 125
-1.707385774736150e-01, // 126
-1.178652189433161e+02, // 127
 5.507228897653530e+01, // 128
 9.436420698320327e-02, // 129
-5.691072466463404e+01, // 130
 5.261260752051738e+00, // 131
 3.147547922505350e-01, // 132
-1.716271833617677e+01, // 133
 7.851514569328236e+01, // 134
 8.978655110779074e+01, // 135
 4.839376742604221e-01, // 136
-7.523877843968405e-01, // 137
-2.908804195046118e+02, // 138
 4.203537083012902e+01, // 139
-9.233904006858384e-01, // 140
-1.435943159967114e+01, // 141
 3.829116751429612e+01, // 142
 1.141217882999163e+02, // 143
 2.278788746432305e+02, // 144
 1.179953833228197e+01, // 145
-1.956387310859249e+01, // 146
-8.355148696139750e+00, // 147
 4.928297150834439e+01, // 148
 9.991524013636602e+01, // 149
-4.389960187142240e+01, // 150
 6.244933689574556e-01, // 151
-6.867222959664757e+00, // 152
 5.867088285604291e+00, // 153
 9.178668810843629e+00, // 154
 1.528507202384350e+00, // 155
-1.976773816067021e+02, // 156
 1.386315928879392e+01, // 157
-4.400000039392399e+00, // 158
-1.045646744267739e+00, // 159
-1.129292164625668e+01, // 160
 7.411671504776089e-03, // 161
 2.786448317093675e+00, // 162
 2.471857318829915e+01, // 163
 3.104552001372275e+01, // 164
 1.343809921327789e+01, // 165
 5.211092004973469e+01, // 166
 2.491260876625205e+01, // 167
 1.224681382113573e+01, // 168
-3.761900649242698e+01, // 169
-2.346906208520255e+02, // 170
-3.791570734502908e-01, // 171
 3.096164813181415e+01, // 172
 5.922076286446235e+01, // 173
 7.946015393029354e-01, // 174
 1.555049772855526e+00, // 175
-6.662767249847059e+00, // 176
 4.263197230198963e-01, // 177
 1.452532880042382e+02, // 178
 3.590840016912701e-01, // 179
 1.373090523459560e+02, // 180
 2.733326251716334e+02, // 181
 3.841894470549394e+00, // 182
-1.587260085434934e+00, // 183
 5.023836160154810e+02, // 184
 8.337930279534335e-03, // 185
 1.781627438018446e-02, // 186
 2.929198087589422e+02, // 187
 5.419533721926882e-02, // 188
-1.872895106520043e+00, // 189
 7.887116556532852e+01, // 190
 1.774793273982453e+02, // 191
-1.584717658779162e+00, // 192
-2.093212541246356e+00, // 193
-3.664277114884327e+01, // 194
-1.376449157309367e-01, // 195
-3.897539179298466e-01, // 196
-2.833018751789773e-01, // 197
-8.325107637263055e+01, // 198
 2.295713715777489e+00, // 199
-2.102812055644306e+01, // 200
-1.314798291931506e+00, // 201
 2.190921042385906e+00, // 202
-5.174775525578159e+00, // 203
 2.980751383634309e+00, // 204
 2.940899744383035e+01, // 205
-5.284463950165764e+02, // 206
-2.473619945940892e-02, // 207
 3.833250474361411e+02, // 208
-1.817488675379371e+02, // 209
-1.902029956048355e-01, // 210
-1.684578623845605e+01, // 211
 6.440891036462979e-03, // 212
 5.337926482701858e+00, // 213
 1.750251790062434e+01, // 214
 5.174542570654850e-01, // 215
 1.414892068996391e+01, // 216
-3.402181836343114e+01, // 217
 2.048951038332160e-01, // 218
 6.823647698070211e-01, // 219
 2.583309044536990e-01, // 220
-8.132686743258782e+00, // 221
-4.734255491133278e+01, // 222
-1.497485905541904e+01, // 223
-2.442001633516138e+00, // 224
 1.592431101434430e+02, // 225
-6.498470258348236e-02, // 226
 1.274744420049847e-05, // 227
-3.076529464952154e-01, // 228
 3.511209335317229e-01, // 229
 4.677103345335813e+00, // 230
 1.428738730317009e+02, // 231
 3.566790368669120e+00, // 232
 2.722425940408904e-03, // 233
-2.429265425686596e+02, // 234
 4.333845848050217e-02, // 235
 3.548009049559580e+02, // 236
 6.700992528977167e+00, // 237
 7.036507468314844e-01, // 238
-5.627426726011672e+01, // 239
-5.400780965311475e+01, // 240
-1.233644781030034e+00, // 241
-3.335044134051239e-01, // 242
 8.397042869096944e+01, // 243
 4.307730356326078e-04, // 244
 3.014576831221901e-01, // 245
-9.930684920889916e-01, // 246
 6.550958438345725e-02, // 247
-1.994632785525629e-01, // 248
-1.506442462399342e-01, // 249
 4.015698579818400e-02, // 250
-1.595267557045160e+00, // 251
 1.528088730166091e+00, // 252
 2.990718028696365e+00, // 253
 2.643868188695176e+02, // 254
-6.205776684532198e-01, // 255
 4.649585686520499e-02, // 256
 2.871700158243418e+01, // 257
 1.516822103713668e+00, // 258
-7.461590608819846e+01, // 259
-8.901861661826138e+01, // 260
-3.983660658932472e+00, // 261
 6.506845584127217e+01, // 262
 8.869904420834895e-01, // 263
-1.926343730081335e-01, // 264
-1.135595715583325e+00, // 265
 1.859098298201267e-03, // 266
-4.644638196560317e+00, // 267
 4.129373794417393e+01, // 268
 7.808661460989685e+00, // 269
 5.439134531068150e+00, // 270
 1.317872368173599e+01, // 271
-3.336968493988473e+01, // 272
 9.551517160745655e+00, // 273
 1.118945000884928e+01, // 274
 3.669527871101153e+00, // 275
 1.085416233560470e+01, // 276
 1.074004479336670e+00, // 277
-1.778962044707249e+01, // 278
 5.507038694526070e-02, // 279
 8.230164993414089e-03, // 280
 1.593406189207653e+01, // 281
-1.408586879168756e+01, // 282
 9.311609085094258e+00, // 283
-1.160643404299241e+00, // 284
-8.778026436896118e-01, // 285
-7.883666129190048e-01, // 286
-1.948129013300244e+00, // 287
-6.319331376936513e+01, // 288
-4.495729430822749e-03, // 289
-2.957288212491552e-02, // 290
-3.575391694525356e+01, // 291
-1.743960354400933e+02, // 292
-4.425585231397910e+00, // 293
-1.524180931514242e+02, // 294
 2.469952871338988e+01, // 295
-8.229973706074563e+00, // 296
 3.368617675069868e+02, // 297
 5.818028467514001e-01, // 298
-4.196294726057009e+01, // 299
-3.440716820694194e+00, // 300
 2.149175148534076e+01, // 301
 3.044180220892883e-05, // 302
-5.142674741420604e+01, // 303
-6.584439890471013e-01, // 304
-7.928309076675506e+00, // 305
 1.530756868375833e+01, // 306
-4.077428496222747e-04, // 307
 1.138433678886359e+01, // 308
 4.199373319471041e-05, // 309
 1.398146378369118e+01, // 310
 1.220959629629205e+00, // 311
 3.246598695851588e+01, // 312
-8.951126383908247e+01, // 313
 3.838878572106755e+00, // 314
-7.067987139790780e-01, // 315
-1.069505818584037e-02, // 316
 7.645527409518218e+01, // 317
 1.853114814167859e-03, // 318
 5.729354858756203e-02, // 319
-6.413814632285952e+00, // 320
 2.760589305420962e+01, // 321
 6.474641663806525e-03, // 322
 2.118051069160660e-02, // 323
 1.857881142038675e+02, // 324
-8.474129214230622e+00, // 325
-7.527617866611158e-01, // 326
 1.528497815317008e+02, // 327
-1.809694362395586e+02, // 328
 8.799703058627126e-02, // 329
-2.195769105906530e+00, // 330
-4.020298721626546e+00, // 331
-3.167666970800534e+00, // 332
-8.925438836278309e+00, // 333
 7.503827012179943e+00, // 334
 9.220765198668322e-03, // 335
-1.982814147247497e-01, // 336
-2.466091613615723e-03, // 337
 1.791800882595660e+00, // 338
 1.790193595906519e+01, // 339
 3.647760148742591e+01, // 340
-5.935935877184466e-03, // 341
 4.083237169581061e+00, // 342
-4.333303487303606e+01, // 343
 1.346912556932381e+00, // 344
-2.601671617102982e+00, // 345
-1.306771586467025e+02, // 346
-1.349045507046959e-01, // 347
 1.815534293848145e+02, // 348
-4.671338216212535e-03, // 349
-3.206770137984926e+01, // 350
 8.202082743128228e+00, // 351
-1.668725901238163e+01, // 352
 9.881167476456359e+01, // 353
-4.176781710737003e-04, // 354
-1.686839585581744e+02, // 355
 9.428311202564469e+00, // 356
-9.942479432238282e-03, // 357
 1.677860810093556e+01, // 358
 2.899493287327330e-01, // 359
 5.610566699648976e+01, // 360
-2.011270981684378e+01, // 361
-6.123988976586925e-03, // 362
 2.549781403617934e-01, // 363
 2.625097958204826e+01, // 364
-1.996977755057294e+01, // 365
 3.093276299300933e-01, // 366
-3.014513331450792e-01, // 367
-3.741227911218607e+00, // 368
 2.731354126570239e+01, // 369
 5.614794846817529e-02, // 370
 6.243549134345977e+02, // 371
 2.111918361968716e+00, // 372
 5.041451549288695e-03, // 373
-1.141727785052463e+00, // 374
 1.746104638687403e+01, // 375
-5.557234515666194e+00, // 376
 3.512098952447048e-01, // 377
-3.236262634015863e+01, // 378
 6.020910462584565e+01, // 379
 1.037304576376412e+00, // 380
-2.765348053740108e+01, // 381
-6.456477168255240e-04, // 382
 2.119410560712979e-03, // 383
-7.245606888667141e+02, // 384
 1.311019370618749e-01, // 385
-2.391725601016902e-01, // 386
-2.604120727290694e+02, // 387
-4.725737453505759e+00, // 388
-6.916494626111348e-01, // 389
 2.592396515315526e+02, // 390
 1.803761118619000e-01, // 391
-5.464634483326734e+00, // 392
-1.475149375342229e+01, // 393
-3.360167311982072e-03, // 394
-9.003693776544329e-02, // 395
 3.751681570214821e+01, // 396
-2.865215105677783e-01, // 397
-1.289119329230509e+00, // 398
 7.128290685206966e+00, // 399
 2.152537711651827e+00, // 400
 3.813212822038588e+01, // 401
 2.476710460545514e-04, // 402
-3.506407057356326e-01, // 403
 5.359344236329516e-01, // 404
 2.370548997164654e+00, // 405
 2.013529783985901e+00, // 406
-1.144009170261322e-01, // 407
-2.510405392380391e+02, // 408
 2.298063703417561e-02, // 409
-1.221762542993923e+00, // 410
-3.182879332564311e+02, // 411
 7.167089537886641e-02, // 412
-7.229598888286259e+01, // 413
-1.198062584245135e+01, // 414
 3.686177605390171e+00, // 415
-2.241746247040275e-03, // 416
-7.946210370042448e+01, // 417
 1.754935076768910e+01, // 418
 3.185607209356830e+00, // 419
 5.942438668992451e-01, // 420
 9.492779170050663e-01, // 421
-7.908723449720066e+00, // 422
-9.050877757890114e-03, // 423
 1.922422910351769e+01, // 424
-2.974907687820438e+00, // 425
-7.064453881731175e+00, // 426
 1.145132212329107e+00, // 427
 3.153034616815733e-01  // 428
};

//----------------------------------------------------------------------------//

x2b_h2o_ion_v1x_p::x2b_h2o_ion_v1x_p()
{
    m_k_HH_intra =         2.239996679390635e-01; // A^(-1)
    m_k_OH_intra =         2.511419062764456e-01; // A^(-1)
                           
    m_k_XH_coul =          6.308687525749147e-01; // A^(-1)
    m_k_XO_coul =          7.494714869650595e-01; // A^(-1)
                           
    m_k_XLp_main =         1.050726845257100e+00; // A^(-1)
                           
    m_d_HH_intra =         2.046944768703250e-01; // A^(-1)
    m_d_OH_intra =         1.257544612711722e+00; // A^(-1)
                           
    m_d_XH_coul =          6.995812922320198e+00; // A^(-1)
    m_d_XO_coul =          6.999901335477379e+00; // A^(-1)
                           
    m_d_XLp_main =         4.793936418157202e+00; // A^(-1)
                           
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
