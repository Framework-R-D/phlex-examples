#ifndef MIGRATION_GAUSS_HIT_FINDER_GAUS_HPP
#define MIGRATION_GAUSS_HIT_FINDER_GAUS_HPP

////////////////////////////////////////////////////////////////////////
/// \file   gaus.hpp
///
/// \brief  Gaussian function shared by the find_hits_with_gaussians
///         design variants.
////////////////////////////////////////////////////////////////////////

#include <cmath>

namespace examples {

  // This is an edited copy of the TMath::Gaus function from ROOT, since we
  // don't want to depend on ROOT in this example.
  inline double Gaus(double x, double mean, double sigma, bool norm)
  {
    if (sigma == 0)
      return 1.e30;
    double arg = (x - mean) / sigma;
    // for |arg| > 39  result is zero in double precision
    if (arg < -39.0 || arg > 39.0)
      return 0.0;
    double res = std::exp(-0.5 * arg * arg);
    if (!norm)
      return res;
    return res / (2.50662827463100024 * sigma); //sqrt(2*Pi)=2.50662827463100024
  }

} // namespace examples

#endif // MIGRATION_GAUSS_HIT_FINDER_GAUS_HPP
