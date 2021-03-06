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
-2.243283187216157e+02, // 0
-2.449441044507246e+02, // 1
 1.258440679196070e+02, // 2
 1.794994389783666e+02, // 3
-2.154313261632096e+02, // 4
-1.073387568283169e+02, // 5
-1.758854958094831e+02, // 6
-2.977831152070315e+01, // 7
 2.332012307614150e+02, // 8
-8.660974051107469e+01, // 9
 2.207739262289079e+02, // 10
-3.722042000883695e+01, // 11
-3.497329325828402e+01, // 12
-2.460662423886733e+02, // 13
 1.536722989313948e+02, // 14
-9.723324353640554e+01, // 15
 5.573218091407585e+02, // 16
 1.014781113225840e+02, // 17
 7.057512761847011e+01, // 18
 1.859603547763768e+02, // 19
-4.138850830459371e+01, // 20
 5.380991984125634e+01, // 21
 2.336985868236242e+02, // 22
-8.175616012986003e+00, // 23
 4.167542633971097e+01, // 24
 1.458876641996530e+02, // 25
-1.409689192034505e+02, // 26
 3.334164000691352e+01, // 27
 9.635884035203455e+00, // 28
 1.807247598695684e+02, // 29
-1.439856978048483e+02, // 30
-1.065023753024244e+02, // 31
 3.142728441183513e+00, // 32
-6.054610218196496e+01, // 33
 1.056890290248972e+02, // 34
 1.054572773984106e+01, // 35
-6.056337314769021e+01, // 36
-3.112954337830066e+02, // 37
 8.727496214757564e+01, // 38
 9.640379362905445e+01, // 39
 7.963648241500397e+01, // 40
 1.339549798312496e+02, // 41
 1.210900224749711e+00, // 42
-5.367016463548116e+01, // 43
-1.594282163844225e+02, // 44
 5.590898404415096e+01, // 45
-4.192009788476669e+02, // 46
 2.777368907666193e+02, // 47
 2.274176585181142e+02, // 48
 2.065204010063241e+02, // 49
-9.451257084397834e+01, // 50
 7.089105092939649e+01, // 51
-4.504557763569180e+01, // 52
 1.247752624844365e+02, // 53
-2.150876703733641e+02, // 54
-9.552581841690918e+02, // 55
-6.071200192194154e+00, // 56
-1.314821454126417e+02, // 57
-5.189877574229473e+01, // 58
 2.118901921283224e+02, // 59
-3.306214803070961e+02, // 60
-5.913822233078283e-01, // 61
-7.444036495125836e+00, // 62
-1.934429383234728e+02, // 63
-1.573132073737722e+01, // 64
 3.061352381654751e+01, // 65
 1.362375027478304e+02, // 66
-3.373401126941204e+01, // 67
-2.023335355814295e+02, // 68
 2.244491062784982e+01, // 69
-2.462972387952878e+00, // 70
 9.358699376755384e+00, // 71
-1.011908554529378e+01, // 72
 4.037875955686660e+02, // 73
 5.297916794820143e+00, // 74
 5.222089014070671e+01, // 75
 5.190358602110170e-01, // 76
 2.809657399167399e+02, // 77
-1.148681351406651e+02, // 78
 1.775553140591271e+02, // 79
-9.282923811865534e-01, // 80
-5.857716503407457e+00, // 81
 1.181898073093837e+01, // 82
 1.364780048107697e+00, // 83
 5.477029055194310e+01, // 84
-1.286539627325463e+01, // 85
-4.641259097593242e+01, // 86
-1.806167313939077e+02, // 87
-4.021147540697918e+02, // 88
 1.325108382570935e+02, // 89
-5.145875184398447e+01, // 90
-1.066323946958438e+01, // 91
 6.877318066816435e-01, // 92
-1.663161635518736e+02, // 93
-9.214440795942505e+01, // 94
 5.047169801279631e+00, // 95
-4.635307706790028e+02, // 96
 8.654300396666379e+01, // 97
-2.578282263111156e+02, // 98
-7.918767750443476e+01, // 99
 3.271660414882320e+01, // 100
 3.647569052894947e+01, // 101
-2.675434917948717e+01, // 102
 4.829452962509449e+01, // 103
-2.441515188892211e+02, // 104
-5.615625821646348e+01, // 105
-1.067609216168885e+01, // 106
-7.440381433249202e+00, // 107
-2.064556946975169e+01, // 108
 3.775203088960629e+00, // 109
 3.182077950500635e+01, // 110
-3.094052849579677e+01, // 111
 1.421407089557442e-01, // 112
-3.831843662834452e+00, // 113
 2.252420227564416e+01, // 114
-2.557840064471514e+00, // 115
 1.247732488544452e+02, // 116
 2.552981773675841e+01, // 117
 3.533703419268580e+01, // 118
 3.811952993406915e+00, // 119
-2.013009151728772e+02, // 120
 4.294101204344934e+01, // 121
-1.302090299769055e-01, // 122
 6.962801664649380e+01, // 123
-2.019937062312679e+01, // 124
-3.782828510240586e+00, // 125
-5.777126903171033e+01, // 126
 1.541973518267816e+01, // 127
-2.618968473958332e+00, // 128
-1.751868906103741e+01, // 129
-4.783131104181017e-02, // 130
 2.055611790770940e+02, // 131
 2.246033067628072e+01, // 132
-1.074098547613624e+02, // 133
-3.362318112607684e+01, // 134
 1.074687465042612e+01, // 135
 2.068045826201591e+02, // 136
 3.292132329383799e+01, // 137
 5.342892466226975e+01, // 138
-1.505482990326501e+02, // 139
-8.270304136935914e+00, // 140
-2.216498315058420e+02, // 141
 5.447111160076418e+00, // 142
 3.840561214068481e+02, // 143
 7.877434243337711e+01, // 144
 1.828928023991494e+00, // 145
 3.501679719259771e+01, // 146
-4.562897867371255e+01, // 147
-1.852415912600665e+02, // 148
-7.500599053575839e+00, // 149
 5.510686476544256e+02, // 150
 4.205731517214048e+01, // 151
 1.206885534875296e+02, // 152
-4.542672247025332e-01, // 153
 5.913358138602504e-03, // 154
 2.892173400042033e+01, // 155
-7.976822591680646e+01, // 156
-2.520245182714056e+01, // 157
-5.073419643993958e-02, // 158
 3.091914804624944e+01, // 159
 1.965137992957602e+01, // 160
-9.525451052383117e+00, // 161
 1.963961104782754e+02, // 162
-9.748642117514932e+01, // 163
 1.672997806391834e+02, // 164
 4.099676531216343e+01, // 165
 9.868791560429511e+01, // 166
-1.512416845323235e+00, // 167
 4.006870381791776e+01, // 168
-9.870716891138181e+01, // 169
-7.081041569731862e+00, // 170
-3.172614298762814e+01, // 171
-3.142069218868132e+01, // 172
 6.798839835416747e+01, // 173
-7.576053332604181e+00, // 174
-1.708394111292453e+02, // 175
-4.642698911853424e+01, // 176
-8.131066912296435e+00, // 177
-5.062900982326932e+00, // 178
-3.152512193111028e+02, // 179
-1.131431079829122e-01, // 180
 6.957687634041881e+01, // 181
-5.093524137958042e+01, // 182
 3.346266266250310e+01, // 183
 2.565306149765084e+02, // 184
-4.635128954219314e+01, // 185
 9.560798567609159e-01, // 186
 6.351689027694936e+00, // 187
-3.984400674528895e-02, // 188
 8.878417519448804e-02, // 189
 6.065986888381856e+01, // 190
 1.457002897822201e+01, // 191
 1.407984358716041e-01, // 192
-1.668674162184249e+01, // 193
-2.877001752019700e+01, // 194
 1.818786980040736e+01, // 195
-4.165401716252582e-01, // 196
-4.095907318538644e+01, // 197
-5.166527625434354e+01, // 198
-5.048294043068572e+00, // 199
 1.073328337270920e+02, // 200
-2.573860693700399e-01, // 201
-4.703216243915119e+00, // 202
 6.258513249623692e+01, // 203
 8.612954503558276e+00, // 204
-1.999387725898927e+01, // 205
 1.309198384386858e+00, // 206
-5.564876850886455e+00, // 207
 1.751286916875876e+02, // 208
-3.657377714307831e+01, // 209
-3.536414345637180e-06, // 210
 4.769064960611979e-03, // 211
-1.519263305797363e+01, // 212
 2.669241528310579e-02, // 213
 9.159089634632318e+00, // 214
 3.655054138291530e+00, // 215
-2.313107166422866e+01, // 216
 3.188493151146943e+00, // 217
 7.963912248247948e+01, // 218
-9.660240782710904e-01, // 219
 2.988311505407677e+00, // 220
-4.802115414707573e-02, // 221
-2.490849499410433e-01, // 222
-1.003511562591322e+00, // 223
-7.130470495821052e-01, // 224
 3.208178617909487e+01, // 225
-1.850616313245248e+00, // 226
 4.687211815691458e-02, // 227
-4.618355230004064e+00, // 228
 2.064879052855627e+01, // 229
-5.995852405306291e+00, // 230
 1.555194784826019e-01, // 231
 2.721403126605565e+01, // 232
 6.373044153822304e+00, // 233
-3.676610427808749e-01, // 234
 1.770268246714850e+01, // 235
 2.836704384315712e-02, // 236
-9.177795644136360e+01, // 237
-7.471426743234963e+01, // 238
-1.126989068951806e+02, // 239
 3.155372462991879e+02, // 240
-5.378567435269812e+00, // 241
-6.730285892825429e-01, // 242
-2.251544205007986e+01, // 243
-2.841243671284934e-02, // 244
-2.786326909815838e-02, // 245
-1.257547791949599e+01, // 246
 1.550873107919678e+00, // 247
 5.923945374138906e+00, // 248
 1.061741356624157e+00, // 249
-5.084639475520285e+01, // 250
-2.157085494150632e+01, // 251
-7.234483823276290e+01, // 252
 7.015507312474651e+00, // 253
-5.828447504294529e+00, // 254
 1.429163197676326e+01, // 255
-1.333768402330452e+01, // 256
 4.493871031342154e+01, // 257
 1.749644355886605e-02, // 258
-1.234528434086175e+02, // 259
 1.175181778161733e+00, // 260
 2.885925540247689e-02, // 261
 6.366065564082194e-01, // 262
 2.868244409807195e+01, // 263
-1.510339127278811e+01, // 264
 1.932659647432365e+01, // 265
 8.407426977091621e-01, // 266
-3.804827537105629e+01, // 267
 1.959386946096513e-02, // 268
-2.895909575928602e-05, // 269
-2.729222989578040e+00, // 270
-3.209644812967813e-02, // 271
-7.794801989185599e+01, // 272
 1.397175144423083e+02, // 273
 2.985763636901072e+00, // 274
 6.769792404791443e-02, // 275
-4.344801482878430e+01, // 276
-6.059959478778862e-07, // 277
-6.010404250001844e+00, // 278
 4.866523744686751e+01, // 279
-4.861326270065142e+00, // 280
 7.119970941306169e+00, // 281
-3.121633678805455e+01, // 282
 3.636792590542928e+01, // 283
-6.339778933396189e+00, // 284
-9.666072229415866e-01, // 285
 3.210568254897404e+01, // 286
 3.263630575699698e-01, // 287
 4.118099850734604e-01, // 288
-6.213912823613015e-01, // 289
-9.873186644221898e+00, // 290
 4.014557485598670e+00, // 291
-1.910830796914988e+00, // 292
 1.003852281836773e+00, // 293
-2.477178137641936e+01, // 294
-6.203894072442505e+01, // 295
 1.969578169741021e+00, // 296
 8.734686282316812e+00, // 297
-2.433903461925436e-03, // 298
-2.307563487548258e-02, // 299
-1.645336161484814e+01, // 300
-6.041633459295137e+01, // 301
 2.149931859855130e-01, // 302
-2.426809352787965e+00, // 303
-2.459267998445273e-01, // 304
-1.742771207467851e+01, // 305
-5.467827312564718e+01, // 306
 3.227537967623854e-02, // 307
 7.546239343478308e+01, // 308
-9.175243679967268e-02, // 309
-4.540543117704051e+00, // 310
 3.612964159824675e+00, // 311
-2.433180089522087e+00, // 312
 6.446914794586959e+00, // 313
 6.907459104417481e-03, // 314
 1.769660228038797e+01, // 315
 7.930359157165142e+00, // 316
-7.093475913543475e+00, // 317
 6.540925238395259e+00, // 318
 1.453093561637229e+01, // 319
 6.869683509392728e-01, // 320
 1.484705265226953e+01, // 321
-2.223081405189488e+01, // 322
-3.837753255924312e+00, // 323
-7.709269703310548e+00, // 324
 5.456562901386233e+01, // 325
-2.789863732442745e+01, // 326
-1.426008233114658e+01, // 327
-6.214764926114212e+00, // 328
 6.190804058732636e+00, // 329
 2.701813037318677e-04, // 330
-1.298469143947828e+02, // 331
 1.255626280457061e+01, // 332
-6.350211940652289e+01, // 333
 8.308836291572906e+00, // 334
 4.146275500878590e-03, // 335
-6.193687891578076e+00, // 336
 2.336977011067257e+00, // 337
-1.007954962427166e+00, // 338
 5.355058538672223e+00, // 339
 1.529493333582503e-01, // 340
-9.088798266961071e+00, // 341
-1.410178872345149e+00, // 342
-8.670452780870395e+00, // 343
 2.252852521034835e-01, // 344
-1.242952737084916e+01, // 345
-2.328580842572753e+00, // 346
-2.658144808863315e+01, // 347
 3.893447541361039e+02, // 348
-2.929665556510479e+01, // 349
-1.782792157089505e-01, // 350
 4.456601081088837e+00, // 351
-1.291629631704621e+02, // 352
 4.284516282938253e-02, // 353
 5.086786725334397e-01, // 354
-5.617615308191460e+00, // 355
 2.944376511855229e+01, // 356
-1.449055980290875e-01, // 357
-3.998712754380992e+01, // 358
-6.647379528956627e+00, // 359
 2.475992076566774e-02, // 360
-1.352877897292113e-03, // 361
 3.438100019140155e+00, // 362
-5.039793122476035e+01, // 363
-5.004980720077749e-03, // 364
 4.809600928846707e+01, // 365
 2.885056006976463e-03, // 366
-1.380610454229620e+01, // 367
-4.274023123957010e+00, // 368
-9.172843481665258e-01, // 369
-8.778473467037197e-03, // 370
-9.591495874178188e+00, // 371
-3.533099828245579e-03, // 372
-4.274782299456763e+00, // 373
 4.403411698188135e+01, // 374
-7.886141745933216e+01, // 375
-4.422249320711288e+00, // 376
 1.517739146850757e+01, // 377
 4.731787004958280e+01, // 378
-9.366765052754227e+00, // 379
 4.040803315321606e-01, // 380
 1.039839607054145e+01, // 381
 3.140142013020391e+00, // 382
-6.884029599175324e+00, // 383
-2.229349131015670e+02, // 384
-1.859433941037432e+01, // 385
-3.549014563504543e-04, // 386
-7.840078740229414e+00, // 387
 7.027104932239660e+00, // 388
 1.847229175476320e+01, // 389
 1.179830667646010e+00, // 390
-8.207598296070882e+00, // 391
-1.374288300580197e+00, // 392
 6.867270400352381e-01, // 393
 3.819545415031014e-01, // 394
 4.403681830041091e+00, // 395
 8.767352405506698e-01, // 396
-1.101492833120719e+01, // 397
 2.301750443690731e+01, // 398
 1.573788893008088e+00, // 399
 9.001773408764008e+00, // 400
 5.091141016752800e+00, // 401
-6.838360974986858e-01, // 402
-2.139135485788375e+01, // 403
-1.534723201128804e+01, // 404
-8.062011612272080e+00, // 405
-5.556853885099252e-02, // 406
 2.259581948664775e+00, // 407
-1.126443856024269e-02, // 408
-1.008702875260046e+00, // 409
 5.342679653336167e+01, // 410
 3.548728964834079e+00, // 411
 5.768987463106535e+00, // 412
 1.962601967662659e+01, // 413
 1.559915467997043e+01, // 414
 2.022481296543765e+01, // 415
 4.258257983152095e+00, // 416
 1.679688630171184e+00, // 417
 1.604217368711257e+02, // 418
 2.210608470182716e-01, // 419
-9.199436743872020e+00, // 420
 1.537818490638085e+01, // 421
 5.767070935509118e-04, // 422
 2.686219982066855e+01, // 423
-1.596226914593955e-04, // 424
 3.470924027550510e-07, // 425
-2.318096722648014e+00, // 426
 3.020168349081242e+02, // 427
-8.876936623099642e+00 // 428
};

//----------------------------------------------------------------------------//

x2b_h2o_ion_v1x_p::x2b_h2o_ion_v1x_p()
{
    m_k_HH_intra =         3.195475762417059e-01; // A^(-1)    
    m_k_OH_intra =         6.446164826239851e-01; // A^(-1)
                           
    m_k_XH_coul =          5.104027836124081e-01; // A^(-1)
    m_k_XO_coul =          1.273907587244391e+00; // A^(-1)
                           
    m_k_XLp_main =         7.482531727951891e-01; // A^(-1)
                           
    m_d_HH_intra =         1.431386688762102e+00; // A^(-1)
    m_d_OH_intra =         1.999530480711730e+00; // A^(-1)
                           
    m_d_XH_coul =          4.312600876869783e+00; // A^(-1)
    m_d_XO_coul =          4.872074227770685e+00; // A^(-1)
                           
    m_d_XLp_main =         3.821777646259915e+00; // A^(-1)
                           
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
void mbnrg_2b_h2o_rb_poly(const double* w, const double* x ,
                    double* E, double* g1, double* g2)
#else
void mbnrg_2b_h2o_rb_poly_(const double* w, const double* x ,
                    double* E, double* g1, double* g2)
#endif
{
    *E = the_model(w , x , g1, g2);
}

//----------------------------------------------------------------------------//

#ifdef BGQ
void mbnrg_2b_h2o_rb_cutoff(double* r)
#else
void mbnrg_2b_h2o_rb_cutoff_(double* r)
#endif
{
    *r = the_model.m_r2f;
}

//----------------------------------------------------------------------------//

} // extern "C"

////////////////////////////////////////////////////////////////////////////////
