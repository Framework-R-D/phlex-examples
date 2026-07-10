#ifndef MIGRATION_GAUSS_HIT_FINDER_FIND_HITS_WITH_GAUSSIANS_DESIGN1_HPP
#define MIGRATION_GAUSS_HIT_FINDER_FIND_HITS_WITH_GAUSSIANS_DESIGN1_HPP

// See REAMDME.md for some general comments about this example.

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

#include <memory>
#include <utility>
#include <vector>

#include "tbb/concurrent_vector.h"

#include "copied_from_larsoft_minor_edits/CandHitStandard.h"
#include "copied_from_larsoft_minor_edits/Hit.h"
#include "copied_from_larsoft_minor_edits/HitFilterAlg.h"
#include "copied_from_larsoft_minor_edits/PeakFitterMrqdt.h"
#include "copied_from_larsoft_minor_edits/Wire.h"

namespace examples {

  // First, declare the unfold algorithm
  // Outputs Wire objects using a vector<recob::Wire> as input

  class unfold_wire_vector_design1 {
  public:
    explicit unfold_wire_vector_design1(std::vector<recob::Wire> const& wires);

    using const_iterator = std::vector<recob::Wire>::const_iterator;

    const_iterator initial_value() const;

    bool predicate(const_iterator current) const;

    std::pair<const_iterator, recob::Wire> unfold(const_iterator current) const;

  private:
    const_iterator begin_;
    const_iterator end_;
  };

  // Second, define the transform, the actual hit finding algorithm

  struct find_hits_with_gaussians_design1_cfg {
    bool filter_hits;

    std::vector<int> long_max_hits_vec;    ///<Maximum number hits on a really long pulse train
    std::vector<int> long_pulse_width_vec; ///<Sets width of hits used to describe long pulses
    int max_multi_hit;                     ///<maximum hits for multi fit
    int area_method;                       ///<Type of area calculation
    std::vector<double> area_norms_vec; ///<factors for converting area to same units as peak height
    double chi2_ndf;                    ///maximum Chisquared / NDF allowed for a hit to be saved

    std::vector<float> pulse_height_cuts;
    std::vector<float> pulse_width_cuts;
    std::vector<float> pulse_ratio_cuts;
  };

  tbb::concurrent_vector<recob::Hit> find_hits_with_gaussians_design1(
    find_hits_with_gaussians_design1_cfg const& cfg,
    recob::Wire const& wire,
    std::vector<std::shared_ptr<CandHitStandard>> const& cand_hit_standard,
    PeakFitterMrqdt const& peak_fitter_mrqdt,
    HitFilterAlg const& hit_filter_alg);

  // Third, define the fold, fills the output vector of hits
  void fold_hits_into_vector_design1(std::vector<recob::Hit>& hits,
                                     tbb::concurrent_vector<recob::Hit> const& hits_from_wire);
}
#endif // MIGRATION_GAUSS_HIT_FINDER_FIND_HITS_WITH_GAUSSIANS_DESIGN1_HPP
