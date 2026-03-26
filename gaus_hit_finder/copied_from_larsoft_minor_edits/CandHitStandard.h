#ifndef CANDHITSTANDARD_H
#define CANDHITSTANDARD_H

////////////////////////////////////////////////////////////////////////
/// \file   CandHitStandard.cc
/// \author T. Usher
////////////////////////////////////////////////////////////////////////

#include <cstddef>
#include <vector>

#include "ICandidateHitFinder.h"
#include "Wire.h"

namespace examples {

  struct CandHitStandardCfg {
    float fRoiThreshold;
  };

  class CandHitStandard : public ICandidateHitFinder {
  public:
    explicit CandHitStandard(const CandHitStandardCfg& cfg);

    void findHitCandidates(const recob::Wire::RegionsOfInterest_t::datarange_t&,
                           const size_t,
                           const size_t,
                           HitCandidateVec&) const override;

    void MergeHitCandidates(const recob::Wire::RegionsOfInterest_t::datarange_t&,
                            const HitCandidateVec&,
                            MergeHitCandidateVec&) const override;

  private:
    void findHitCandidates(std::vector<float>::const_iterator,
                           std::vector<float>::const_iterator,
                           const size_t,
                           HitCandidateVec&) const;

    const float fRoiThreshold; ///< minimum maximum to minimum peak distance
  };

} // namespace examples
#endif
