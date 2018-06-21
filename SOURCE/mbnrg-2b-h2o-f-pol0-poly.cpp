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
 1.115710584309605e+01, // 0
-1.193272008992895e+03, // 1
 3.969662219068674e+02, // 2
 1.695262101759848e+01, // 3
 1.678990770434885e+01, // 4
-8.248953791934971e+01, // 5
 1.582434728649788e+03, // 6
-7.257434363774589e+00, // 7
 3.795663157426047e+02, // 8
-4.339092258706976e+02, // 9
-1.150611351262662e+02, // 10
 1.016179332588498e+02, // 11
-5.045177211701738e+02, // 12
-1.771324365670002e+01, // 13
-1.319760469436475e+02, // 14
 2.436181410499855e+01, // 15
-6.889194022691826e+02, // 16
 3.134301901091815e+02, // 17
 7.493128109003690e+01, // 18
 7.205144012857504e+02, // 19
 3.117904509575211e+02, // 20
 2.803961231006621e+01, // 21
 1.193877894734111e+00, // 22
 5.036744272982879e+00, // 23
-2.744237789152072e+02, // 24
 4.831329962828161e+01, // 25
 2.984431113174791e+02, // 26
 6.126029499151729e+02, // 27
 4.364414223433500e+02, // 28
-8.543692225908684e+01, // 29
 4.397458754310048e+00, // 30
 1.494958686993133e+00, // 31
 3.600478686773468e-01, // 32
 6.182575495266671e+01, // 33
-1.732647263409661e+03, // 34
 1.148666631296332e+02, // 35
-6.136235168205497e-01, // 36
-5.045494504451824e+01, // 37
-4.150642735006620e+02, // 38
-6.596737654850155e+02, // 39
 5.179211528881950e+01, // 40
-1.024307460167805e+01, // 41
-9.285254554153987e+00, // 42
 7.843114471933353e-02, // 43
-2.062177642637984e+02, // 44
-4.659456975734106e+01, // 45
 2.962186880797584e+00, // 46
 1.739963792196411e+01, // 47
-3.045623430224952e+01, // 48
 7.426390684699329e-01, // 49
-5.782005872688778e+00, // 50
-5.281852599708814e+00, // 51
-2.106307107592931e+02, // 52
 1.196661069728557e+03, // 53
 5.232785557617594e+02, // 54
-5.829777627954418e+02, // 55
 1.260344446070777e+01, // 56
 1.591527441090214e+02, // 57
-3.379700711700823e+02, // 58
-4.462158758179151e+02, // 59
-1.523315745138803e+00, // 60
-1.817750487029594e+02, // 61
 2.068476848776330e-02, // 62
-5.148312382990439e+00, // 63
 6.280354506205541e-01, // 64
-3.529266942416351e+01, // 65
-8.972367224277489e+00, // 66
 5.236631358308194e+00, // 67
 6.147035794694592e+00, // 68
-7.040315421851029e-05, // 69
-1.796239260780934e+01, // 70
 1.383718134249057e-03, // 71
 5.786381847420462e+00, // 72
-3.794503825332864e+02, // 73
 4.365285581238793e+00, // 74
-3.711128454566120e+02, // 75
-1.420662380588864e+00, // 76
 1.018586753279760e+01, // 77
-2.316426662470293e+00, // 78
 8.872937232311911e+01, // 79
 4.437776291315709e-03, // 80
-4.192252408231498e+02, // 81
-7.988453889292563e-03, // 82
 8.662566425676907e-04, // 83
-5.947996575507158e-04, // 84
-1.140505623187442e-05, // 85
-3.630116524326596e+02, // 86
 7.709985877987363e-01, // 87
 5.879281920240333e+02, // 88
 9.650299507264598e-02, // 89
-1.166243153229489e+00, // 90
 1.822167431985037e+01, // 91
-4.956075453641049e+00, // 92
-4.364316007208227e+02, // 93
 4.322368248025969e+01, // 94
 4.207642411283056e+02, // 95
 5.588425074424760e-01, // 96
-8.597750223419240e+01, // 97
 3.978659241593144e+00, // 98
-1.424708039464321e+01, // 99
-9.793006535783614e-01, // 100
 9.311515006632069e-04, // 101
-2.806833971712192e-01, // 102
-4.452549646766939e+00, // 103
-9.813756524283137e+01, // 104
-1.135234822163827e+00, // 105
-1.184227105416931e-01, // 106
 2.153692362946498e+02, // 107
-6.301277624256896e+01, // 108
-4.155871348054909e+00, // 109
-2.125388689208399e-02, // 110
 2.055385106551621e-01, // 111
-5.331505742400785e+00, // 112
 1.993221252108368e-04, // 113
-3.620201212393272e-02, // 114
-4.022708121162232e+02, // 115
-5.622663314984511e-01, // 116
 6.281176683635048e+01, // 117
-1.581439675681376e+00, // 118
-6.090147523374340e+00, // 119
-1.687277020051027e+00, // 120
 4.280816986061289e+01, // 121
-9.450305276157647e-02, // 122
-2.627801811455631e+01, // 123
-3.989741815448474e+01, // 124
-6.653841829774554e-02, // 125
 4.189551672371926e-03, // 126
-6.555405490655600e+02, // 127
 7.245363231824112e+00, // 128
-2.285521219143915e-02, // 129
 3.687288166384286e+02, // 130
-5.154266174341934e+01, // 131
-2.002201663032391e-01, // 132
-1.026941054722420e+01, // 133
 6.142145341081017e+00, // 134
 1.160218194686079e+00, // 135
 3.125545030218749e-03, // 136
-9.293036869900570e-02, // 137
-9.798696001166383e+01, // 138
 4.333349661906652e+02, // 139
-2.376293407082479e-03, // 140
-1.169049110476799e+00, // 141
 3.998967738805113e+02, // 142
-2.114208876610745e+02, // 143
 1.677823620733672e+01, // 144
 1.137181597984663e-01, // 145
-3.314644912665200e+00, // 146
-6.012795348201094e-01, // 147
 3.168291805844169e+02, // 148
 7.598969641040710e+00, // 149
-3.324326216735202e+00, // 150
 2.596439849837381e-02, // 151
 5.856802448776367e+00, // 152
 3.238350931540902e-02, // 153
 3.266448757493128e-02, // 154
 9.574365840637514e-03, // 155
-1.996806793314754e+02, // 156
 4.171309266032503e+00, // 157
-1.768737105803806e-02, // 158
-4.235705175852395e-02, // 159
-8.242469277966732e-02, // 160
-2.228432301364662e-05, // 161
 1.886254061836053e+00, // 162
 2.253538591530946e+01, // 163
 2.326345359502027e+01, // 164
 9.752066567229216e-01, // 165
 3.616122865877596e+02, // 166
 1.412582379348337e-01, // 167
 4.558140103549338e-01, // 168
-7.112742456369661e-01, // 169
-4.089846790812935e+01, // 170
 2.127917679078970e-03, // 171
-1.515577228703986e+01, // 172
 2.023740330109482e+00, // 173
-1.649890243317130e-02, // 174
-1.704219301339478e+01, // 175
-3.309680933226609e-02, // 176
 1.774569482067680e-03, // 177
 9.924992804450102e-01, // 178
 5.638441028503731e-03, // 179
 1.274227558551333e+01, // 180
-6.255365401055470e+02, // 181
 6.591888857902133e-02, // 182
-7.482533678250189e-03, // 183
 4.728485849550323e+02, // 184
 2.935784659175086e-05, // 185
 5.399973516175232e-06, // 186
 1.116193209058208e+02, // 187
-2.626149272393885e-04, // 188
 3.188781196410090e-04, // 189
-1.581772926781157e+01, // 190
 9.377097547701798e+01, // 191
-5.265313499213095e-02, // 192
 3.083916922568073e-03, // 193
-8.290232158882581e+00, // 194
-1.156061178888658e-03, // 195
 4.995677113584070e-04, // 196
 1.192890640106363e-04, // 197
-6.193136714051090e+01, // 198
 5.486339014407987e-03, // 199
-1.853885039505693e+00, // 200
-3.805430409073363e-03, // 201
 2.905120789584853e-02, // 202
 4.227398051446016e-02, // 203
 1.121412747675376e-01, // 204
-9.425133538110280e-01, // 205
-7.298348431126816e+01, // 206
 1.350883037448233e-04, // 207
-1.308349075087242e+00, // 208
-2.229324666043181e+01, // 209
-8.282815634427246e-05, // 210
 3.286167197929310e-03, // 211
-1.276735512085659e-05, // 212
-8.913139095899351e-03, // 213
-3.743452633276481e+00, // 214
-9.212495438171797e-06, // 215
 1.762002288235788e-01, // 216
-9.536887911442812e-01, // 217
 5.686850110747012e-01, // 218
 1.102369714166957e-03, // 219
 4.494573703114610e-04, // 220
 1.615980339094722e-02, // 221
-1.035752789507867e+01, // 222
 3.496354284922245e+00, // 223
 2.585648420129485e-02, // 224
 9.240117538675435e+00, // 225
-2.107895134430371e-05, // 226
-5.649384829538424e-09, // 227
-1.039827560397074e-03, // 228
 1.067538693576774e-03, // 229
 1.695629251900921e-01, // 230
-5.278169955854597e+01, // 231
 3.706842681524774e-01, // 232
-9.001765593252159e-07, // 233
 1.271798100269051e+02, // 234
-1.998946629388486e-04, // 235
 8.306284520150541e+00, // 236
-8.690414749586697e-02, // 237
 9.678184450536004e-02, // 238
 2.839183410141954e+01, // 239
-7.795402799888851e-01, // 240
 1.738930094274763e-02, // 241
-2.120700714456120e-04, // 242
 5.269190454631077e-02, // 243
-1.361558095568755e-06, // 244
 1.578393375999378e-04, // 245
 1.792587982745157e-01, // 246
-1.127314613772218e-04, // 247
 4.027228993255028e-03, // 248
-1.019791008355128e-04, // 249
 8.103885459047254e-06, // 250
-7.171575862450858e-03, // 251
 1.295767516990922e-02, // 252
 8.715333537009382e-02, // 253
 4.318405926668830e+01, // 254
 6.028294890758200e-03, // 255
 1.435042184555412e-02, // 256
 2.023602761305192e+00, // 257
-5.899234704456886e-04, // 258
 1.457623729206747e+02, // 259
-4.128212419456803e+01, // 260
 1.527943207219620e+00, // 261
-4.780514228765284e-01, // 262
 3.652627422166506e-03, // 263
-2.398418700194091e-02, // 264
-1.391245159240435e-01, // 265
 2.049151369462070e-04, // 266
-2.343754986074360e-01, // 267
-5.687200029501202e-02, // 268
-1.988239615366616e-03, // 269
 1.124810920213362e-01, // 270
-2.132297308277390e-03, // 271
 1.661240005303578e+01, // 272
-5.013873405371942e-02, // 273
-1.477493983525050e-02, // 274
-2.167490690788525e-04, // 275
-3.616037118915832e+00, // 276
-8.486974928938241e-07, // 277
-9.169410133636836e-01, // 278
 1.371751331536849e-02, // 279
 6.704729952550755e-06, // 280
-3.045558018766800e+01, // 281
-5.894901963110802e-01, // 282
-9.016984024065036e-01, // 283
-2.041628915601485e-03, // 284
 2.487537864853113e-04, // 285
-1.118862623143910e-02, // 286
-2.071229121273308e-02, // 287
 8.496633151666785e-02, // 288
-1.784664226267480e-06, // 289
 7.330844080567837e-05, // 290
 2.888246313349221e-01, // 291
-4.724995429467906e+00, // 292
 7.462179071499582e-04, // 293
-6.945407921989299e+01, // 294
 8.295142986188961e-01, // 295
-2.659208100773103e-01, // 296
 2.985252983103774e+01, // 297
-1.791632409609556e-02, // 298
 3.136852811546221e-01, // 299
 3.069462872098327e-01, // 300
-3.079837949543951e+00, // 301
 3.477425813116624e-08, // 302
-3.925728787768207e-01, // 303
-2.850255834563680e-04, // 304
 2.098923004103419e+00, // 305
 2.860344133248350e+01, // 306
-2.259141379780579e-07, // 307
 4.359218711690419e+00, // 308
 1.761037328356583e-08, // 309
-1.306343370955502e+01, // 310
-9.656526861659508e-05, // 311
-5.628716030571163e-02, // 312
 4.901568264778067e+01, // 313
-9.694411765336628e-04, // 314
-1.096211393633790e-02, // 315
 5.561994525058697e-06, // 316
 2.714590978142892e+00, // 317
-7.131894416630983e-07, // 318
 8.795732906689619e-04, // 319
 1.175564609446734e+01, // 320
 4.644857132220284e-01, // 321
 2.041648737423591e-04, // 322
-1.288608273703116e-04, // 323
-1.512259572915990e+02, // 324
 7.714244330202196e-01, // 325
-1.125063221625452e-02, // 326
-4.562941317784604e+00, // 327
 2.154031048979504e+01, // 328
 7.049190692747547e-05, // 329
 2.039102739248482e-04, // 330
 2.233169119359461e+00, // 331
-7.166360778464463e-02, // 332
-1.946988084828044e-02, // 333
 3.025184231781914e-01, // 334
 5.629098814743005e-07, // 335
-5.254648670924301e-04, // 336
 1.263835229241843e-06, // 337
-4.951329114055532e+00, // 338
-3.047358897795185e-01, // 339
-3.027409276252231e-01, // 340
-1.457294783842027e-04, // 341
-1.461959722430127e-02, // 342
 2.380276088006527e+00, // 343
 6.151097350964654e-03, // 344
 3.392207687425468e+00, // 345
 6.283810359720357e+00, // 346
-3.933013034328344e-04, // 347
-8.292268103012229e+01, // 348
 7.157858393946652e-06, // 349
 4.519661371886331e-02, // 350
-1.152138519844072e-02, // 351
 2.844583776711475e-01, // 352
-3.296018853094886e+00, // 353
 8.982959088299241e-07, // 354
 5.352344146849390e+02, // 355
 4.961700874083939e-01, // 356
 5.402295885406405e-07, // 357
 4.782949760152809e-01, // 358
-1.926972840891169e-04, // 359
 5.809261946834385e-01, // 360
 3.042289611699304e-02, // 361
 1.933307232864427e-03, // 362
-4.593697400788258e+00, // 363
 5.910771186708430e+00, // 364
-1.465970375102547e-01, // 365
 2.682247508967355e-05, // 366
-1.667263568440595e-02, // 367
-3.230588459691733e-02, // 368
 1.721227210492023e+00, // 369
 2.260262483243192e-05, // 370
 1.076417479314221e+03, // 371
 3.213536863162889e-02, // 372
-4.819058752436142e-07, // 373
-1.264003995686197e-02, // 374
 2.529311164785714e-01, // 375
-5.515732147953849e-01, // 376
 1.863575444517667e-03, // 377
 1.638613121613747e+00, // 378
 5.410616509860519e+00, // 379
 1.174408144113707e-03, // 380
-2.247270648940011e+00, // 381
 1.448474091705456e-05, // 382
 3.043338799575442e-06, // 383
-6.205095594614261e+02, // 384
-2.855858072187877e-04, // 385
-5.164863212781075e-05, // 386
-5.660472986080040e+02, // 387
 8.734480589940344e-01, // 388
-3.908890322011626e-03, // 389
-4.850756468491659e+02, // 390
 3.891781393594193e-02, // 391
-1.867862509391158e-01, // 392
 1.376880786584604e-01, // 393
-4.262099615007692e-06, // 394
 1.784461610305943e-05, // 395
-2.084979201181443e+00, // 396
 1.917571843776111e-04, // 397
 2.012803089401482e-02, // 398
-1.593868404427194e+00, // 399
 8.061466521261881e-02, // 400
-5.255344777758795e+00, // 401
 1.147071381032450e-07, // 402
-2.913542888180548e-03, // 403
-9.698813267352063e-05, // 404
-1.764261437214373e-02, // 405
-7.825303996542653e+00, // 406
-1.197595011490233e-03, // 407
-5.143657419087954e+00, // 408
 2.341974219748562e-05, // 409
 1.098271917035399e+00, // 410
-8.531379182758213e+00, // 411
-3.776132771660581e-05, // 412
 4.082393424415623e+02, // 413
 1.359599428271394e+01, // 414
-6.438961880566584e-02, // 415
-3.656963107569570e-04, // 416
 1.051898239447730e-01, // 417
 1.042826719924058e-01, // 418
 2.423450179197417e-02, // 419
 1.294121226112055e-03, // 420
 2.621295425656095e-02, // 421
 1.920477050891365e-03, // 422
-2.800678424297989e-05, // 423
-1.382017674214584e-01, // 424
 8.614122611630060e-04, // 425
 6.639386825514764e-04, // 426
 1.043370525483802e-02, // 427
 1.862399522998303e-02  // 428
};

//----------------------------------------------------------------------------//

x2b_h2o_ion_v1x_p::x2b_h2o_ion_v1x_p()
{
    m_k_HH_intra =         1.198289240265508e-01; // A^(-1)
    m_k_OH_intra =         2.270606085681964e-01; // A^(-1)
                           
    m_k_XH_coul =          8.653779284320098e-01; // A^(-1)
    m_k_XO_coul =          8.629435215971726e-01; // A^(-1)
                           
    m_k_XLp_main =         1.045757220762339e+00; // A^(-1)
                           
    m_d_HH_intra =         6.497863079017504e-01; // A^(-1)
    m_d_OH_intra =         9.018131053340623e-01; // A^(-1)
                           
    m_d_XH_coul =          6.335566001322430e+00; // A^(-1)
    m_d_XO_coul =          6.871501255485637e+00; // A^(-1)
                           
    m_d_XLp_main =         5.300969484503110e+00; // A^(-1)
                           
    m_in_plane_gamma =     -9.721486914088159e-02;
     m_out_of_plane_gamma= 9.859272078406150e-02;

    m_r2i =  5.000000000000000e+00; // A
    m_r2f =  6.000000000000000e+00; // A

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
void mbnrg_2b_h2o_f_poly(const double* w, const double* x ,
                    double* E, double* g1, double* g2)
#else
void mbnrg_2b_h2o_f_poly_(const double* w, const double* x ,
                    double* E, double* g1, double* g2)
#endif
{
    *E = the_model(w , x , g1, g2);
}

//----------------------------------------------------------------------------//

#ifdef BGQ
void mbnrg_2b_h2o_f_cutoff(double* r)
#else
void mbnrg_2b_h2o_f_cutoff_(double* r)
#endif
{
    *r = the_model.m_r2f;
}

//----------------------------------------------------------------------------//

} // extern "C"

////////////////////////////////////////////////////////////////////////////////