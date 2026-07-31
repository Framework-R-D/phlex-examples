#ifndef PHLEX_EXAMPLES_FIND_HITS_WITH_GAUSSIANS_DESIGN2_HPP
#define PHLEX_EXAMPLES_FIND_HITS_WITH_GAUSSIANS_DESIGN2_HPP

// Design2 extends design1 by also replacing the inner parallel_for
// (over ROIs) with a second unfold-transform-fold.

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
#include <memory>
#include <utility>
#include <vector>

#include "copied_from_larsoft_minor_edits/CandHitStandard.h"
#include "copied_from_larsoft_minor_edits/Hit.h"
#include "copied_from_larsoft_minor_edits/HitFilterAlg.h"
#include "copied_from_larsoft_minor_edits/PeakFitterMrqdt.h"
#include "copied_from_larsoft_minor_edits/RawTypes.h" // raw::ChannelID_t
#include "copied_from_larsoft_minor_edits/Wire.h"

namespace examples {

  // ---------------------------------------------------------------
  // A lightweight struct that bundles a single ROI (datarange_t)
  // together with the wire-level context needed by the transform.
  // ---------------------------------------------------------------
  struct wire_roi_data {
    recob::Wire::RegionsOfInterest_t::datarange_t range;
    raw::ChannelID_t channel;
    int view;
  };

  // ---------------------------------------------------------------
  // First unfold: vector<Wire> -> individual Wire objects
  // (unchanged from design1)
  // ---------------------------------------------------------------
  class unfold_wire_vector_design2 {
  public:
    explicit unfold_wire_vector_design2(std::vector<recob::Wire> const& wires);

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
  // Replaces the inner tbb::parallel_for over ROI ranges.
  // ---------------------------------------------------------------
  class unfold_wire_design2 {
  public:
    explicit unfold_wire_design2(recob::Wire const& wire);

    using state_type = std::size_t;  // index into signalROI ranges

    state_type initial_value() const;
    bool predicate(state_type current) const;
    std::pair<state_type, wire_roi_data> unfold(state_type current) const;

  private:
    recob::Wire const& wire_;
    std::size_t n_ranges_;
  };

  // ---------------------------------------------------------------
  // Configuration for the transform
  // ---------------------------------------------------------------
  struct find_hits_with_gaussians_design2_cfg {
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
  // Transform: processes a single ROI and returns the hits found
  // ---------------------------------------------------------------
  std::vector<recob::Hit> find_hits_with_gaussians_design2(
    find_hits_with_gaussians_design2_cfg const& cfg,
    wire_roi_data const& roi_data,
    std::vector<std::shared_ptr<CandHitStandard>> const& cand_hit_standard,
    PeakFitterMrqdt const& peak_fitter_mrqdt,
    HitFilterAlg const& hit_filter_alg);

  // ---------------------------------------------------------------
  // Inner fold: collects hits from individual ROIs into a
  // per-wire vector  (roi layer -> wire layer)
  // ---------------------------------------------------------------
  void fold_roi_hits_design2(std::vector<recob::Hit>& hits,
                             std::vector<recob::Hit> const& hits_from_roi);

  // ---------------------------------------------------------------
  // Outer fold: collects per-wire hit vectors into the final
  // output vector  (wire layer -> spill layer)
  // ---------------------------------------------------------------
  void fold_hits_into_vector_design2(std::vector<recob::Hit>& hits,
                                     std::vector<recob::Hit> const& hits_from_wire);
}
#endif // PHLEX_EXAMPLES_FIND_HITS_WITH_GAUSSIANS_DESIGN2_HPP
