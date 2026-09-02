// Design4: registration of the cand_hit_standard transform as
// a separate module.  This can be replaced with an alternative
// candidate-hit-finding implementation by registering a different
// module that produces merge_hit_candidate_vec from wire_roi_data.
//
// See README.md for some general comments about this example.

#include <string>
#include <vector>

#include "phlex/concurrency.hpp"
#include "phlex/configuration.hpp"
#include "phlex/core/product_selector.hpp"
#include "phlex/module.hpp"

#include "cand_hit_standard.hpp"

using namespace phlex;

PHLEX_REGISTER_ALGORITHMS(m, config)
{
  auto const roi_creator = config.get<std::string>("roi_creator");
  auto const roi_layer = config.get<std::string>("roi_layer");

  m.transform(
     "cand_hit_standard",
     [roi_threshold =
        config.get<std::vector<float>>("roi_threshold")](examples::wire_roi_data const& roi_data) {
       return examples::cand_hit_standard::find_and_merge_hit_candidates(roi_data, roi_threshold);
     },
     concurrency::unlimited)
    .input_family(product_selector{.creator = roi_creator, .layer = roi_layer});
}
