// Design4: registration of the cand_hit_derivative transform as
// a separate module.  This can be replaced with an alternative
// candidate-hit-finding implementation by registering a different
// module that produces merge_hit_candidate_vec from wire_roi_data.
//
// See README.md for some general comments about this example.

#include <cstddef>
#include <string>

#include "phlex/concurrency.hpp"
#include "phlex/configuration.hpp"
#include "phlex/core/product_selector.hpp"
#include "phlex/module.hpp"

#include "cand_hit_derivative.hpp"

using namespace phlex;

PHLEX_REGISTER_ALGORITHMS(m, config)
{
  auto const roi_creator = config.get<std::string>("roi_creator");
  auto const roi_layer = config.get<std::string>("roi_layer");

  examples::cand_hit_derivative::cand_hit_derivative_cfg const cfg{
    .min_delta_ticks = config.get<int>("min_delta_ticks"),
    .min_delta_peaks = config.get<float>("min_delta_peaks"),
    .min_hit_height = config.get<float>("min_hit_height"),
    .num_intervening_ticks = config.get<std::size_t>("num_intervening_ticks")};

  m.transform(
     "cand_hit_derivative",
     [cfg](examples::wire_roi_data const& roi_data) {
       return examples::cand_hit_derivative::find_and_merge_hit_candidates(cfg, roi_data);
     },
     concurrency::unlimited)
    .input_family(product_selector{.creator = roi_creator, .layer = roi_layer});
}
