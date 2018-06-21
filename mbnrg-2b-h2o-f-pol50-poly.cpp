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
 1.073521509858902e+02, // 0
-9.516157011739464e+02, // 1
 4.046446854730594e+02, // 2
 2.684056181409260e+00, // 3
 1.132356346524585e+01, // 4
-8.213869755861555e+01, // 5
 1.632730902558642e+03, // 6
-5.023611713236790e+00, // 7
 2.350188949219313e+02, // 8
-6.355202967463597e+02, // 9
 2.543874487169771e+00, // 10
-6.227538336768142e+00, // 11
-4.388158116436284e+02, // 12
 2.154596775967533e+00, // 13
-3.578712610756831e+02, // 14
 2.829562660344074e+02, // 15
-6.802665094929455e+02, // 16
 2.146959245164051e+02, // 17
 2.354255257927450e+01, // 18
 5.047881014126529e+02, // 19
 2.366385816949568e+02, // 20
 3.747262972812530e+01, // 21
 1.229972678904453e+00, // 22
 1.788808136705485e+00, // 23
-6.515125947800989e+01, // 24
 1.452587937495524e+01, // 25
 1.129701001843353e+02, // 26
 6.965535578977369e+02, // 27
-1.678951678799702e+02, // 28
-1.712636432712867e+02, // 29
 4.348337158437380e+00, // 30
-1.583961335477333e+00, // 31
 3.036052309522655e-01, // 32
 1.059359547766253e+02, // 33
-1.869074363940666e+03, // 34
 3.110164857513538e+02, // 35
-3.125524396970618e-01, // 36
-3.361569232009623e+01, // 37
-3.786397572090053e+02, // 38
-7.261240769640262e+02, // 39
 4.573477161217998e+01, // 40
-7.355527159001428e+00, // 41
-1.222130781992410e+00, // 42
 4.001312103327628e-02, // 43
-2.006892303316477e+02, // 44
-2.617017679435509e+01, // 45
 2.548480550905483e+00, // 46
 1.301488428300158e+01, // 47
-2.972398997978161e+01, // 48
 6.903805078547008e-01, // 49
-1.429258582853682e+00, // 50
-3.033494052447547e+00, // 51
 2.943426697364954e+01, // 52
 4.240031440984162e+02, // 53
 9.145573085411191e+02, // 54
 5.940008718039419e+02, // 55
 1.973060517461552e+00, // 56
 2.417004343127676e+02, // 57
-2.476789358604833e+02, // 58
-3.441425769619610e+02, // 59
-8.388596739437143e-01, // 60
-2.595557807881024e+02, // 61
 1.259601927019923e-02, // 62
-3.437504145440748e+00, // 63
-3.475769692892099e-01, // 64
-2.731528631132593e+01, // 65
 1.577582613837924e+00, // 66
 3.945547863087423e+00, // 67
 1.115856016619179e+01, // 68
-5.770251547866797e-05, // 69
-5.527393939765658e+00, // 70
 1.464236155972382e-03, // 71
 4.025958286804990e+00, // 72
-3.250665124658216e+02, // 73
 2.854531755742281e+00, // 74
-1.516760152382110e+02, // 75
-1.724593197319505e+00, // 76
 1.048566276270605e+01, // 77
-1.814192278199455e+00, // 78
 4.285100155361351e+01, // 79
-3.095051786149683e-03, // 80
-1.426402646649524e+02, // 81
-5.393096954458344e-03, // 82
 3.481305059375304e-04, // 83
-7.583685159726943e-04, // 84
-9.307450141524142e-06, // 85
-1.022205325965669e+02, // 86
 5.186593434065205e-01, // 87
 5.583346862294292e+02, // 88
 7.559593267765997e-02, // 89
-6.427958791102486e-01, // 90
 7.797092239152343e+00, // 91
-2.144019501851125e+00, // 92
-1.851111607930101e+02, // 93
 1.548802361794406e+01, // 94
 4.278718965257144e+02, // 95
 3.804246623642180e-01, // 96
-1.654150264066117e+02, // 97
-6.527173547758947e+00, // 98
-6.531529362220196e+00, // 99
-3.051913941169250e-01, // 100
 7.143201433074913e-04, // 101
-2.843043130625754e-01, // 102
-3.324912965712205e+00, // 103
-2.876372126738114e+01, // 104
-9.698869829182454e-01, // 105
-5.617451068108543e-02, // 106
-1.319496077660442e+02, // 107
-5.291387566589661e+01, // 108
-3.976268002065879e+00, // 109
-2.705414255614804e-02, // 110
 7.115700998363991e-02, // 111
-2.295655608921901e+00, // 112
 1.235643842518665e-04, // 113
-2.709083380971820e-02, // 114
-2.097413723876512e+02, // 115
-1.379043251420022e+00, // 116
 5.554017926460835e+01, // 117
-1.833260486521062e+00, // 118
-2.491865220671430e+00, // 119
-2.700951268473636e+00, // 120
 2.772227605096284e+01, // 121
-4.998029025585164e-03, // 122
-1.177238359186254e+01, // 123
 4.253179253015650e+02, // 124
-7.097278302302862e-02, // 125
 1.331379016440658e-02, // 126
-5.461709574906725e+02, // 127
 3.233666605049937e+00, // 128
-2.159736672069288e-02, // 129
 1.248353823619130e+02, // 130
-8.884907368942032e+01, // 131
 9.274480251878299e-01, // 132
-8.968464870170250e+00, // 133
 3.962803336222371e+00, // 134
 4.728413422366079e-01, // 135
-2.416408280496649e-03, // 136
-5.026338712224362e-02, // 137
-5.024670829845785e+01, // 138
 2.796617445442395e+02, // 139
-1.702314446476587e-03, // 140
-1.359392384104582e+00, // 141
 7.882436724480351e+01, // 142
-8.574064830225996e+02, // 143
 1.429797071062223e+01, // 144
 3.874945743954219e-02, // 145
-3.513754006774341e+00, // 146
-5.676880174107425e-01, // 147
 5.355388903263888e+02, // 148
 5.595545086041587e+00, // 149
-2.897545380269393e+00, // 150
 7.946044340270460e-02, // 151
 1.159804402127643e+01, // 152
 2.190475961177480e-02, // 153
-1.156579759625370e-03, // 154
 1.121993623965332e-02, // 155
 6.562485662627845e+02, // 156
 3.478124558826923e+00, // 157
-7.381390078800234e-03, // 158
-1.450911715169336e-02, // 159
 2.652150605848307e-02, // 160
-2.840450865792365e-05, // 161
 4.549747072059862e-01, // 162
 1.902143034562135e+01, // 163
 6.077338041491357e+00, // 164
 5.609311129846049e-01, // 165
-8.815238540809280e+01, // 166
-1.759953240287106e-02, // 167
 2.727981711621977e-01, // 168
-3.116137428347538e-01, // 169
-1.770378733113821e+01, // 170
 2.593950590844230e-03, // 171
 8.990737697624532e+00, // 172
 9.513988041083911e-01, // 173
-1.333317606176436e-01, // 174
-2.017290640413461e+01, // 175
-2.902605996469224e-02, // 176
 2.529920922626399e-03, // 177
 2.701807099663843e+00, // 178
 7.342418754998276e-03, // 179
 2.663342138069830e+00, // 180
-7.572853905023019e+02, // 181
 1.418805681323169e-01, // 182
 5.384791878814134e-03, // 183
 4.253440679582341e+02, // 184
 1.355840495885413e-05, // 185
 1.673574141316983e-05, // 186
 6.165397497055201e+01, // 187
-2.260937791145775e-04, // 188
 1.818605056359076e-03, // 189
-1.339766524667227e+01, // 190
-4.806787571338197e+02, // 191
-2.572782576790360e-02, // 192
 2.223246459828038e-02, // 193
-6.647633323810396e+00, // 194
-1.813214176069091e-03, // 195
 1.259134681998546e-03, // 196
 6.493503329380818e-03, // 197
 1.448477610577902e+02, // 198
 1.805027476226420e-02, // 199
-1.862780544802215e+00, // 200
-2.307411942708710e-03, // 201
 2.414410617917062e-02, // 202
 2.347651173824628e-01, // 203
 1.320075511068968e-01, // 204
-6.009631528465964e-01, // 205
-4.260272382689715e+01, // 206
 2.028764680307838e-04, // 207
 2.699296255386320e+01, // 208
-1.927659996873764e+01, // 209
-4.044159576284854e-05, // 210
 9.298153616562974e-03, // 211
 4.601412585391022e-06, // 212
-9.573727970391972e-03, // 213
-2.703592506446358e+00, // 214
-5.345380602858206e-04, // 215
 1.926045811890184e-01, // 216
-5.825691738809755e-01, // 217
 1.112289587248240e+00, // 218
-1.660105289323692e-04, // 219
 3.298659984260346e-04, // 220
 2.659276932859718e-02, // 221
-2.549930480220040e+00, // 222
 1.587086451743453e+00, // 223
 4.477559419010580e-03, // 224
-8.725423510445871e+01, // 225
-1.897031901289137e-05, // 226
-1.411155555182223e-08, // 227
-5.702841110593586e-05, // 228
 1.817855335117454e-03, // 229
 1.684623848631161e-01, // 230
-2.262218739150707e+01, // 231
 4.281257931765856e-01, // 232
-2.652297569611659e-06, // 233
 6.800911446129713e+01, // 234
-1.258933802690505e-03, // 235
 1.149717287305481e+00, // 236
-3.296147457577480e-01, // 237
 8.642348180304373e-02, // 238
 2.581716632800579e+01, // 239
-1.617587226136812e+00, // 240
 9.953607076277673e-03, // 241
-2.755846889105968e-04, // 242
 4.416953044651143e-01, // 243
-1.236302518058847e-06, // 244
 1.301448978798970e-04, // 245
-1.270094297405168e-01, // 246
-1.165820598964183e-04, // 247
 2.249224693052373e-03, // 248
-1.556051413696064e-04, // 249
-2.697645331180230e-05, // 250
-2.909823556942877e-02, // 251
 1.989009511587592e-02, // 252
 1.859736521086481e-02, // 253
 4.296627761546134e+01, // 254
 1.107078543531937e-02, // 255
 1.109945969327695e-02, // 256
 3.547209953302472e+00, // 257
-1.220498453809194e-03, // 258
 1.418088214559492e+02, // 259
 1.025055049864758e+02, // 260
 6.288784117946477e-01, // 261
-7.354542587531439e-01, // 262
 1.096535269402860e-02, // 263
-3.534036390405661e-02, // 264
-6.006419958937797e-02, // 265
 2.429598134124543e-04, // 266
-2.334249973445452e-01, // 267
-1.130157137663158e-01, // 268
-2.842198407984430e-03, // 269
 3.828339940508538e-02, // 270
-1.196165123484953e-02, // 271
 4.929693125196059e+01, // 272
-2.340283948296732e-01, // 273
-7.195410645711013e-02, // 274
-1.470863865601005e-03, // 275
-3.589438446925354e+00, // 276
-9.292518601033429e-05, // 277
-5.910909815310830e-01, // 278
 1.044439639163862e-02, // 279
 1.368101626714237e-05, // 280
-2.311731058647632e+01, // 281
-6.075439671802899e-01, // 282
-9.795655351097032e-01, // 283
-1.416504656016734e-04, // 284
 2.187786387443799e-03, // 285
-8.550129686644226e-03, // 286
-1.013175249287636e-02, // 287
 3.334682705960463e-01, // 288
-7.541415254830344e-06, // 289
 1.348086686000480e-04, // 290
 7.228406111003178e-01, // 291
-1.436778797662553e+00, // 292
 7.506460865153454e-03, // 293
-6.570778477158412e+01, // 294
 5.370457983651743e-01, // 295
-1.536326127513367e-01, // 296
 1.432937350816820e+01, // 297
-2.414303896326994e-03, // 298
 2.714981529496655e-01, // 299
 2.722699026323875e-01, // 300
-4.238614363830981e+00, // 301
 6.040274468147042e-08, // 302
 6.211682180884201e-02, // 303
 5.060143516351148e-04, // 304
 2.548658816814800e+00, // 305
 3.284888860995820e+01, // 306
-2.549144489450526e-07, // 307
 4.120060644605471e+00, // 308
 3.554203355760238e-08, // 309
-7.463080043027190e+00, // 310
-2.934346338530081e-03, // 311
-3.419545292266487e-01, // 312
 3.671826083889387e+01, // 313
-4.549378922261510e-03, // 314
-2.731993918612322e-02, // 315
 2.752832780249149e-06, // 316
 1.125247032876049e+00, // 317
-3.004737870183470e-06, // 318
 3.994209777875907e-04, // 319
 4.798631709463820e+00, // 320
-4.430876980211089e-01, // 321
 5.549936848045756e-04, // 322
-4.505038971099494e-04, // 323
-1.228823396246125e+02, // 324
 9.124785912100242e-01, // 325
-1.711968801165871e-02, // 326
 3.842146163647922e+00, // 327
 9.676375644296327e+00, // 328
 1.581049420525844e-04, // 329
 5.251505535113031e-04, // 330
 2.456261334788565e+00, // 331
-4.833018369393354e-02, // 332
 2.359748131789079e-02, // 333
 1.619438114209302e-01, // 334
-4.617155403264071e-08, // 335
-4.119623709772645e-04, // 336
 2.486712228526680e-06, // 337
-3.311328476823991e+00, // 338
-1.121117169152358e+00, // 339
-4.034962213322026e-01, // 340
-2.830153497744153e-04, // 341
-2.572372146002648e-02, // 342
 1.303639349927538e+00, // 343
 6.417357635543649e-03, // 344
 3.080352909485670e+00, // 345
 2.674954422422047e-01, // 346
-2.976631522835448e-04, // 347
-9.195459061665845e+01, // 348
-4.464825609814974e-05, // 349
 1.473768864552153e-01, // 350
-4.962405832566636e-02, // 351
 3.213441264509274e-01, // 352
 3.812739199567163e-02, // 353
 1.340244033241428e-06, // 354
 2.943657735152810e+02, // 355
 5.404032120739646e-01, // 356
 1.782649378736073e-06, // 357
 4.829079747138918e-01, // 358
-1.057641391956672e-03, // 359
 9.442343243672932e-02, // 360
 3.200722546893629e-02, // 361
 7.997175313919279e-04, // 362
-4.271506940867230e+00, // 363
 8.185617222462950e-01, // 364
 6.570735267431373e-01, // 365
-3.127625774417718e-04, // 366
-1.260695773574105e-02, // 367
-2.430425275695761e-02, // 368
 1.213424772950902e+00, // 369
 1.371804076289598e-05, // 370
 1.086211751167761e+03, // 371
 9.657952150690454e-03, // 372
-2.431909579919579e-06, // 373
 2.751656880012468e-02, // 374
 7.599833968083919e-01, // 375
-3.778945206806383e-01, // 376
 3.013128468176665e-03, // 377
-1.204606360222336e+00, // 378
 2.515795914873979e+00, // 379
-3.609563195938152e-04, // 380
-2.527599542686457e+00, // 381
 1.851498716358741e-05, // 382
 5.106483091286256e-06, // 383
-6.446737356835794e+02, // 384
 9.394676650647683e-04, // 385
 7.288025318265748e-05, // 386
-4.483655214851477e+02, // 387
 7.644322526226991e-01, // 388
-6.184881259141128e-03, // 389
-1.696531441352741e+02, // 390
 2.219563619962981e-02, // 391
-1.310155157245080e-01, // 392
 2.688338671086778e-01, // 393
-4.199117008820954e-06, // 394
 3.325118775841997e-04, // 395
-6.927233918973229e-01, // 396
 1.359016259196429e-03, // 397
 3.282826207409886e-02, // 398
-1.009280589208148e+00, // 399
 7.083022113931518e-02, // 400
-2.711782759589786e+00, // 401
 2.194238537995574e-07, // 402
-5.549967231793762e-03, // 403
-1.799484044890434e-03, // 404
-4.200606748689330e-02, // 405
-2.055621109150329e+00, // 406
-3.518715763304632e-04, // 407
-5.093398941477054e-01, // 408
 6.982199276278816e-05, // 409
 1.256764700702474e+00, // 410
-6.272106422664970e-01, // 411
-5.314091648097498e-04, // 412
 3.722321809164679e+02, // 413
 5.803836792266684e+00, // 414
-5.312975847864013e-02, // 415
-3.700868271079482e-04, // 416
-1.688824874335700e-01, // 417
-2.030753557292267e-01, // 418
 1.011474980712783e-02, // 419
-1.290126208827141e-03, // 420
 1.463178238945923e-02, // 421
 4.976070536418526e-03, // 422
-4.732132526584964e-05, // 423
-7.225116893253974e-02, // 424
 6.737974755908335e-04, // 425
 9.619678976067131e-03, // 426
 2.334580823968107e-03, // 427
 7.686436219316948e-03  // 428
};

//----------------------------------------------------------------------------//

x2b_h2o_ion_v1x_p::x2b_h2o_ion_v1x_p()
{
    m_k_HH_intra =         1.616400335828359e-01; // A^(-1)
    m_k_OH_intra =         3.090618526198022e-01; // A^(-1)
                           
    m_k_XH_coul =          8.516313154660000e-01; // A^(-1)
    m_k_XO_coul =          8.677252408732146e-01; // A^(-1)
                           
    m_k_XLp_main =         1.001095638344430e+00; // A^(-1)
                           
    m_d_HH_intra =         1.717640275353714e-01; // A^(-1)
    m_d_OH_intra =         4.741238016488328e-01; // A^(-1)
                           
    m_d_XH_coul =          6.326959804886554e+00; // A^(-1)
    m_d_XO_coul =          6.998093445441315e+00; // A^(-1)
                           
    m_d_XLp_main =         5.108064890685443e+00; // A^(-1)
                           
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