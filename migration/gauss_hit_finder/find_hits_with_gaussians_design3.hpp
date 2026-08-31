#ifndef PHLEX_EXAMPLES_FIND_HITS_WITH_GAUSSIANS_DESIGN3_HPP
#define PHLEX_EXAMPLES_FIND_HITS_WITH_GAUSSIANS_DESIGN3_HPP

// Design3 extends design2 by extracting the candidate-hit-finding step
// (cand_hit_standard) into a separate transform that runs after the
// second unfold.  The Gaussian hit-fitting transform receives the
// pre-computed merged hit candidates instead of computing them itself.

// See README.md for some general comments about this example.

////////////////////////////////////////////////////////////////////////
//
// GaussHitFinder class
//
// jaasaadi@syr.edu
//
//  This algorithm is designed to find hits on wires after deconvolution.
// -----------------------------------
// This algorithm is based on the FFTHitFinder written by Brian Page,
// Michigan State University, for the ArgoNeuT experiment.
//
//
// The algorithm walks along the wire and looks for pulses above threshold
// The algorithm then attempts to fit n-gaussians to these pulses where n
// is set by the number of peaks found in the pulse
// If the Chi2/NDF returned is "bad" it attempts to fit n+1 gaussians to
// the pulse. If this is a better fit it then uses the parameters of the
// Gaussian fit to characterize the "hit" object
//
// To use this simply include the following in your producers:
// gaushit:     @local::microboone_gaushitfinder
// gaushit:	@local::argoneut_gaushitfinder
////////////////////////////////////////////////////////////////////////

#include <cstddef>
#include <utility>
#include <vector>

#include "copied_from_larsoft_minor_edits/Hit.h"
#include "copied_from_larsoft_minor_edits/HitFilterAlg.h"
#include "copied_from_larsoft_minor_edits/PeakFitterMrqdt.h"
#include "copied_from_larsoft_minor_edits/Wire.h"
#include "hit_candidate.hpp"
#include "wire_roi_data.hpp"

namespace examples {

  // ---------------------------------------------------------------
  // First unfold: vector<Wire> -> individual Wire objects
  // (unchanged from design2)
  // ---------------------------------------------------------------
  class unfold_wire_vector_design3 {
  public:
    explicit unfold_wire_vector_design3(std::vector<recob::Wire> const& wires);

    using const_iterator = std::vector<recob::Wire>::const_iterator;

    const_iterator initial_value() const;
    bool predicate(const_iterator current) const;
    std::pair<const_iterator, recob::Wire> unfold(const_iterator current) const;

  private:
    const_iterator begin_;
    const_iterator end_;
  };

  // ---------------------------------------------------------------
  // Second unfold: Wire -> individual wire_roi_data objects
  // (unchanged from design2)
  // ---------------------------------------------------------------
  class unfold_wire_design3 {
  public:
    explicit unfold_wire_design3(recob::Wire const& wire);

    using state_type = std::size_t;  // index into signalROI ranges

    state_type initial_value() const;
    bool predicate(state_type current) const;
    std::pair<state_type, wire_roi_data> unfold(state_type current) const;

  private:
    recob::Wire const& wire_;
    std::size_t n_ranges_;
  };

  // ---------------------------------------------------------------
  // Configuration for the Gaussian hit-fitting transform
  // (unchanged from design2)
  // ---------------------------------------------------------------
  struct find_hits_with_gaussians_design3_cfg {
    bool filter_hits;

    std::vector<int> long_max_hits_vec;    ///<Maximum number hits on a really long pulse train
    std::vector<int> long_pulse_width_vec; ///<Sets width of hits used to describe long pulses
    int max_multi_hit; ///<maximum hits for multi fit
    int area_method;     ///<Type of area calculation
    std::vector<double>
      area_norms_vec;       ///<factors for converting area to same units as peak height
    double chi2_ndf; ///maximum Chisquared / NDF allowed for a hit to be saved

    std::vector<float> pulse_height_cuts;
    std::vector<float> pulse_width_cuts;
    std::vector<float> pulse_ratio_cuts;
  };

  // ---------------------------------------------------------------
  // Transform: processes pre-computed merged hit candidates for a
  // single ROI and returns the hits found.  Unlike design2, this
  // does NOT find hit candidates internally — that work has
  // already been done by the upstream cand_hit_standard transform.
  //
  // Both the wire_roi_data (from the second unfold) and the
  // merge_hit_candidate_vec (from cand_hit_standard) are provided
  // as separate inputs.
  // ---------------------------------------------------------------
  std::vector<recob::Hit> find_hits_with_gaussians_design3(
    find_hits_with_gaussians_design3_cfg const& cfg,
    wire_roi_data const& roi_data,
    merge_hit_candidate_vec const& merged_candidates,
    PeakFitterMrqdt const& peak_fitter_mrqdt,
    HitFilterAlg const& hit_filter_alg);

  // ---------------------------------------------------------------
  // Inner fold: collects hits from individual ROIs into a
  // per-wire vector  (roi layer -> wire layer)
  // (unchanged from design2)
  // ---------------------------------------------------------------
  void fold_roi_hits_design3(std::vector<recob::Hit>& hits,
                             std::vector<recob::Hit> const& hits_from_roi);

  // ---------------------------------------------------------------
  // Outer fold: collects per-wire hit vectors into the final
  // output vector  (wire layer -> spill layer)
  // (unchanged from design2)
  // ---------------------------------------------------------------
  void fold_hits_into_vector_design3(std::vector<recob::Hit>& hits,
                                     std::vector<recob::Hit> const& hits_from_wire);
}
#endif // PHLEX_EXAMPLES_FIND_HITS_WITH_GAUSSIANS_DESIGN3_HPP
