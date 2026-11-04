#ifndef MIGRATION_GAUSS_HIT_FINDER_CAND_HIT_DERIVATIVE_HPP
#define MIGRATION_GAUSS_HIT_FINDER_CAND_HIT_DERIVATIVE_HPP

////////////////////////////////////////////////////////////////////////
/// \file   cand_hit_derivative.hpp
/// \author T. Usher
////////////////////////////////////////////////////////////////////////

// WARNING: This is intended to give identical results to
// CandHitDerivatice from the LArSoft larreco repository.
// Unlike cand_hit_standard, this function was not tested to
// verify that the output is identical yet. I did test that
// It builds and executes without exceptions or seg faults.
// If this is actually used, then some more testing should be
// done and then this comment can be deleted...

#include <cstddef>
#include <vector>

#include "hit_candidate.hpp"
#include "wire_roi_data.hpp"

namespace examples {

  namespace cand_hit_derivative {

    struct cand_hit_derivative_cfg {
      int min_delta_ticks;               //< minimum ticks from max to min to consider
      float min_delta_peaks;             //< minimum maximum to minimum peak distance
      float min_hit_height;              //< drop candidate hits with height less than this
      std::size_t num_intervening_ticks; //< number of ticks between candidate hits to merge
    };

    merge_hit_candidate_vec find_and_merge_hit_candidates(cand_hit_derivative_cfg const& cfg,
                                                          wire_roi_data const& roi_data);

    void find_hit_candidates(cand_hit_derivative_cfg const& cfg,
                             std::vector<float>::const_iterator start,
                             std::vector<float>::const_iterator stop,
                             std::size_t const roi_start_tick,
                             int delta_ticks_threshold,
                             float delta_peak_threshold,
                             hit_candidate_vec& hit_candidates);
  } // namespace cand_hit_derivative

} // namespace examples
#endif // MIGRATION_GAUSS_HIT_FINDER_CAND_HIT_DERIVATIVE_HPP
