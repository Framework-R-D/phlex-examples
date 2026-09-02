// Design4: registration of the Gaussian hit-fitting transform as
// a separate module.  Takes two inputs: wire_roi_data from the
// second unfold and merge_hit_candidate_vec from cand_hit_standard.
//
// See README.md for some general comments about this example.

#include <memory>
#include <string>
#include <vector>

#include "phlex/concurrency.hpp"
#include "phlex/configuration.hpp"
#include "phlex/core/product_selector.hpp"
#include "phlex/module.hpp"

#include "copied_from_larsoft_minor_edits/HitFilterAlg.h"
#include "copied_from_larsoft_minor_edits/PeakFitterMrqdt.h"
#include "find_hits_with_gaussians_design4.hpp"

using namespace phlex;

namespace {
  examples::find_hits_with_gaussians_design4_cfg main_cfg(configuration config)
  {
    return {.filter_hits = config.get<bool>("filter_hits"),
            .long_max_hits_vec = config.get<std::vector<int>>("long_max_hits_vec"),
            .long_pulse_width_vec = config.get<std::vector<int>>("long_pulse_width_vec"),
            .max_multi_hit = config.get<int>("max_multi_hit"),
            .area_method = config.get<int>("area_method"),
            .area_norms_vec = config.get<std::vector<double>>("area_norms_vec"),
            .chi2_ndf = config.get<double>("chi2_ndf"),
            .pulse_height_cuts = config.get<std::vector<float>>("pulse_height_cuts"),
            .pulse_width_cuts = config.get<std::vector<float>>("pulse_width_cuts"),
            .pulse_ratio_cuts = config.get<std::vector<float>>("pulse_ratio_cuts")};
  }

  std::shared_ptr<examples::PeakFitterMrqdt> make_peak_fitter_mrqdt(configuration config)
  {
    auto fitter_config = config.get<configuration>("peak_fitter_mrqdt_config");
    return std::make_shared<examples::PeakFitterMrqdt>(
      examples::PeakFitterMrqdtCfg{.fMinWidth = fitter_config.get<double>("min_width"),
                                   .fMaxWidthMult = fitter_config.get<double>("max_width_mult"),
                                   .fPeakRange = fitter_config.get<double>("peak_range_fact"),
                                   .fAmpRange = fitter_config.get<double>("peak_amp_range")});
  }

  std::shared_ptr<examples::HitFilterAlg> make_hit_filter_alg(configuration config)
  {
    auto filter_config = config.get<configuration>("hit_filter_alg_config");
    return std::make_shared<examples::HitFilterAlg>(examples::HitFilterAlgCfg{
      .fMinPulseHeight = filter_config.get<std::vector<float>>("min_pulse_height"),
      .fMinPulseSigma = filter_config.get<std::vector<float>>("min_pulse_sigma")});
  }
}

PHLEX_REGISTER_ALGORITHMS(m, config)
{
  auto const layer_roi = config.get<std::string>("layer_roi");
  auto const cand_hit_finder = config.get<std::string>("cand_hit_finder");

  m.transform(
     "find_hits_with_gaussians_design4",
     [cfg = main_cfg(config),
      peak_fitter_mrqdt = make_peak_fitter_mrqdt(config),
      hit_filter_alg =
        make_hit_filter_alg(config)](examples::wire_roi_data const& roi_data,
                                     examples::merge_hit_candidate_vec const& merged_candidates) {
       return examples::find_hits_with_gaussians_design4(
         cfg, roi_data, merged_candidates, *peak_fitter_mrqdt, *hit_filter_alg);
     },
     concurrency::unlimited)
    .input_family(product_selector{.creator = "unfold_wire_design4", .layer = layer_roi},
                  product_selector{.creator = cand_hit_finder, .layer = layer_roi});
}
