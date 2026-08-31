#ifndef PHLEX_EXAMPLES_MIGRATION_GAUSS_HIT_FINDER_CAND_HIT_STANDARD_HPP
#define PHLEX_EXAMPLES_MIGRATION_GAUSS_HIT_FINDER_CAND_HIT_STANDARD_HPP

////////////////////////////////////////////////////////////////////////
/// \file   cand_hit_standard.hpp
/// \author T. Usher
////////////////////////////////////////////////////////////////////////

#include <cstddef>
#include <vector>

#include "hit_candidate.hpp"
#include "wire_roi_data.hpp"

namespace examples {

  namespace cand_hit_standard {

    merge_hit_candidate_vec find_and_merge_hit_candidates(
        wire_roi_data const& roi_data,
        std::vector<float> const& roi_thresholds);

    void find_hit_candidates(std::vector<float>::const_iterator start,
                             std::vector<float>::const_iterator stop,
                             const std::size_t roi_start_tick,
                             const float roi_threshold,
                             hit_candidate_vec& hit_candidates);
  } // namespace cand_hit_standard

} // namespace examples
#endif // PHLEX_EXAMPLES_MIGRATION_GAUSS_HIT_FINDER_CAND_HIT_STANDARD_HPP
