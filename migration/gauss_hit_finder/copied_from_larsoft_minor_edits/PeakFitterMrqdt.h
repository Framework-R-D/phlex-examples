#ifndef PEAKFITTERMRQDT_H
#define PEAKFITTERMRQDT_H

#include <memory>
#include <vector>

#include "ICandidateHitFinder.h"
#include "IPeakFitter.h"
#include "MarqFitAlg.h" //marqfit functions

namespace examples {

  struct PeakFitterMrqdtCfg {
    double fMinWidth;
    double fMaxWidthMult;
    double fPeakRange;
    double fAmpRange;
  };

  class PeakFitterMrqdt : public IPeakFitter {
  public:
    explicit PeakFitterMrqdt(const PeakFitterMrqdtCfg& cfg);

    void findPeakParameters(const std::vector<float>&,
                            const ICandidateHitFinder::HitCandidateVec&,
                            PeakParamsVec&,
                            double&,
                            int&) const override;

  private:
    const double fMinWidth;
    const double fMaxWidthMult;
    const double fPeakRange;
    const double fAmpRange;

    std::unique_ptr<gshf::MarqFitAlg> fMarqFitAlg;
  };
}
#endif
