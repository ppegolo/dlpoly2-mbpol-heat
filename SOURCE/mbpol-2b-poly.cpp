#include <cmath>
#include <algorithm>

#include "poly-2b-v6x.h"

////////////////////////////////////////////////////////////////////////////////

namespace {

//----------------------------------------------------------------------------//

struct variable {
    double v_exp(const double& r0, const double& k,
                 const double* xcrd, int o1, int o2);

    double v_coul(const double& r0, const double& k,
                  const double* xcrd, int o1, int o2);

    void grads(const double& gg, double* xgrd, int o1, int o2, double* force_matrix_tmp) const;

    double g[3]; // diff(value, p1 - p2)
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - //

double variable::v_exp(const double& r0, const double& k,
                       const double* xcrd, int o1, int o2)
{
    g[0] = xcrd[o1++] - xcrd[o2++];
    g[1] = xcrd[o1++] - xcrd[o2++];
    g[2] = xcrd[o1]   - xcrd[o2];

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
                        const double* xcrd, int o1, int o2)
{
    g[0] = xcrd[o1++] - xcrd[o2++];
    g[1] = xcrd[o1++] - xcrd[o2++];
    g[2] = xcrd[o1]   - xcrd[o2];

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

void variable::grads(const double& gg, double* xgrd, int o1, int o2, double* force_matrix_tmp) const // PP_: added force_matrix
{
  int idx1 = 10*o1 + o2;
  int idx2 = 10*o2 + o1;
  for (int i = 0; i < 3; ++i) {
      const double d = gg*g[i];
      xgrd[o1++] += d;
      xgrd[o2++] -= d;
      // PP_:
      // Write the gradients in the force_matrix
      force_matrix_tmp[idx1+i] =  d;
      force_matrix_tmp[idx2+i] = -d;
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

    void distribute_AX(double* gX, double* grdO) const;

    void distribute_XO(double* gX1, double* gX2,
                      const double& in_plane_g, const double& out_of_plane_g,
                      double* grdO) const;

    void distribute_XH(double* gX1, double* gX2,
                      const double& in_plane_g, const double& out_of_plane_g,
                      double* grdO) const;

    void distribute_XX(double* gX1, double* gX2,
                      const double& in_plane_g, const double& out_of_plane_g,
                      double* grdO) const;
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

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - //

void monomer::distribute_AX(double* gX, double* grdO) const
{

  for (int i = 0; i < 3; ++i) {

      grdO[i] += gX[i];

      gX[i] = 0;

  }
}

void monomer::distribute_XO(double* gX1, double* gX2,
                         const double& in_plane_g, const double& out_of_plane_g,
                         double* grdO) const
{
     const double gm[3] = {
         gX1[0] - gX2[0],
         gX1[1] - gX2[1],
         gX1[2] - gX2[2]
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
         const double gsum = gX1[i] + gX2[i];
         const double in_plane = 0.5*in_plane_g*gsum;

         const double gh1 = in_plane + out_of_plane_g*t1[i];
         const double gh2 = in_plane - out_of_plane_g*t2[i];

         grdO[i + 30*0] += gsum - (gh1 + gh2); // O
         grdO[i + 30*1] += gh1; // H1
         grdO[i + 30*2] += gh2; // H2
     }
 }

void monomer::distribute_XH(double* gX1, double* gX2,
                         const double& in_plane_g, const double& out_of_plane_g,
                         double* grdO) const
{
    const double gm[3] = {
        gX1[0] - gX2[0],
        gX1[1] - gX2[1],
        gX1[2] - gX2[2]
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
        const double gsum = gX1[i] + gX2[i];
        const double in_plane = 0.5*in_plane_g*gsum;

        const double gh1 = in_plane + out_of_plane_g*t1[i];
        const double gh2 = in_plane - out_of_plane_g*t2[i];

        grdO[i + 30*0] += gsum - (gh1 + gh2); // O
        grdO[i + 30*1] += gh1; // H1
        grdO[i + 30*2] += gh2; // H2
    }
}
void monomer::distribute_XX(double* gX1, double* gX2,
                         const double& in_plane_g, const double& out_of_plane_g,
                         double* grdO) const
{

  const double force_difference[3] = {
    gX1[0] - gX2[0],
    gX1[1] - gX2[1],
    gX1[2] - gX2[2]
  };

  const double rOH2_cross_force_diff[3] = {
      oh2[1]*force_difference[2] - oh2[2]*force_difference[1],
      oh2[2]*force_difference[0] - oh2[0]*force_difference[2],
      oh2[0]*force_difference[1] - oh2[1]*force_difference[0]
  };

  const double rOH1_cross_force_diff[3] = {
      oh1[1]*force_difference[2] - oh1[2]*force_difference[1],
      oh1[2]*force_difference[0] - oh1[0]*force_difference[2],
      oh1[0]*force_difference[1] - oh1[1]*force_difference[0]
  };

  // Update the gradients on the physical atoms

  for (int i = 0; i < 3; ++i) {
      const double gsum = gX1[i] + gX2[i];
      const double in_plane = 0.5*in_plane_g*gsum;

      const double gh1 = in_plane + out_of_plane_g*rOH2_cross_force_diff[i];
      const double gh2 = in_plane - out_of_plane_g*rOH1_cross_force_diff[i];

      grdO[i + 30*0] += gsum - (gh1 + gh2);  // O
      grdO[i + 30*1] += gh1; // H1
      grdO[i + 30*2] += gh2; // H2

      gX1[i] = 0;
      gX2[i] = 0;
  }
}

////////////////////////////////////////////////////////////////////////////////

struct x2b_v6x_p {
    x2b_v6x_p();

    typedef x2o::poly_2b_v6x poly_type;

    double operator()(const double* w1, const double* w2,
                      double* g1, double* g2, double* force_matrix) const;

protected:
    double m_k_HH_intra;
    double m_k_OH_intra;

    double m_k_HH_coul;
    double m_k_OH_coul;
    double m_k_OO_coul;

    double m_k_XH_main;
    double m_k_XO_main;
    double m_k_XX_main;

    double m_in_plane_gamma;
    double m_out_of_plane_gamma;

public:
    double m_r2i;
    double m_r2f;

    double f_switch(const double&, double&) const; // O-O separation

protected:
    double m_poly[poly_type::size];
};

////////////////////////////////////////////////////////////////////////////////

double x2b_v6x_p::f_switch(const double& r, double& g) const
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

double x2b_v6x_p::operator()
    (const double* w1, const double* w2, double* g1, double* g2, double* force_matrix) const  // PP_: added force_matrix
{
    // the switch

    const double dOO[3] = {w1[0] - w2[0],
                           w1[1] - w2[1],
                           w1[2] - w2[2]};

    const double rOOsq = dOO[0]*dOO[0] + dOO[1]*dOO[1] + dOO[2]*dOO[2];
    const double rOO = std::sqrt(rOOsq);

    if (rOO > m_r2f)
        return 0.0;

    // offsets

    const int Oa  = 0;
    const int Ha1 = 3;
    const int Ha2 = 6;

    const int Ob  = 9;
    const int Hb1 = 12;
    const int Hb2 = 15;

    const int Xa1 = 18;
    const int Xa2 = 21;

    const int Xb1 = 24;
    const int Xb2 = 27;

    double xcrd[30]; // coordinates including extra-points

    std::copy(w1, w1 + 9, xcrd);
    std::copy(w2, w2 + 9, xcrd + 9);

    // the extra-points

    monomer ma, mb;

    ma.setup(xcrd + Oa,
             m_in_plane_gamma, m_out_of_plane_gamma,
             xcrd + Xa1, xcrd + Xa2);

    mb.setup(xcrd + Ob,
             m_in_plane_gamma, m_out_of_plane_gamma,
             xcrd + Xb1, xcrd + Xb2);

    // variables

    const double d0_intra = 1.0;
    const double d0_inter = 4.0;

    double v[31]; // stored separately (gets passed to poly::eval)

    variable ctxt[31];
//
    v[0] = ctxt[0].v_exp(d0_intra, m_k_HH_intra, xcrd, Ha1, Ha2);
    v[1] = ctxt[1].v_exp(d0_intra, m_k_HH_intra, xcrd, Hb1, Hb2);
    v[2] = ctxt[2].v_exp(d0_intra, m_k_OH_intra, xcrd, Oa, Ha1);
    v[3] = ctxt[3].v_exp(d0_intra, m_k_OH_intra, xcrd, Oa, Ha2);
    v[4] = ctxt[4].v_exp(d0_intra, m_k_OH_intra, xcrd, Ob, Hb1);
    v[5] = ctxt[5].v_exp(d0_intra, m_k_OH_intra, xcrd, Ob, Hb2);
    v[6] = ctxt[6].v_coul(d0_inter, m_k_HH_coul, xcrd, Ha1, Hb1);
    v[7] = ctxt[7].v_coul(d0_inter, m_k_HH_coul, xcrd, Ha1, Hb2);
    v[8] = ctxt[8].v_coul(d0_inter, m_k_HH_coul, xcrd, Ha2, Hb1);
    v[9] = ctxt[9].v_coul(d0_inter, m_k_HH_coul, xcrd, Ha2, Hb2);
    v[10] = ctxt[10].v_coul(d0_inter, m_k_OH_coul, xcrd, Oa, Hb1);
    v[11] = ctxt[11].v_coul(d0_inter, m_k_OH_coul, xcrd, Oa, Hb2);
    v[12] = ctxt[12].v_coul(d0_inter, m_k_OH_coul, xcrd, Ob, Ha1);
    v[13] = ctxt[13].v_coul(d0_inter, m_k_OH_coul, xcrd, Ob, Ha2);
    v[14] = ctxt[14].v_coul(d0_inter, m_k_OO_coul, xcrd, Oa, Ob);
    v[15] = ctxt[15].v_exp(d0_inter, m_k_XH_main, xcrd, Xa1, Hb1);
    v[16] = ctxt[16].v_exp(d0_inter, m_k_XH_main, xcrd, Xa1, Hb2);
    v[17] = ctxt[17].v_exp(d0_inter, m_k_XH_main, xcrd, Xa2, Hb1);
    v[18] = ctxt[18].v_exp(d0_inter, m_k_XH_main, xcrd, Xa2, Hb2);
    v[19] = ctxt[19].v_exp(d0_inter, m_k_XH_main, xcrd, Xb1, Ha1);
    v[20] = ctxt[20].v_exp(d0_inter, m_k_XH_main, xcrd, Xb1, Ha2);
    v[21] = ctxt[21].v_exp(d0_inter, m_k_XH_main, xcrd, Xb2, Ha1);
    v[22] = ctxt[22].v_exp(d0_inter, m_k_XH_main, xcrd, Xb2, Ha2);
    v[23] = ctxt[23].v_exp(d0_inter, m_k_XO_main, xcrd, Oa, Xb1);
    v[24] = ctxt[24].v_exp(d0_inter, m_k_XO_main, xcrd, Oa, Xb2);
    v[25] = ctxt[25].v_exp(d0_inter, m_k_XO_main, xcrd, Ob, Xa1);
    v[26] = ctxt[26].v_exp(d0_inter, m_k_XO_main, xcrd, Ob, Xa2);
    v[27] = ctxt[27].v_exp(d0_inter, m_k_XX_main, xcrd, Xa1, Xb1);
    v[28] = ctxt[28].v_exp(d0_inter, m_k_XX_main, xcrd, Xa1, Xb2);
    v[29] = ctxt[29].v_exp(d0_inter, m_k_XX_main, xcrd, Xa2, Xb1);
    v[30] = ctxt[30].v_exp(d0_inter, m_k_XX_main, xcrd, Xa2, Xb2);
//
    double g[31];
    const double E_poly = x2o::poly_2b_v6x::eval(m_poly, v, g);
//
    double xgrd[30];
    std::fill(xgrd, xgrd + 30, 0.0);
//
    // PP_: Build force matrix
    double force_matrix_tmp[300];

    // PP_: added force_matrix
//
    ctxt[0].grads(g[0], xgrd, Ha1, Ha2, force_matrix_tmp);
    ctxt[1].grads(g[1], xgrd, Hb1, Hb2, force_matrix_tmp);
    ctxt[2].grads(g[2], xgrd, Oa, Ha1, force_matrix_tmp);
    ctxt[3].grads(g[3], xgrd, Oa, Ha2, force_matrix_tmp);
    ctxt[4].grads(g[4], xgrd, Ob, Hb1, force_matrix_tmp);
    ctxt[5].grads(g[5], xgrd, Ob, Hb2, force_matrix_tmp);
    ctxt[6].grads(g[6], xgrd, Ha1, Hb1, force_matrix_tmp);
    ctxt[7].grads(g[7], xgrd, Ha1, Hb2, force_matrix_tmp);
    ctxt[8].grads(g[8], xgrd, Ha2, Hb1, force_matrix_tmp);
    ctxt[9].grads(g[9], xgrd, Ha2, Hb2, force_matrix_tmp);
    ctxt[10].grads(g[10], xgrd, Oa, Hb1, force_matrix_tmp);
    ctxt[11].grads(g[11], xgrd, Oa, Hb2, force_matrix_tmp);
    ctxt[12].grads(g[12], xgrd, Ob, Ha1, force_matrix_tmp);
    ctxt[13].grads(g[13], xgrd, Ob, Ha2, force_matrix_tmp);
    ctxt[14].grads(g[14], xgrd, Oa, Ob, force_matrix_tmp);
    ctxt[15].grads(g[15], xgrd, Xa1, Hb1, force_matrix_tmp);
    ctxt[16].grads(g[16], xgrd, Xa1, Hb2, force_matrix_tmp);
    ctxt[17].grads(g[17], xgrd, Xa2, Hb1, force_matrix_tmp);
    ctxt[18].grads(g[18], xgrd, Xa2, Hb2, force_matrix_tmp);
    ctxt[19].grads(g[19], xgrd, Xb1, Ha1, force_matrix_tmp);
    ctxt[20].grads(g[20], xgrd, Xb1, Ha2, force_matrix_tmp);
    ctxt[21].grads(g[21], xgrd, Xb2, Ha1, force_matrix_tmp);
    ctxt[22].grads(g[22], xgrd, Xb2, Ha2, force_matrix_tmp);
    ctxt[23].grads(g[23], xgrd, Oa, Xb1, force_matrix_tmp);
    ctxt[24].grads(g[24], xgrd, Oa, Xb2, force_matrix_tmp);
    ctxt[25].grads(g[25], xgrd, Ob, Xa1, force_matrix_tmp);
    ctxt[26].grads(g[26], xgrd, Ob, Xa2, force_matrix_tmp);
    ctxt[27].grads(g[27], xgrd, Xa1, Xb1, force_matrix_tmp);
    ctxt[28].grads(g[28], xgrd, Xa1, Xb2, force_matrix_tmp);
    ctxt[29].grads(g[29], xgrd, Xa2, Xb1, force_matrix_tmp);
    ctxt[30].grads(g[30], xgrd, Xa2, Xb2, force_matrix_tmp);

    // distribute gradients w.r.t. the X-points
    //
    ma.grads(xgrd + Xa1, xgrd + Xa2,
             m_in_plane_gamma, m_out_of_plane_gamma,
             xgrd + Oa);
    //
    mb.grads(xgrd + Xb1, xgrd + Xb2,
             m_in_plane_gamma, m_out_of_plane_gamma,
             xgrd + Ob);
    //
    // // distribute force_matrix_tmp to physical atoms

    // Pair indeces
    const int OaHa1 = 3;
    const int OaHa2 = 6;
    const int OaOb = 9;
    const int OaHb1 = 12;
    const int OaHb2 = 15;
    const int OaXa1 = 18;
    const int OaXa2 = 21;
    const int OaXb1 = 24;
    const int OaXb2 = 27;
    const int Ha1Oa = 30;
    const int Ha1Ha2 = 36;
    const int Ha1Ob = 39;
    const int Ha1Hb1 = 42;
    const int Ha1Hb2 = 45;
    const int Ha1Xa1 = 48;
    const int Ha1Xa2 = 51;
    const int Ha1Xb1 = 54;
    const int Ha1Xb2 = 57;
    const int Ha2Oa = 60;
    const int Ha2Ha1 = 63;
    const int Ha2Ob = 69;
    const int Ha2Hb1 = 72;
    const int Ha2Hb2 = 75;
    const int Ha2Xa1 = 78;
    const int Ha2Xa2 = 81;
    const int Ha2Xb1 = 84;
    const int Ha2Xb2 = 87;
    const int ObOa = 90;
    const int ObHa1 = 93;
    const int ObHa2 = 96;
    const int ObHb1 = 102;
    const int ObHb2 = 105;
    const int ObXa1 = 108;
    const int ObXa2 = 111;
    const int ObXb1 = 114;
    const int ObXb2 = 117;
    const int Hb1Oa = 120;
    const int Hb1Ha1 = 123;
    const int Hb1Ha2 = 126;
    const int Hb1Ob = 129;
    const int Hb1Hb2 = 135;
    const int Hb1Xa1 = 138;
    const int Hb1Xa2 = 141;
    const int Hb1Xb1 = 144;
    const int Hb1Xb2 = 147;
    const int Hb2Oa = 150;
    const int Hb2Ha1 = 153;
    const int Hb2Ha2 = 156;
    const int Hb2Ob = 159;
    const int Hb2Hb1 = 162;
    const int Hb2Xa1 = 168;
    const int Hb2Xa2 = 171;
    const int Hb2Xb1 = 174;
    const int Hb2Xb2 = 177;
    const int Xa1Oa = 180;
    const int Xa1Ha1 = 183;
    const int Xa1Ha2 = 186;
    const int Xa1Ob = 189;
    const int Xa1Hb1 = 192;
    const int Xa1Hb2 = 195;
    const int Xa1Xa2 = 201;
    const int Xa1Xb1 = 204;
    const int Xa1Xb2 = 207;
    const int Xa2Oa = 210;
    const int Xa2Ha1 = 213;
    const int Xa2Ob = 219;
    const int Xa2Hb1 = 222;
    const int Xa2Hb2 = 225;
    const int Xa2Xa1 = 228;
    const int Xa2Xb1 = 234;
    const int Xa2Xb2 = 237;
    const int Xb1Oa = 240;
    const int Xb1Ha1 = 243;
    const int Xb1Ha2 = 246;
    const int Xb1Ob = 249;
    const int Xb1Hb1 = 252;
    const int Xb1Hb2 = 255;
    const int Xb1Xa1 = 258;
    const int Xb1Xa2 = 261;
    const int Xb1Xb2 = 267;
    const int Xb2Oa = 270;
    const int Xb2Ha1 = 273;
    const int Xb2Ha2 = 276;
    const int Xb2Ob = 279;
    const int Xb2Hb1 = 282;
    const int Xb2Hb2 = 285;
    const int Xb2Xa1 = 288;
    const int Xb2Xa2 = 291;
    const int Xb2Xb1 = 294;
    //
    // Force between X sites and atoms

    ma.distribute_XO(force_matrix_tmp + Xa1Ob, force_matrix_tmp + Xa2Ob,
                     m_in_plane_gamma, m_out_of_plane_gamma,
                     force_matrix_tmp + OaOb);
    ma.distribute_XH(force_matrix_tmp + Xa1Hb1, force_matrix_tmp + Xa2Hb1,
                           m_in_plane_gamma, m_out_of_plane_gamma,
                           force_matrix_tmp + OaHb1);
    ma.distribute_XH(force_matrix_tmp + Xa1Hb2, force_matrix_tmp + Xa2Hb2,
                           m_in_plane_gamma, m_out_of_plane_gamma,
                           force_matrix_tmp + OaHb2);
    //
    mb.distribute_XO(force_matrix_tmp + Xb1Oa, force_matrix_tmp + Xb2Oa,
                           m_in_plane_gamma, m_out_of_plane_gamma,
                           force_matrix_tmp + ObOa);
    mb.distribute_XH(force_matrix_tmp + Xb1Ha1, force_matrix_tmp + Xb2Ha1,
                           m_in_plane_gamma, m_out_of_plane_gamma,
                           force_matrix_tmp + ObHa1);
    mb.distribute_XH(force_matrix_tmp + Xb1Ha2, force_matrix_tmp + Xb2Ha2,
                           m_in_plane_gamma, m_out_of_plane_gamma,
                           force_matrix_tmp + ObHa2);


    // Force between atoms and X sites
    ma.distribute_AX(force_matrix_tmp + OaXb1, force_matrix_tmp + OaOb);
    ma.distribute_AX(force_matrix_tmp + OaXb2, force_matrix_tmp + OaOb);
    ma.distribute_AX(force_matrix_tmp + Ha1Xb1, force_matrix_tmp + Ha1Ob);
    ma.distribute_AX(force_matrix_tmp + Ha1Xb2, force_matrix_tmp + Ha1Ob);
    ma.distribute_AX(force_matrix_tmp + Ha2Xb1, force_matrix_tmp + Ha2Ob);
    ma.distribute_AX(force_matrix_tmp + Ha2Xb2, force_matrix_tmp + Ha2Ob);
    mb.distribute_AX(force_matrix_tmp + ObXa1, force_matrix_tmp + ObOa);
    mb.distribute_AX(force_matrix_tmp + ObXa2, force_matrix_tmp + ObOa);
    mb.distribute_AX(force_matrix_tmp + Hb1Xa1, force_matrix_tmp + Hb1Oa);
    mb.distribute_AX(force_matrix_tmp + Hb1Xa2, force_matrix_tmp + Hb1Oa);
    mb.distribute_AX(force_matrix_tmp + Hb2Xa1, force_matrix_tmp + Hb2Oa);
    mb.distribute_AX(force_matrix_tmp + Hb2Xa2, force_matrix_tmp + Hb2Oa);

    ma.distribute_XX(force_matrix_tmp + Xa1Xb1, force_matrix_tmp + Xa2Xb1,
                       m_in_plane_gamma, m_out_of_plane_gamma,
                       force_matrix_tmp + OaOb);
    ma.distribute_XX(force_matrix_tmp + Xa1Xb2, force_matrix_tmp + Xa2Xb2,
                           m_in_plane_gamma, m_out_of_plane_gamma,
                           force_matrix_tmp + OaOb);

    mb.distribute_XX(force_matrix_tmp + Xb1Xa1, force_matrix_tmp + Xb2Xa1,
                           m_in_plane_gamma, m_out_of_plane_gamma,
                           force_matrix_tmp + ObOa);
    mb.distribute_XX(force_matrix_tmp + Xb1Xa2, force_matrix_tmp + Xb2Xa2,
                           m_in_plane_gamma, m_out_of_plane_gamma,
                           force_matrix_tmp + ObOa);

    // Construct the actual force matrix: discard X1 and X2 terms

    for (int i = 0; i < 18; ++i) {
      force_matrix[i + 0*18] = force_matrix_tmp[i + 0*30];
      force_matrix[i + 1*18] = force_matrix_tmp[i + 1*30];
      force_matrix[i + 2*18] = force_matrix_tmp[i + 2*30];
      force_matrix[i + 3*18] = force_matrix_tmp[i + 3*30];
      force_matrix[i + 4*18] = force_matrix_tmp[i + 4*30];
      force_matrix[i + 5*18] = force_matrix_tmp[i + 5*30];
    };

    // the switch

    double gsw;
    const double sw = f_switch(rOO, gsw);

    for (int i = 0; i < 9; ++i) {
        g1[i] = sw*xgrd[i];
        g2[i] = sw*xgrd[i + 9];
    };

    for (int i = 0; i < 108; ++i) {
      force_matrix[i] = sw*force_matrix[i];
    };

    // gradient of the switch

    gsw *= E_poly/rOO;
    for (int i = 0; i < 3; ++i) {
        const double d = gsw*dOO[i];
        g1[i] += d;
        g2[i] -= d;

        force_matrix[i + 18*0 + 3*3] += d;
        force_matrix[i + 18*3 + 3*0] -= d;
    };

    return sw*E_poly;
}

//----------------------------------------------------------------------------//

// this is C6-only (v9-cos/01) fit [23-Sep-2013]

static const double the_poly[] = {
 7.832551386996325e+00, // 0
 6.137897864547213e+01, // 1
 1.798766797188997e+02, // 2
-7.839942322381600e+01, // 3
-5.210506199304373e+01, // 4
-3.994663298305017e+00, // 5
 1.092480745585855e+01, // 6
 2.935836455238271e+00, // 7
-7.757952003911277e+00, // 8
-2.425224922333037e+01, // 9
 1.426931011477325e+00, // 10
-1.253067810456554e+01, // 11
 1.499091322216989e-01, // 12
-7.633998030145180e+01, // 13
 1.456133557092916e+00, // 14
-9.385851518014061e+00, // 15
-1.929715094457045e+01, // 16
 4.570242344223167e+00, // 17
 8.074407318781903e+00, // 18
-9.173375867507918e+00, // 19
-5.364306150209694e-01, // 20
-5.425084833370777e+01, // 21
-1.607031993610310e+00, // 22
 1.656527998645836e+00, // 23
 4.169227915091282e+01, // 24
-6.123341439358478e+00, // 25
-3.921761134377481e+00, // 26
-1.094420421820826e+02, // 27
-1.033493741838044e+01, // 28
 1.830880701755060e+00, // 29
-3.828600143523920e+01, // 30
-6.155224494552493e+01, // 31
 8.895822012979245e+00, // 32
 8.097940944553670e+01, // 33
 6.098993523458549e+01, // 34
 2.138677333167251e+00, // 35
 2.688686346060689e+00, // 36
 9.427260123456676e+01, // 37
 2.227583260558067e+01, // 38
 3.920420560561956e+01, // 39
-1.996890382119360e+01, // 40
-4.725085517853018e+00, // 41
 5.663375187766173e+00, // 42
-1.168790562647662e+01, // 43
 6.174493189082303e-01, // 44
 3.203733792477367e+02, // 45
 2.181718186115986e+01, // 46
-7.549929137574643e+00, // 47
 5.939711826368223e+01, // 48
 8.934021948928660e+00, // 49
-8.094707515303755e+00, // 50
 5.534268297094094e+01, // 51
-2.908960782337640e+01, // 52
 7.768398817503095e+00, // 53
-6.741325000374783e+01, // 54
 3.526983873782447e+00, // 55
-5.534097436497698e+00, // 56
-1.230030927607199e+02, // 57
 1.009323400317015e+02, // 58
-7.002482795954832e+01, // 59
-1.394260056493270e+02, // 60
 1.197270700757993e+02, // 61
 1.937310199098437e+02, // 62
 2.111438904689935e+01, // 63
 2.139970371964630e+02, // 64
-1.246056275077503e+00, // 65
 5.363839264357208e+00, // 66
-3.268973918587432e+02, // 67
 4.540659623053999e+01, // 68
-7.460688692567373e+00, // 69
-1.149182513893551e+00, // 70
 5.574122852307052e+01, // 71
-2.170472557690990e-01, // 72
-2.268970978593247e+00, // 73
 8.012732488515441e-02, // 74
 3.014722519025778e-01, // 75
-1.491103798460976e-01, // 76
 3.395090319760058e+00, // 77
-3.145732550810992e-02, // 78
 2.963867988789531e+00, // 79
 1.243032296105406e+01, // 80
-3.378970334526584e+01, // 81
-3.217248567806825e-03, // 82
-3.265415270907243e+00, // 83
-3.007659286258467e+00, // 84
 4.407271878652995e+01, // 85
-6.110855876423297e-01, // 86
-8.087253470874094e+00, // 87
 3.967534740896316e-01, // 88
 1.035479448603993e+02, // 89
 1.760992392514364e+02, // 90
-1.738320642354094e+00, // 91
-7.800889774061514e+01, // 92
 1.384843857376291e-01, // 93
 1.880171942523485e-01, // 94
-2.140753621405904e+01, // 95
-2.409916439074609e+01, // 96
-1.312062452897410e+00, // 97
-5.999125139264914e+01, // 98
 5.675541461430853e-01, // 99
 1.883923040716645e+00, // 100
-3.755052947904272e+00, // 101
-4.234822685211084e+00, // 102
 2.034502827535883e+00, // 103
 1.801742206489307e+01, // 104
-9.962175476485523e+01, // 105
-3.025184772753843e-01, // 106
-1.622000906888283e+02, // 107
 3.897443881956888e+00, // 108
-9.503492187269424e+01, // 109
 2.592787828266840e+00, // 110
 9.403701981982531e+00, // 111
-9.043664034099377e-02, // 112
-4.675289310602908e+00, // 113
-1.715497584216354e-02, // 114
 7.664822043122441e-01, // 115
-2.094636467646331e+01, // 116
-5.004340418566139e+01, // 117
 5.157642459127540e+00, // 118
-3.158186124767128e-02, // 119
-9.763265682959496e-02, // 120
-6.538520707483649e-01, // 121
 1.424218813300146e-01, // 122
 5.838180368659902e+01, // 123
 2.368800194549486e-01, // 124
 2.053837575665226e+01, // 125
 1.164114600753403e+01, // 126
 1.547114866871538e+01, // 127
 1.671266402246336e+00, // 128
-2.038270415529239e+01, // 129
-1.684218336966149e+00, // 130
 6.545313575754865e-01, // 131
 1.095701058189120e-01, // 132
-7.904634761151795e-02, // 133
 1.779687191929903e-01, // 134
 1.265342973685417e+02, // 135
-7.233548714170462e-03, // 136
-3.518646413283942e+01, // 137
-1.201091384570350e+01, // 138
 2.711218428140894e+01, // 139
-1.068357411390933e+02, // 140
 6.113659790332782e+01, // 141
-4.687025912440291e+01, // 142
 3.262360567097801e+01, // 143
 3.117684858414167e+01, // 144
 3.389329405780220e-02, // 145
 9.805276096237103e+00, // 146
 8.003365918160492e+01, // 147
-5.746578584263080e+00, // 148
-3.126468056509798e-02, // 149
 2.358958684259357e+01, // 150
 9.458707020842405e+01, // 151
-1.493782173177485e+00, // 152
-8.298422011547532e-03, // 153
-1.051006352350195e+01, // 154
 9.534314893602978e-03, // 155
 5.827203300711427e+01, // 156
-1.276559327002832e+00, // 157
-2.003353730677165e+01, // 158
-4.437549760965094e-01, // 159
-8.942644358644249e-01, // 160
-1.239298182211595e+00, // 161
-5.172006684017586e+00, // 162
 5.526519904277389e-01, // 163
-5.214826832108125e-01, // 164
-1.251435401209433e+02, // 165
-8.029650666569589e+01, // 166
 3.263654887555291e+00, // 167
-6.107701938979547e+01, // 168
 1.372310352584248e+00, // 169
 4.665241937630666e+01, // 170
-4.097061860274001e-02, // 171
 5.319170548829180e-01, // 172
-2.994023837335916e+01, // 173
-2.535780321447407e-01, // 174
 2.646768871226873e+02, // 175
-1.416971017679291e+00, // 176
-3.733920496699698e+01, // 177
 1.980990886643740e-01, // 178
 4.474154614535784e-01, // 179
-1.406979798759887e+00, // 180
-1.318755247393729e+01, // 181
-2.376184452154926e-01, // 182
 8.161388511363488e+01, // 183
-9.550817088966794e+01, // 184
-2.490357702672333e-03, // 185
 1.080695241130366e+00, // 186
-1.110646327858891e+02, // 187
 1.260795796400829e-02, // 188
 2.427560652002630e+01, // 189
-1.382512861493908e-01, // 190
-8.422357906159049e-01, // 191
-1.270874383267034e-02, // 192
 4.021297160462763e+00, // 193
-1.130116719781862e+00, // 194
-6.130152324702380e-01, // 195
 8.019588362327048e-01, // 196
-1.070651249443909e-01, // 197
-4.403091952869543e-01, // 198
-4.495338548742326e-01, // 199
 2.504144562823849e+01, // 200
-1.087191621424846e+01, // 201
 3.181556833766158e+00, // 202
 8.464291289716770e-01, // 203
 6.305630900837715e+00, // 204
-7.377723898737448e+00, // 205
 1.926621146449472e+00, // 206
 1.374633663715851e+02, // 207
-5.625696091106061e+01, // 208
-1.818968492887796e-01, // 209
 9.549270959391157e+00, // 210
 7.609017871393399e-01, // 211
 1.970911502896740e+00, // 212
 1.170145539926940e+00, // 213
-3.222351818107218e-01, // 214
-9.993255385473774e-04, // 215
 1.107098501169585e+02, // 216
-1.474605026503804e-02, // 217
-1.535511193097425e+00, // 218
-3.698473802798817e-03, // 219
 6.576338652631401e+01, // 220
-2.046873774250415e-02, // 221
-3.704108740652651e+01, // 222
-1.044749414323985e+02, // 223
-1.544717522628573e-01, // 224
 1.637879429003370e-01, // 225
 1.355500348160650e+02, // 226
 1.807963645363681e+01, // 227
 3.155334507041031e+00, // 228
 2.476637111169107e+02, // 229
-1.282688183078145e+00, // 230
 2.097648686038975e-01, // 231
-4.313644096963809e-01, // 232
 6.037611974496232e+01, // 233
 4.876490463320928e-01, // 234
-1.191271012764338e+02, // 235
 1.675341336556648e+02, // 236
-3.705433215659573e-03, // 237
 2.347545603858072e+00, // 238
 1.448664361993616e+00, // 239
 1.458792425984085e+01, // 240
-1.148394264696029e+00, // 241
-3.532976098517852e-01, // 242
 1.075319899786511e+00, // 243
 1.157574614952704e-02, // 244
 6.954131346062532e+00, // 245
-3.605501632348740e-01, // 246
 2.119069927036647e+01, // 247
 1.534816284739505e+00, // 248
 4.810377779917071e+01, // 249
 3.694707954743530e-01, // 250
-1.161283682127054e+00, // 251
 8.836396126069733e+00, // 252
-3.586544382431377e+01, // 253
-8.526265907151432e-01, // 254
-9.846990219173686e-01, // 255
-3.359810235241284e+01, // 256
-1.583153682951723e+00, // 257
-3.492743326832242e-01, // 258
-1.311056246992306e+00, // 259
 7.349106600216308e-03, // 260
-3.061391252590886e+00, // 261
-5.964562335481687e-01, // 262
 8.655401607532236e-02, // 263
 2.063582807398585e+02, // 264
 8.002359710636469e+01, // 265
 7.004078431485783e-03, // 266
-7.965191874523541e+01, // 267
-1.085689227608063e+01, // 268
 2.149107493825396e+01, // 269
-4.584565352113422e-01, // 270
-4.452324714581364e-02, // 271
 6.013671260821728e-01, // 272
 5.433557378586883e+01, // 273
-2.213007284232322e+01, // 274
-3.620766488807581e+00, // 275
-7.371052632358636e-02, // 276
 1.782803112450780e+01, // 277
 1.939921452413612e+02, // 278
-8.295130713956095e+01, // 279
-1.163405243017683e+00, // 280
-5.919248067841039e-04, // 281
 2.166689307422634e+01, // 282
-1.314505852054261e+00, // 283
-2.087304933294059e+00, // 284
-7.870452171055250e+00, // 285
 3.755456769227835e+01, // 286
 5.007973610371047e-01, // 287
-4.249184086181153e-05, // 288
 6.575174042128673e+00, // 289
-2.079373481096493e+00, // 290
 1.175368492712388e+00, // 291
 1.683418723979145e+01, // 292
 7.843202301372544e+01, // 293
-4.702074633479649e+01, // 294
 6.865741310981173e-02, // 295
 5.747596483130288e-01, // 296
 1.551117114352467e+01, // 297
 4.438007788921044e+01, // 298
-5.702499971337380e+00, // 299
 7.920471502591110e-02, // 300
-1.514754567554540e-02, // 301
 1.300346728410930e-02, // 302
 2.477288198732035e-01, // 303
 8.150941218151310e-02, // 304
-1.615861595931967e+00, // 305
-8.860035017893896e+00, // 306
-3.608197810789768e+01, // 307
 2.098696808440961e+00, // 308
-3.129363449779999e+00, // 309
-6.076178839483434e-06, // 310
-1.815861090201502e+01, // 311
 1.619449182609442e+01, // 312
-4.478088706856229e-01, // 313
-5.810331456237689e+00, // 314
 2.722366910202047e-02, // 315
-6.024422588802070e+01, // 316
 5.133671559092459e+00, // 317
-4.945323784575854e+00, // 318
 2.536916469737054e-01, // 319
-7.652997430636554e+00, // 320
-5.413669580916588e+00, // 321
 1.754240230505612e+01, // 322
-7.458705403549970e+01, // 323
 1.003112591707118e+01, // 324
-1.486812220077427e+02, // 325
-5.462302408263682e-02, // 326
 2.722668561621407e-02, // 327
-3.219829726230058e-02, // 328
-3.277286721293480e-01, // 329
 1.474712145444653e+02, // 330
-6.135306265032056e-01, // 331
 6.492496177020427e-01, // 332
-2.210738026327968e+01, // 333
-2.244011121991858e+01, // 334
-5.197989816352838e+00, // 335
 1.557659864940533e+00, // 336
-3.590978563559249e+00, // 337
-1.902593164234708e+02, // 338
-2.988650538160161e-02, // 339
-1.902105788346319e+00, // 340
 2.359234402308117e+00, // 341
 8.159395169106284e+00, // 342
-1.506484395258568e+01, // 343
-2.972480806708754e-01, // 344
 3.369915748372350e-02, // 345
-1.128732827286465e-01, // 346
 1.091550759912192e+02, // 347
 9.122134420446148e+00, // 348
-1.868221601177148e+01, // 349
-9.044129667193274e-01, // 350
-3.106279868819332e+00, // 351
 4.785476747001258e-01, // 352
-2.903672713992594e-05, // 353
 2.327402303546987e+00, // 354
-6.934314223959137e+01, // 355
-1.660352673591393e+00, // 356
-1.501541471803449e+01, // 357
 1.266209813074985e+01, // 358
-1.091932987015305e+02, // 359
-1.841912920847630e+02, // 360
-6.844340651628706e-03, // 361
 6.971877829952396e+00, // 362
-1.100320561143838e+01, // 363
-1.694549258236605e-01, // 364
 3.558886603905885e+00, // 365
-1.934787116124005e-01, // 366
 6.715021602360319e+01, // 367
-1.501646142244661e+00, // 368
 2.752175043750008e+00, // 369
 4.927310229515508e+00, // 370
 2.383923157782827e-04, // 371
 2.890187257042649e+00, // 372
-1.003150821025619e+02, // 373
 1.161321015361442e+00, // 374
 1.169135704336023e+02, // 375
 1.829806417512073e-02, // 376
-7.544509793633342e-01, // 377
 1.900418252093231e+02, // 378
 2.366171891248096e-01, // 379
 1.820007186350477e+01, // 380
-7.246316236075276e-01, // 381
 9.870142234576665e-01, // 382
-4.533075674101353e+00, // 383
-1.746062171280300e+02, // 384
-5.821575373737998e+01, // 385
 3.613008806013197e-02, // 386
 1.371644673001524e+02, // 387
 1.704191572943774e+01, // 388
-1.341497605116022e+02, // 389
-3.566245755267443e-01, // 390
-4.276391174267781e+01, // 391
-3.964283765113721e-01, // 392
 2.506003349697718e+01, // 393
 4.129340112308941e-03, // 394
-1.367895852213591e+02, // 395
 7.581608616469026e+01, // 396
 5.177329948748897e+01, // 397
-1.552521526929499e-01, // 398
-3.819785605829221e+01, // 399
 3.127689190822245e+01, // 400
 2.064197808264725e-03, // 401
-1.480002276782493e-04, // 402
-1.451383735794164e+01, // 403
 1.095467819833560e+00, // 404
-3.133049471407391e-01, // 405
-2.175975898993249e+02, // 406
 7.519761113191808e+01, // 407
 3.024673259952351e-01, // 408
 1.528329914582447e+00, // 409
 2.615220965259130e+01, // 410
 1.258951245050138e+01, // 411
 1.791497687134955e-02, // 412
 2.299747899665654e+01, // 413
-6.255934719428136e+00, // 414
 2.700851171501027e+01, // 415
 6.001392866218295e-03, // 416
 9.221718912345178e+00, // 417
 8.691224830084848e-04, // 418
 1.792210943249589e+02, // 419
 1.903762684197920e-01, // 420
 1.378727784071324e+02, // 421
 1.140211209190933e-02, // 422
-2.044047500909154e+01, // 423
 3.137035167239688e+00, // 424
-8.823788435053672e-01, // 425
-5.770455644662110e-01, // 426
-6.507112347845364e+01, // 427
-1.249523126171929e+00, // 428
 2.061285287886397e+01, // 429
 7.610355215230417e+00, // 430
 2.412906830545707e+00, // 431
 1.465472626735905e+01, // 432
-5.844240500984061e+00, // 433
-7.257032314704964e-01, // 434
 2.205820402304416e+00, // 435
 5.098387302595021e-01, // 436
 4.316997662898007e+00, // 437
 2.123458598537317e-01, // 438
 7.819239584112239e-02, // 439
-6.429414180410307e-01, // 440
-7.512573326130925e+00, // 441
-2.228324646462588e+00, // 442
-2.383824631532038e-02, // 443
-5.860837217074647e+01, // 444
-2.133778309904123e+01, // 445
 3.533230227866158e+00, // 446
-8.169505305701243e+01, // 447
 7.601500058538680e-01, // 448
-1.774273360998872e+02, // 449
-3.088723141627122e-02, // 450
-8.062729932072624e-04, // 451
 9.344949492747184e-02, // 452
-3.739146011422852e+00, // 453
-3.005698851917741e+01, // 454
-2.952069794924677e+01, // 455
-8.191248145701927e+01, // 456
-8.445650833418012e+00, // 457
 5.238865343606549e+01, // 458
 2.146092064475963e+01, // 459
 2.056212600177092e+01, // 460
 7.549102380019736e-02, // 461
 9.440934693508489e-01, // 462
-1.103707958858964e+01, // 463
-2.980584698191593e+01, // 464
-3.707483591250823e+01, // 465
 7.458050816104448e+01, // 466
 2.102258371937965e+00, // 467
 1.784482768633701e-03, // 468
-7.797192946787296e-02, // 469
-1.039313501886298e-02, // 470
-1.924682612189293e+01, // 471
-3.207238813036891e+02, // 472
-1.944961744043141e+02, // 473
-1.640904433596370e+02, // 474
 8.420231006115487e-01, // 475
-9.509532036634404e+01, // 476
 4.903801914912574e+01, // 477
 5.172627648554833e+00, // 478
-1.177644324389016e-02, // 479
 3.417506524700097e-02, // 480
-1.868044688569088e+00, // 481
-8.480411065637863e+00, // 482
-2.679266933119748e-03, // 483
 1.436856567181074e+01, // 484
-3.650646538117204e-01, // 485
-1.009451940315673e+00, // 486
 1.793880000732866e+02, // 487
-5.741263140011338e+01, // 488
 9.883237389885448e+01, // 489
 2.129433350225432e+00, // 490
-3.759685540764868e+01, // 491
 6.966624526758605e+01, // 492
-1.199997649939574e-02, // 493
 4.768666294149792e-04, // 494
-5.809521254788246e+00, // 495
 1.338539466049344e+02, // 496
 3.616812331767312e+00, // 497
 7.950946260903032e+01, // 498
-1.398889044291828e-02, // 499
 1.394228254241360e-01, // 500
 4.125131457651250e-01, // 501
 1.252589452427830e+01, // 502
 2.341754178551955e-03, // 503
 7.545819924726320e+01, // 504
 1.467309979747833e-01, // 505
-1.570056084006402e-02, // 506
 1.506484144440766e+00, // 507
 1.599618509904874e-01, // 508
 1.705614762249340e+02, // 509
-2.105015132678406e+01, // 510
 3.554362199204409e+00, // 511
-1.059692422312236e+01, // 512
 1.362549170274769e-03, // 513
 3.076413323002648e-02, // 514
-3.431357708592919e+00, // 515
-1.470903077724314e+01, // 516
-1.000050867558424e+00, // 517
-3.653763646207106e+01, // 518
-1.908604379952718e+01, // 519
-2.893186536576098e+00, // 520
 4.980897621733800e+00, // 521
 1.926554459439625e+02, // 522
-6.210390964749501e-01, // 523
 1.176807878497552e+02, // 524
 9.045489876916224e-03, // 525
-1.961713708427663e+02, // 526
 9.722745302460704e-01, // 527
-2.347395621732081e+01, // 528
-5.240338365513774e-01, // 529
 1.273477033560875e+01, // 530
 7.718034355758532e-02, // 531
 2.327855096259171e-01, // 532
-2.073938016306372e-02, // 533
 3.103288117741936e+00, // 534
-4.607892449323960e+01, // 535
-1.097887450750012e+00, // 536
 1.141000627342025e+01, // 537
-7.123939398254156e+01, // 538
 1.095266515862879e+02, // 539
 7.861965618499328e-02, // 540
 4.955652464733351e-01, // 541
-3.377428172955801e+00, // 542
 7.929737309501663e+00, // 543
-6.216937073156008e-01, // 544
 2.622202139643352e+00, // 545
-3.797908276800752e+00, // 546
-1.334643029348670e+01, // 547
-1.250468281726644e+02, // 548
 7.492165376856285e+01, // 549
 1.901002454087140e+00, // 550
-3.879546811172830e-03, // 551
-2.617709092261101e-01, // 552
 9.279366641588813e-01, // 553
-1.176582245691152e+01, // 554
-1.485163065350554e+02, // 555
 6.696984729564015e+01, // 556
-5.030963265617255e+01, // 557
 5.813150511509078e+01, // 558
 3.766286784284196e+00, // 559
 2.652334434585588e+01, // 560
 1.056336944454371e+00, // 561
 5.451815384209804e-01, // 562
-2.162024647443116e+00, // 563
 4.954332179661588e+01, // 564
-1.031602530985993e-02, // 565
 2.153677877697190e+01, // 566
-1.849562189649678e+00, // 567
 3.345087157892890e+00, // 568
 4.111515448209190e-01, // 569
-2.022339929151257e-01, // 570
-2.751565624601795e+00, // 571
-1.955554388600738e-02, // 572
 2.704304217335617e-01, // 573
 1.396123519136204e+00, // 574
-9.287771610089207e+01, // 575
 1.026830131217192e-01, // 576
-2.896841174922692e-03, // 577
 6.139575805593801e+01, // 578
-2.687188156164738e+01, // 579
-3.862914135503994e+00, // 580
 1.962658428026759e-01, // 581
-1.752257410182547e+01, // 582
-4.912603886062675e-01, // 583
 2.625039077234784e-02, // 584
-7.239151796615890e+01, // 585
 2.617275866422434e+01, // 586
 1.632722503074159e+01, // 587
-1.033510285195171e-03, // 588
-2.957409181484127e+01, // 589
 5.997599768096196e-01, // 590
 5.861124026841678e-02, // 591
 4.528679102556163e+01, // 592
 8.725169017093988e-02, // 593
 2.825118639324063e+00, // 594
 2.636112061472642e+00, // 595
-7.704782151605362e-02, // 596
 8.961945980695633e+01, // 597
 2.209832439092995e-01, // 598
-5.004567370969963e+01, // 599
-1.357242919338445e-02, // 600
-1.314532536727448e+00, // 601
 2.326149094823661e+00, // 602
-5.653662329402416e-02, // 603
-1.270665827433288e-01, // 604
-2.951940893991542e-01, // 605
-3.106626759702694e-01, // 606
 6.231348317140077e+01, // 607
 5.098798050635470e-01, // 608
 4.186774133579554e+00, // 609
-2.558579158084501e+00, // 610
-4.044646715643711e+01, // 611
 7.566246608434366e+00, // 612
-2.030334333602268e+01, // 613
-1.726559801368108e-01, // 614
 5.867803214455398e-01, // 615
 1.547508541847944e+01, // 616
 2.542556462052958e+02, // 617
-4.032058606402240e-02, // 618
 9.880876741562491e+01, // 619
 7.083726280132113e-02, // 620
-4.379143175209569e+00, // 621
 6.174588621954949e-03, // 622
 5.081273584560468e+01, // 623
-2.484682001064188e-01, // 624
 1.509096077905999e+00, // 625
-4.834273809703314e+00, // 626
-4.634138128848450e-01, // 627
-4.054994139962230e-01, // 628
 1.713852268192123e+01, // 629
 1.966029615406848e+02, // 630
-5.218233026824304e+00, // 631
 4.730477224476375e+00, // 632
-6.017939227852100e+01, // 633
 5.519774763286587e+01, // 634
 3.175266079516037e-01, // 635
 9.663699425557330e-02, // 636
 1.953676124961879e+01, // 637
-7.814404987441870e+01, // 638
-4.745980913823422e-01, // 639
 9.319954799044832e+01, // 640
 1.979741662952239e-01, // 641
 2.708135760051580e-02, // 642
-3.797903336486915e-01, // 643
-7.507588413150917e+00, // 644
-8.132698699971447e+00, // 645
-2.854006519133762e-01, // 646
 3.128715052544238e+00, // 647
-5.868822096434657e-01, // 648
-1.113815120257408e+00, // 649
 8.886202561621630e+00, // 650
 7.449131772502188e-02, // 651
-2.158079884828330e+00, // 652
-3.330020607573140e-01, // 653
-1.682688766131595e-01, // 654
-8.813178442051868e+01, // 655
 5.142216818504075e+00, // 656
-1.212085594601385e+01, // 657
-6.138407641350999e-02, // 658
 4.217957328282667e-02, // 659
-1.433990032269788e+00, // 660
 2.053799687993601e-01, // 661
 1.263194962757974e-01, // 662
 1.187132896803754e+02, // 663
-8.213286917436671e-02, // 664
-1.352551927273397e+00, // 665
 3.276530482934393e-01, // 666
 1.560105804711903e+00, // 667
 1.371591408230951e+01, // 668
-9.775097499246027e+01, // 669
 9.311685776293388e+00, // 670
 1.417209170545397e-01, // 671
 2.339527184519664e-01, // 672
-6.625436673958827e-01, // 673
-2.419002602232075e-01, // 674
-4.814268476488460e-01, // 675
 4.542729165277182e-02, // 676
-9.745336761276938e-03, // 677
 6.399441509607958e+01, // 678
-4.618590064000817e+01, // 679
 1.281259126623325e+02, // 680
-1.865330637966804e-02, // 681
 2.486463746944619e-01, // 682
 1.055611700446093e-01, // 683
 1.723802568153281e+01, // 684
-4.868773293434438e-01, // 685
-7.085110186071200e+00, // 686
-2.100151000805737e-03, // 687
 3.897278777853994e-02, // 688
 4.744891923658233e+00, // 689
-1.732447413117931e+00, // 690
 8.274155358094081e-01, // 691
 3.885574378433923e-01, // 692
 1.201289924851435e+01, // 693
-3.089184348381895e+01, // 694
-3.437711770344480e-02, // 695
-9.489492458723749e-01, // 696
-4.028481970145126e-01, // 697
-1.513076018555997e+01, // 698
-7.447028480243587e+01, // 699
 1.922123945277565e-01, // 700
 1.067826056589318e+02, // 701
-1.589042917191324e+00, // 702
 4.731860995868531e-01, // 703
-1.485173569729498e-01, // 704
-2.574394563273000e+01, // 705
-2.631847131723959e-02, // 706
 7.261315528426401e+00, // 707
-2.217726104708288e-01, // 708
-1.060722919260499e+01, // 709
-1.931943571454267e+00, // 710
 4.707449603530686e-02, // 711
-4.982851031207991e+01, // 712
-1.295330761457953e+00, // 713
-1.618260147087590e-01, // 714
-1.257894784044551e-01, // 715
-1.980890904070063e-01, // 716
-2.063210343106004e+00, // 717
-6.524186613018631e+01, // 718
-6.084612966019647e-02, // 719
 2.050660311608447e+00, // 720
 1.326536868318449e-01, // 721
 2.073109114736424e+00, // 722
-2.029016692935870e-01, // 723
 5.386975167852760e+00, // 724
 1.481507688564372e+00, // 725
-1.488046281230049e-02, // 726
 9.044628591018626e-01, // 727
 3.361636615346583e+00, // 728
 9.728043499321098e-01, // 729
-5.058595182777106e+00, // 730
 7.943358367948257e+01, // 731
-2.224203785038031e+00, // 732
-6.881862122959974e+00, // 733
 3.078891020526529e-01, // 734
 1.742364327284812e-01, // 735
 4.102198981968619e-01, // 736
 8.121104440962240e+00, // 737
 9.028487817964828e-01, // 738
-1.297413990275464e-01, // 739
-7.264998032414850e-01, // 740
-3.332610991330177e-01, // 741
-6.692255738619384e+01, // 742
 2.268197852251623e+00, // 743
 1.675002608612670e-01, // 744
 3.158580867030208e+01, // 745
 3.637179086926910e-01, // 746
 2.391800796943564e+00, // 747
-8.830846237270419e-01, // 748
 6.784929608778669e+00, // 749
-2.404519461786299e-02, // 750
 1.746358790613158e+01, // 751
 1.582533443784660e+00, // 752
 1.694808277861700e-02, // 753
 6.894033784615630e+01, // 754
-2.726992485436673e-01, // 755
-1.885883131319025e+00, // 756
 2.631173239537578e+01, // 757
 2.861430976189002e-02, // 758
-2.494434912216298e+01, // 759
 5.553505709930447e-02, // 760
-5.487576759761652e+01, // 761
 9.977833802651227e-01, // 762
 5.987847128085409e+00, // 763
-8.882205984999857e-03, // 764
 7.147126750363321e+01, // 765
 3.017192468512619e+00, // 766
-3.369780251481744e-02, // 767
 2.830021426639219e+01, // 768
 7.463122168684349e-01, // 769
-2.784024076473691e+00, // 770
-4.468346227784163e+00, // 771
 2.911528357430358e-02, // 772
-6.679682791452071e-02, // 773
-5.142379672832242e-01, // 774
 1.643961816744670e-01, // 775
 1.978033070389948e+01, // 776
-4.159011361431059e-03, // 777
-1.778175835677019e-01, // 778
-2.006355684759656e-01, // 779
-6.100543665449387e-02, // 780
 1.046868839377014e+01, // 781
 6.986019489249315e-01, // 782
-4.526680641168419e-01, // 783
-2.570792570047121e+01, // 784
-1.279213247132171e+01, // 785
 9.551504911994185e-01, // 786
 1.084177937090279e+00, // 787
 2.184686362598251e+01, // 788
 1.720852635553380e-01, // 789
-6.230123032114573e-01, // 790
 3.105689086923322e-01, // 791
 6.478975502207113e+01, // 792
-1.959292835069026e-01, // 793
 5.697781567307452e-02, // 794
-2.283155401230113e+00, // 795
-5.966383085009873e-04, // 796
-4.166612770003092e-01, // 797
-2.633987075933053e+01, // 798
-9.044286986868311e+00, // 799
 4.872852869194837e-02, // 800
-2.226184346205662e+00, // 801
 3.820553144946112e+01, // 802
-2.315046800981137e-02, // 803
-2.567193198934303e-01, // 804
 1.033839356375295e+00, // 805
-1.281969164476767e-01, // 806
 2.119139010009940e-03, // 807
 4.805069639436216e+00, // 808
-1.699676321584515e-01, // 809
 3.061714847321495e-01, // 810
 5.315210018004210e+01, // 811
-4.724318500158459e+00, // 812
 1.268983533605457e+00, // 813
-1.563486375665205e+00, // 814
-2.241144668515808e-01, // 815
 3.611190049877776e+00, // 816
-5.317774609180988e+01, // 817
-5.790046472110994e+00, // 818
-2.297005169558322e-01, // 819
-3.446343270225004e-01, // 820
 2.637012199534798e+01, // 821
-1.258315233866819e-02, // 822
 1.170483385075014e+02, // 823
 5.539239316659986e-02, // 824
-5.952350602050602e-01, // 825
-1.298550116540120e-01, // 826
 1.320628571461778e+00, // 827
-7.348525760469263e+01, // 828
 3.221445754163654e+00, // 829
-1.901217444091281e-01, // 830
-2.943473188567516e+00, // 831
 5.267417477554875e+00, // 832
 7.716828831243706e+01, // 833
-3.971548152512190e-02, // 834
-1.090451748330038e+02, // 835
 4.901743049571733e-01, // 836
 1.887796264145865e+01, // 837
-3.139832969201663e-02, // 838
-9.985679250821539e-02, // 839
 5.193585751036421e+01, // 840
-3.466300530929085e+00, // 841
-1.751314025089186e+00, // 842
-4.481889814832560e+00, // 843
-2.828420474615911e-01, // 844
-7.503502546881222e-02, // 845
-1.281951265676867e+01, // 846
 1.616818213072524e+01, // 847
-3.231860978812265e+01, // 848
-1.266809326360554e+00, // 849
-3.017310136828220e-01, // 850
-6.549393992202897e-01, // 851
-3.507053364845612e-01, // 852
-2.732544466404723e-02, // 853
 4.246788461053380e+00, // 854
-2.822310143936404e+00, // 855
-4.122733608789610e-01, // 856
-1.748173907856204e+01, // 857
-1.484981404708002e+01, // 858
 1.975083502702721e+00, // 859
-2.190366277432296e+01, // 860
-1.055923744371541e+01, // 861
 2.883822628158587e-01, // 862
 8.403625946492944e+00, // 863
 2.786214419554230e-02, // 864
-1.326867093949969e-01, // 865
-4.142926955349886e+00, // 866
 3.178259515623406e+01, // 867
-2.425239721418772e-02, // 868
-4.354022394588731e+00, // 869
-7.832837989978353e-02, // 870
 8.698613605341541e-01, // 871
 5.129365682325104e+00, // 872
 2.759371233114460e+01, // 873
-1.694011495433750e+00, // 874
 1.051535265964056e-05, // 875
-6.462985952066164e-01, // 876
-8.720623039461220e+00, // 877
-6.711203872517765e+01, // 878
 3.768219072957492e+00, // 879
-1.549726421448374e-01, // 880
-1.474943103835276e+01, // 881
-1.581578700778702e+01, // 882
 1.911459222499589e-01, // 883
 2.981381709204389e-01, // 884
-1.253789896445243e+01, // 885
 5.157655021391475e-02, // 886
 2.692944723853942e-01, // 887
-8.783612099698878e+00, // 888
-5.234108838373220e+00, // 889
 5.361897832739785e-01, // 890
-3.374945176021414e+01, // 891
 1.525540552041188e+00, // 892
 7.863980055291427e-02, // 893
 1.532932445932340e+01, // 894
 3.630409396183403e-01, // 895
 1.119892623977742e-01, // 896
 6.727082065040716e+01, // 897
 2.135225055339777e+00, // 898
-1.594667442111334e-01, // 899
-1.406230060575998e+02, // 900
 3.679532387355046e+01, // 901
 6.506605742913758e+00, // 902
-1.886021036295957e-01, // 903
 4.680182164724567e-01, // 904
-2.069154364911831e-01, // 905
 8.339197807621957e+01, // 906
-6.419447203497027e-01, // 907
 1.171281309378642e+00, // 908
 9.841650636692414e-02, // 909
-4.165365253300824e+01, // 910
 1.730896210974160e+01, // 911
 4.524149905580855e-02, // 912
 6.397966200287748e+00, // 913
-8.237103201827026e+00, // 914
 2.628949014878565e-01, // 915
 9.856454685736329e-03, // 916
 2.428013145799532e+01, // 917
 5.613745490419843e-02, // 918
-1.366382944875315e+00, // 919
-1.445579675118408e+00, // 920
 3.424723107859182e-01, // 921
-1.726772771307961e+02, // 922
 4.911826338225115e+00, // 923
-2.708314259163625e+02, // 924
-9.011931070277188e-01, // 925
 6.834178885671326e+00, // 926
-2.063728660606050e+00, // 927
 9.889816346454889e-01, // 928
 2.584387487890317e+01, // 929
-1.961807018448654e-01, // 930
-5.812981417913910e+01, // 931
 4.723839697001353e+00, // 932
-2.602570459133419e+00, // 933
 2.479538835526929e+00, // 934
 1.223905606128930e+00, // 935
 5.598205930885086e-02, // 936
 6.018651997570558e-01, // 937
 7.340531598364707e+00, // 938
 1.096848312234155e+00, // 939
-4.967750716278831e+00, // 940
 4.015087261310656e+00, // 941
-2.391644270764874e+01, // 942
 1.164668715763810e+02, // 943
-1.079716841459888e-02, // 944
-3.646608756453689e-01, // 945
-2.775181344908647e-01, // 946
 4.225837856991754e+00, // 947
 1.271640281933485e-01, // 948
-3.023257267358227e+01, // 949
 1.317682455272484e-01, // 950
 4.751050934104493e-02, // 951
-1.598425744409931e+02, // 952
-4.202116317725513e-01, // 953
 7.771932478115205e-01, // 954
 1.238902198830985e-01, // 955
 8.891087358012314e+01, // 956
 5.259701735573916e+01, // 957
-3.456568601778222e-02, // 958
-4.088852746408875e+01, // 959
 3.476806177795893e+01, // 960
 2.195273719448012e-02, // 961
 1.377054796523926e+01, // 962
-2.638234165742815e+00, // 963
 2.283190693365562e-01, // 964
-6.079808182815665e+01, // 965
-9.914428792923660e+01, // 966
 2.324578063786960e-01, // 967
-3.215900548629580e-01, // 968
 1.753067630033766e-01, // 969
 3.680935490740941e-01, // 970
-7.292336684514360e-01, // 971
-1.892582149899777e-01, // 972
 4.890117859606154e-01, // 973
-9.924726701594308e-02, // 974
 8.791260376735896e-02, // 975
 1.058830060557437e+01, // 976
-2.002932202808728e+01, // 977
-9.755203816836935e-01, // 978
-2.260976748726830e-01, // 979
 4.523024081396174e+00, // 980
 1.815860453919855e+01, // 981
 8.261481911240988e-01, // 982
 6.679768215753062e+01, // 983
 2.117102815551387e-02, // 984
-3.453192794951858e+01, // 985
 6.391815672842215e+01, // 986
 3.190841732232922e+00, // 987
-4.160984960599464e-01, // 988
-2.622591905605268e-01, // 989
-1.524619792071420e+00, // 990
 4.908630337397826e-02, // 991
 1.375975151762782e-01, // 992
 1.131548127302969e-01, // 993
 2.628641355407702e-01, // 994
-7.955573229558887e+00, // 995
-3.857643676190168e+01, // 996
-7.418033108489932e-03, // 997
-2.813790103402055e+01, // 998
 1.323782136473282e-01, // 999
 8.504882249549508e-01, // 1000
-2.448614420088641e-01, // 1001
 2.856860068818294e+00, // 1002
-3.419976764027896e+01, // 1003
 1.870826758861098e+01, // 1004
 1.902953612704008e-01, // 1005
-1.113932504560623e+02, // 1006
-2.209552026853531e-01, // 1007
 4.316079765991173e-02, // 1008
 7.792614306106895e-01, // 1009
 2.644124876499101e-01, // 1010
 3.772724137483284e-01, // 1011
-5.256578120944654e+00, // 1012
-1.665713546621686e-01, // 1013
 4.670965024376204e+01, // 1014
 5.805411575771998e+00, // 1015
-1.352144469803762e-01, // 1016
 1.374303139350045e-01, // 1017
 6.822595945771944e+00, // 1018
-2.004870309642120e-01, // 1019
-7.910807493009386e+01, // 1020
-2.560327427798165e+01, // 1021
 9.621445952770443e+00, // 1022
-2.742296098588659e-02, // 1023
 2.803703388041692e+01, // 1024
-9.215549318652433e-02, // 1025
 1.168457719033896e-01, // 1026
 2.952024896927490e-03, // 1027
-5.466244432955993e+00, // 1028
-1.004338072253257e-01, // 1029
-4.015134082610058e-02, // 1030
 3.349273162416199e-01, // 1031
 1.203769773872290e-02, // 1032
-2.280201610459190e-01, // 1033
-2.266011093166224e-01, // 1034
 2.693186144367233e-01, // 1035
 1.292015413281684e+00, // 1036
-7.311285320581250e-01, // 1037
-4.706057253856204e-01, // 1038
 6.547916504439310e+01, // 1039
 4.058406153046940e-01, // 1040
-1.568978731671831e-01, // 1041
 7.955350866077073e-01, // 1042
 7.671906845664097e+01, // 1043
-1.864232610361673e-02, // 1044
 1.709897848508735e-01, // 1045
-1.039419781836381e+01, // 1046
 1.542809379252928e-01, // 1047
-1.772705341848704e-01, // 1048
 5.232737183941151e-01, // 1049
 1.135881568168288e-01, // 1050
-1.857884608293104e-02, // 1051
 8.771354498024677e+01, // 1052
-2.024993477469181e+01, // 1053
-2.456049052163152e+01, // 1054
-7.852026015166602e-03, // 1055
-7.457845095075391e-01, // 1056
-3.051096110359556e-01, // 1057
-1.303753350684433e+01, // 1058
 6.717083355003233e+01, // 1059
 6.786547669990765e+01, // 1060
 1.078811097378346e+02, // 1061
-7.487074914079996e+01, // 1062
-4.430872839210367e+01, // 1063
-1.973230237611513e+00, // 1064
 1.283390914658215e+01, // 1065
 1.436145014343586e+00, // 1066
 5.124818387580059e-01, // 1067
 1.438407085873884e-01, // 1068
-8.494745586542820e+00, // 1069
-2.478265875326957e+00, // 1070
-5.533674940667552e-01, // 1071
-2.222049409327537e+01, // 1072
-1.232541961629145e+00, // 1073
-1.378559733917466e+01, // 1074
 2.134212365303669e-02, // 1075
 7.975222141495535e+00, // 1076
 1.947783857564901e-01, // 1077
-6.590534062866027e-02, // 1078
-7.446673370825257e-01, // 1079
-1.773653523812863e+01, // 1080
-1.574039852759397e-01, // 1081
 1.154568664128096e+02, // 1082
 4.077366614398601e-01, // 1083
 6.058629282499643e+01, // 1084
 5.735031821614970e+00, // 1085
 1.287244606698774e+01, // 1086
 8.972693300150107e+00, // 1087
 9.461731703228866e-01, // 1088
 3.770840395697334e-01, // 1089
-5.209084224321019e-04, // 1090
 2.016969208432047e-02, // 1091
 2.606516487301353e+01, // 1092
-3.715478603899320e+01, // 1093
 1.636079909388601e+00, // 1094
-4.780024696233453e-01, // 1095
 4.485379444992227e+01, // 1096
 3.543082773972279e-02, // 1097
-1.997439789356867e+01, // 1098
 1.670554309396994e-01, // 1099
-1.025698145277279e+01, // 1100
 3.304178820771832e-02, // 1101
-2.214524552612426e+01, // 1102
-4.472094093633118e+01, // 1103
-7.843725979380752e+00, // 1104
 3.676892404535590e+00, // 1105
 6.168566820913009e+00, // 1106
-2.535343341700006e-02, // 1107
 2.203583160175888e-02, // 1108
-2.793266883125836e+00, // 1109
 2.434389988050379e-01, // 1110
-1.287555062737244e-01, // 1111
-4.830964350311815e-03, // 1112
-4.958369075863692e+01, // 1113
-1.217507179237039e+00, // 1114
-8.069695185985854e+00, // 1115
 6.046271593699291e-02, // 1116
-7.048812081096729e-02, // 1117
 2.094083245142395e+01, // 1118
 9.760872901764019e+01, // 1119
-7.775567772448345e+00, // 1120
 3.702194014647192e-01, // 1121
-9.117518400135806e+01, // 1122
-9.973585063892926e-02, // 1123
-1.300831300995172e+00, // 1124
 1.716401711355345e-01, // 1125
-1.178723321445317e+02, // 1126
 4.050320881911354e-01, // 1127
-3.743787075996361e-01, // 1128
 2.904116379210401e-01, // 1129
-1.651238507300621e+02, // 1130
-3.093477140076714e+00, // 1131
-4.875910614305632e-01, // 1132
 4.845739524996125e+00, // 1133
-1.580278730812439e+01, // 1134
 1.116946878413564e+02, // 1135
-2.907755316147156e+01, // 1136
 1.143370740769241e+02, // 1137
-3.900682106207473e-01, // 1138
 5.377077910027745e+01, // 1139
-2.647447536284292e-01, // 1140
-2.146364446376714e+01, // 1141
 1.501037580349408e-01, // 1142
 1.047661973539622e-02, // 1143
-3.032513054209905e-01, // 1144
 6.137219524460383e-01, // 1145
 5.919640806022328e-02, // 1146
 1.422792475097124e+00, // 1147
 6.838143285744047e+01, // 1148
-4.509251327397191e+01, // 1149
-1.523573933088222e-01, // 1150
-8.806271551499056e+01, // 1151
-1.637656207819073e+00  // 1152
};

//----------------------------------------------------------------------------//

x2b_v6x_p::x2b_v6x_p()
{
    m_k_HH_intra = -6.480884773303821e-01; // A^(-1)
    m_k_OH_intra =  1.674518993682975e+00; // A^(-1)

    m_k_HH_coul = 1.148231864355956e+00; // A^(-1)
    m_k_OH_coul = 1.205989761123099e+00; // A^(-1)
    m_k_OO_coul = 1.395357065790959e+00; // A^(-1)

    m_k_XH_main = 7.347036852042255e-01; // A^(-1)
    m_k_XO_main = 7.998249864422826e-01; // A^(-1)
    m_k_XX_main = 7.960663960630585e-01; // A^(-1)

    m_in_plane_gamma = -9.721486914088159e-02;
    m_out_of_plane_gamma = 9.859272078406150e-02;

    m_r2i =  4.500000000000000e+00; // A
    m_r2f =  6.500000000000000e+00; // A

    std::copy(the_poly, the_poly + poly_type::size, m_poly);
};

//----------------------------------------------------------------------------//

static const x2b_v6x_p the_model;

//----------------------------------------------------------------------------//

} // namespace

////////////////////////////////////////////////////////////////////////////////

extern "C" {

//----------------------------------------------------------------------------//

// PP_: added force_matrix

#ifdef BGQ
void mbpol_2b_poly(const double* w1, const double* w2,
                    double* E, double* g1, double* g2, double* force_matrix)
#else
void mbpol_2b_poly_(const double* w1, const double* w2,
                    double* E, double* g1, double* g2, double* force_matrix)
#endif
{
    *E = the_model(w1, w2, g1, g2, force_matrix);  // PP_: added force_matrix
}

//----------------------------------------------------------------------------//

#ifdef BGQ
void mbpol_2b_cutoff(double* r)
#else
void mbpol_2b_cutoff_(double* r)
#endif
{
    *r = the_model.m_r2f;
}

//----------------------------------------------------------------------------//

} // extern "C"

////////////////////////////////////////////////////////////////////////////////
