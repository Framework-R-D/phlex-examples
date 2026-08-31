// Design3 extends design2 by inserting a cand_hit_standard transform
// between the second unfold and the Gaussian hit-fitting transform.
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
#include "copied_from_larsoft_minor_edits/Wire.h"
#include "cand_hit_standard.hpp"
#include "find_hits_with_gaussians_design3.hpp"

using namespace phlex;

namespace {
  examples::find_hits_with_gaussians_design3_cfg main_cfg(configuration config) {
    return {
      .filter_hits = config.get<bool>("filter_hits"),
      .long_max_hits_vec = config.get<std::vector<int>>("long_max_hits_vec"),
      .long_pulse_width_vec = config.get<std::vector<int>>("long_pulse_width_vec"),
      .max_multi_hit = config.get<int>("max_multi_hit"),
      .area_method = config.get<int>("area_method"),
      .area_norms_vec = config.get<std::vector<double>>("area_norms_vec"),
      .chi2_ndf = config.get<double>("chi2_ndf"),
      .pulse_height_cuts = config.get<std::vector<float>>("pulse_height_cuts"),
      .pulse_width_cuts = config.get<std::vector<float>>("pulse_width_cuts"),
      .pulse_ratio_cuts = config.get<std::vector<float>>("pulse_ratio_cuts")
    };
  }

  std::shared_ptr<examples::PeakFitterMrqdt> make_peak_fitter_mrqdt(configuration config) {
    auto fitter_config = config.get<configuration>("peak_fitter_mrqdt_config");
    return std::make_shared<examples::PeakFitterMrqdt>(
        examples::PeakFitterMrqdtCfg{
            .fMinWidth = fitter_config.get<double>("min_width"),
            .fMaxWidthMult = fitter_config.get<double>("max_width_mult"),
            .fPeakRange = fitter_config.get<double>("peak_range_fact"),
            .fAmpRange = fitter_config.get<double>("peak_amp_range")
    });
  }

  std::shared_ptr<examples::HitFilterAlg> make_hit_filter_alg(configuration config) {
    auto filter_config = config.get<configuration>("hit_filter_alg_config");
    return std::make_shared<examples::HitFilterAlg>(
        examples::HitFilterAlgCfg{
            .fMinPulseHeight = filter_config.get<std::vector<float>>("min_pulse_height"),
            .fMinPulseSigma = filter_config.get<std::vector<float>>("min_pulse_sigma")
    });
  }
}

PHLEX_REGISTER_ALGORITHMS(m, config)
{
  auto const layer_vector_of_wires = config.get<std::string>("layer_vector_of_wires");
  auto const layer_wire = config.get<std::string>("layer_wire");
  auto const layer_roi = config.get<std::string>("layer_roi");

  // ---------------------------------------------------------------
  // Outer unfold:  spill -> wire
  // ---------------------------------------------------------------
  m.unfold<examples::unfold_wire_vector_design3>("unfold_wire_vector_design3",
                                                 &examples::unfold_wire_vector_design3::predicate,
                                                 &examples::unfold_wire_vector_design3::unfold,
                                                 layer_wire,
                                                 concurrency::unlimited)
    .input_family(product_selector{.creator = "wires", .layer = layer_vector_of_wires, .suffix = ""});

  // ---------------------------------------------------------------
  // Inner unfold:  wire -> roi
  // ---------------------------------------------------------------
  m.unfold<examples::unfold_wire_design3>("unfold_wire_design3",
                                              &examples::unfold_wire_design3::predicate,
                                              &examples::unfold_wire_design3::unfold,
                                              layer_roi,
                                              concurrency::unlimited)
    .input_family(product_selector{.creator = "unfold_wire_vector_design3", .layer = layer_wire});

  // ---------------------------------------------------------------
  // Candidate hit finding:  wire_roi_data -> merge_hit_candidate_vec
  // ---------------------------------------------------------------
  m.transform("cand_hit_standard",
              [roi_threshold = config.get<std::vector<float>>("roi_threshold")]
              (examples::wire_roi_data const& roi_data) {
                return examples::cand_hit_standard::find_and_merge_hit_candidates(
                    roi_data, roi_threshold);
             },
              concurrency::unlimited)
    .input_family(product_selector{.creator = "unfold_wire_design3", .layer = layer_roi});

  // ---------------------------------------------------------------
  // Transform:  processes pre-computed merged hit candidates
  // for a single ROI.  Takes two inputs: the wire_roi_data from the
  // second unfold and the merge_hit_candidate_vec from
  // cand_hit_standard.
  // ---------------------------------------------------------------
  m.transform("find_hits_with_gaussians_design3",
              [cfg = main_cfg(config),
               peak_fitter_mrqdt = make_peak_fitter_mrqdt(config),
               hit_filter_alg = make_hit_filter_alg(config)]
              (examples::wire_roi_data const& roi_data,
               examples::merge_hit_candidate_vec const& merged_candidates) {
                 return examples::find_hits_with_gaussians_design3(cfg,
                                                                   roi_data,
                                                                   merged_candidates,
                                                                   *peak_fitter_mrqdt,
                                                                   *hit_filter_alg);
             },
              concurrency::unlimited)
    .input_family(product_selector{.creator = "unfold_wire_design3", .layer = layer_roi},
                  product_selector{.creator = "cand_hit_standard", .layer = layer_roi});

  // ---------------------------------------------------------------
  // Inner fold:  roi -> wire  (collects hits from ROIs of one wire)
  // ---------------------------------------------------------------
  m.fold("fold_roi_hits_design3", examples::fold_roi_hits_design3, concurrency::serial, layer_wire)
    .input_family(product_selector{.creator = "find_hits_with_gaussians_design3", .layer = layer_roi});

  // ---------------------------------------------------------------
  // Outer fold:  wire -> spill  (collects hits from all wires)
  // ---------------------------------------------------------------
  m.fold("fold_hits_into_vector_design3", examples::fold_hits_into_vector_design3, concurrency::serial, layer_vector_of_wires)
    .input_family(product_selector{.creator = "fold_roi_hits_design3", .layer = layer_wire});
}
