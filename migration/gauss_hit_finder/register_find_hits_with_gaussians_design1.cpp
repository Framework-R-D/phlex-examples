// See REAMDME.md for some general comments about this example.

#include <exception>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "phlex/concurrency.hpp"
#include "phlex/configuration.hpp"
#include "phlex/core/product_selector.hpp"
#include "phlex/module.hpp"

#include "copied_from_larsoft_minor_edits/CandHitStandard.h"
#include "copied_from_larsoft_minor_edits/HitFilterAlg.h"
#include "copied_from_larsoft_minor_edits/PeakFitterMrqdt.h"
#include "copied_from_larsoft_minor_edits/Wire.h"
#include "find_hits_with_gaussians_design1.hpp"

using namespace phlex;

PHLEX_REGISTER_ALGORITHMS(m, config)
{
  auto const layer_unfold_input = config.get<std::string>("layer_unfold_input");
  auto const layer_unfold_output = config.get<std::string>("layer_unfold_output");

  m.unfold<examples::unfold_wire_vector_design1>("unfold_wire_vector_design1",
                                                 &examples::unfold_wire_vector_design1::predicate,
                                                 &examples::unfold_wire_vector_design1::unfold,
                                                 layer_unfold_output,
                                                 concurrency::unlimited)
    .input_family(product_selector{.creator = "wires", .layer = layer_unfold_input, .suffix = ""});

  examples::find_hits_with_gaussians_design1_cfg cfg = {
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

  std::vector<std::shared_ptr<examples::CandHitStandard>> cand_hit_standard_vec;

  auto finder_configs = config.get<configuration>("cand_hit_standard_configs");
  cand_hit_standard_vec.resize(finder_configs.keys().size());
  for (auto const& key : finder_configs.keys()) {
    auto finder_config = finder_configs.get<configuration>(key);
    auto hit_finder = std::make_shared<examples::CandHitStandard>(
        examples::CandHitStandardCfg{
          .fRoiThreshold = finder_config.get<float>("roiThreshold")
        });
    unsigned int plane = finder_config.get<unsigned int>("Plane");
    if (plane >= cand_hit_standard_vec.size()) {
      std::cerr << "Error: plane number " << plane << " is out of range for cand_hit_standard_vec of size " << cand_hit_standard_vec.size() << std::endl;
      throw std::runtime_error("Invalid plane number");
    }
    cand_hit_standard_vec[plane] = std::move(hit_finder);
  }

  std::shared_ptr<examples::PeakFitterMrqdt> peak_fitter_mrqdt;
  auto fitter_config = config.get<configuration>("peak_fitter_mrqdt_config");
  peak_fitter_mrqdt = std::make_shared<examples::PeakFitterMrqdt>(
      examples::PeakFitterMrqdtCfg{
          .fMinWidth = fitter_config.get<double>("min_width"),
          .fMaxWidthMult = fitter_config.get<double>("max_width_mult"),
          .fPeakRange = fitter_config.get<double>("peak_range_fact"),
          .fAmpRange = fitter_config.get<double>("peak_amp_range")
  });


  std::shared_ptr<examples::HitFilterAlg> hit_filter_alg;
  auto filter_config = config.get<configuration>("hit_filter_alg_config");
  hit_filter_alg = std::make_shared<examples::HitFilterAlg>(
      examples::HitFilterAlgCfg{
          .fMinPulseHeight = filter_config.get<std::vector<float>>("min_pulse_height"),
          .fMinPulseSigma = filter_config.get<std::vector<float>>("min_pulse_sigma")
  });

  m.transform("find_hits_with_gaussians_design1",
              [cfg = std::move(cfg),
               cand_hit_standard_vec = std::move(cand_hit_standard_vec),
               peak_fitter_mrqdt = std::move(peak_fitter_mrqdt),
               hit_filter_alg = std::move(hit_filter_alg)]
              (recob::Wire const& wire) {
                 return examples::find_hits_with_gaussians_design1(cfg,
                                                                   wire,
                                                                   cand_hit_standard_vec,
                                                                   *peak_fitter_mrqdt,
                                                                   *hit_filter_alg);
             },
              concurrency::unlimited)
    .input_family(product_selector{.creator = "unfold_wire_vector_design1", .layer = layer_unfold_output});

  m.fold("fold_hits_into_vector_design1", examples::fold_hits_into_vector_design1, concurrency::serial, layer_unfold_input)
    .input_family(product_selector{.creator = "find_hits_with_gaussians_design1", .layer = layer_unfold_output});
}
