#ifndef PHLEX_EXAMPLES_FIND_HITS_WITH_GAUSSIANS_HPP
#define PHLEX_EXAMPLES_FIND_HITS_WITH_GAUSSIANS_HPP

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
#include <vector>

#include "copied_from_larsoft_minor_edits/CandHitStandard.h"
#include "copied_from_larsoft_minor_edits/Hit.h"
#include "copied_from_larsoft_minor_edits/HitFilterAlg.h"
#include "copied_from_larsoft_minor_edits/PeakFitterMrqdt.h"
#include "copied_from_larsoft_minor_edits/Wire.h"


namespace examples {

  struct find_hits_with_gaussians_cfg {
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

  std::vector<recob::Hit> find_hits_with_gaussians(find_hits_with_gaussians_cfg const& cfg,
                                                   std::vector<recob::Wire> const& wires,
                                                   std::vector<std::shared_ptr<CandHitStandard>> const& cand_hit_standard,
                                                   PeakFitterMrqdt const& peak_fitter_mrqdt,
                                                   HitFilterAlg const& hit_filter_alg);

}
#endif // PHLEX_EXAMPLES_FIND_HITS_WITH_GAUSSIANS_HPP
