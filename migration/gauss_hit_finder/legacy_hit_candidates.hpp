#ifndef MIGRATION_GAUSS_HIT_FINDER_LEGACY_HIT_CANDIDATES_HPP
#define MIGRATION_GAUSS_HIT_FINDER_LEGACY_HIT_CANDIDATES_HPP

////////////////////////////////////////////////////////////////////////
/// \file   legacy_hit_candidates.hpp
///
/// \brief  Temporary bridge from the new hit_candidate_vec type to the
///         legacy ICandidateHitFinder::HitCandidateVec expected by
///         PeakFitterMrqdt.
///
/// In the near future, when we migrate the peak fitter code to be a
/// transform, we should make it also use hit_candidate_vec and delete
/// this file.
////////////////////////////////////////////////////////////////////////

#include "copied_from_larsoft_minor_edits/ICandidateHitFinder.h"
#include "hit_candidate.hpp"

namespace examples {

  // Crude guard against a field being added to only one of the two structs.
  static_assert(sizeof(hit_candidate) == sizeof(ICandidateHitFinder::HitCandidate),
                "hit_candidate and ICandidateHitFinder::HitCandidate have diverged; "
                "update to_legacy_candidates");

  inline ICandidateHitFinder::HitCandidateVec to_legacy_candidates(
    hit_candidate_vec const& candidates)
  {
    ICandidateHitFinder::HitCandidateVec result;
    result.reserve(candidates.size());
    for (auto const& c : candidates) {
      result.push_back({.startTick = c.start_tick,
                        .stopTick = c.stop_tick,
                        .maxTick = c.max_tick,
                        .minTick = c.min_tick,
                        .maxDerivative = c.max_derivative,
                        .minDerivative = c.min_derivative,
                        .hitCenter = c.hit_center,
                        .hitSigma = c.hit_sigma,
                        .hitHeight = c.hit_height});
    }
    return result;
  }

} // namespace examples

#endif // MIGRATION_GAUSS_HIT_FINDER_LEGACY_HIT_CANDIDATES_HPP
