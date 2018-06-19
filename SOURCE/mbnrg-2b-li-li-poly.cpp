#include <cmath>

#include <algorithm>

#include "poly-2b-A1-A1-v1x.h"

////////////////////////////////////////////////////////////////////////////////

namespace {

//----------------------------------------------------------------------------//

struct variable {
    double v_exp(const double& r0, const double& k,
                 const double* xcrd, int x, int y );

    double v_coul(const double& r0, const double& k,
                  const double* xcrd, int x, int y );

    void grads(const double& gg, double* xgrd, int x, int y ) const;

    double g[3]; // diff(value, p1 - p2)
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - //

double variable::v_exp(const double& r0, const double& k,
                       const double* xcrd, int x, int y )
{

    g[0] = xcrd[x++] - xcrd[y++];
    g[1] = xcrd[x++] - xcrd[y++];
    g[2] = xcrd[x]   - xcrd[y];

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
                        const double* xcrd, int x, int y)
{
    g[0] = xcrd[x++] - xcrd[y++];
    g[1] = xcrd[x++] - xcrd[y++];
    g[2] = xcrd[x]   - xcrd[y];

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

void variable::grads(const double& gg, double* xgrd, int x, int y) const
{
    for (int i = 0; i < 3; ++i) {
        const double d = gg*g[i];

        xgrd[x++] += d;
        xgrd[y++] -= d;
    }
}


////////////////////////////////////////////////////////////////////////////////


struct x2b_A1_A1_v1x_p {
    x2b_A1_A1_v1x_p();

    typedef A1_A1::poly_2b_A1_A1_v1x poly_type;

    double operator()(const double* w, const double* x,
                      double* g1, double* g2) const;

protected:
    double k_AA;
    
    double d_AA;

public:
    double m_r2i;
    double m_r2f;

    double f_switch(const double&, double&) const; // X-Y separation

protected:
    double m_poly[poly_type::size];
};

////////////////////////////////////////////////////////////////////////////////
//what should the switching function be for this?
double x2b_A1_A1_v1x_p::f_switch(const double& r, double& g) const
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

double x2b_A1_A1_v1x_p::operator()
    (const double* w, const double* x, double* g1, double* g2) const
{
    // the switch

    const double dAA[3] = {w[0] - x[0],
                           w[1] - x[1],
                           w[2] - x[2]};

    const double rAAsq = dAA[0]*dAA[0] + dAA[1]*dAA[1] + dAA[2]*dAA[2];
    const double rAA = std::sqrt(rAAsq);

    if (rAA > m_r2f)
        return 0.0;

    // offsets
    const int X   = 0;
    const int Y   = 3;

    double xcrd[6]; // coordinates including extra-points
//need to modify this thingy?
    std::copy(w, w + 3 , xcrd);
    std::copy(x , x  + 3, xcrd + 3);

    // variables

//    const double d0_intra = 1.0;  //TODO MBpol values 
//    const double d0_inter = 4.0;

    double v[ 1]; // stored separately (gets passed to poly::eval)

    variable ctxt[1 ];

    v[0] = ctxt[0].v_exp(d_AA, k_AA, xcrd, X, Y);

    double g[1 ];
    const double E_poly = A1_A1::poly_2b_A1_A1_v1x::eval(m_poly, v, g);

    double xgrd[6];
    std::fill(xgrd, xgrd + 6, 0.0);

    ctxt[0].grads(g[0], xgrd, X, Y);
                       
    // distribute gradients w.r.t. the X-points


    // the switch

    double gsw;
    const double sw = f_switch(rAA, gsw);

    for (int i = 0; i < 3; ++i) {
        g1[i] = sw*xgrd[i];
    }

    for (int i = 0; i < 3; ++i) {
        g2[i] += sw*xgrd[i + 3];
    }

    // gradient of the switch

    gsw *= E_poly/rAA;
    for (int i = 0; i < 3; ++i) {
        const double d = gsw*dAA[i];
        g1[i] += d;
        g2[i] -= d;
    }

    return sw*E_poly;
}

//----------------------------------------------------------------------------//

//fit-fullpolargrid-fixedwaterparams effpolfac 03/15/17
static const double the_poly[] = {
 2.189635745813810e-03, // 0
-1.002074315769640e-02, // 1
 2.098752755975131e-02, // 2
-2.317059788562559e-02, // 3
 1.568250415078895e-02, // 4
-6.897335839521867e-03, // 5
 2.045206858834825e-03, // 6
-4.154273538257000e-04, // 7
 5.777313828774001e-05, // 8
-5.390859533752413e-06, // 9
 3.213363659213734e-07, // 10
-1.100551647955350e-08, // 11
 1.651928503898949e-10 // 12
};

//----------------------------------------------------------------------------//

x2b_A1_A1_v1x_p::x2b_A1_A1_v1x_p()
{
    d_AA =  5.567958391183113e+00; // A^(-1))    
    k_AA =  5.212691985268002e-01; // A^(-1))   
 
    m_r2i =  7.000000000000000e+00; // A
    m_r2f =  8.000000000000000e+00; // A

    std::copy(the_poly, the_poly + poly_type::size, m_poly);
};

//----------------------------------------------------------------------------//

static const x2b_A1_A1_v1x_p the_model;

//----------------------------------------------------------------------------//

} // namespace

////////////////////////////////////////////////////////////////////////////////

extern "C" {

//----------------------------------------------------------------------------//

#ifdef BGQ
void mbnrg_2b_li_li_poly(const double* w, const double* x ,
                    double* E, double* g1, double* g2)
#else
void mbnrg_2b_li_li_poly_(const double* w, const double* x ,
                    double* E, double* g1, double* g2)
#endif
{
    *E = the_model(w , x , g1, g2);
}

//----------------------------------------------------------------------------//

#ifdef BGQ
void mbnrg_2b_li_li_cutoff(double* r)
#else
void mbnrg_2b_li_li_cutoff_(double* r)
#endif
{
    *r = the_model.m_r2f;
}

//----------------------------------------------------------------------------//

} // extern "C"

////////////////////////////////////////////////////////////////////////////////
