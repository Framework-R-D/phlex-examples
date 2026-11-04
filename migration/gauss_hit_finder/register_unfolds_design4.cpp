// Design4: registration of both unfold algorithms.
//
// See README.md for some general comments about this example.

#include <string>

#include "phlex/concurrency.hpp"
#include "phlex/configuration.hpp"
#include "phlex/core/product_selector.hpp"
#include "phlex/module.hpp"

#include "find_hits_with_gaussians_design4.hpp"

using namespace phlex;

PHLEX_REGISTER_ALGORITHMS(m, config)
{
  auto const vector_of_wires_layer = config.get<std::string>("vector_of_wires_layer");
  auto const wire_layer = config.get<std::string>("wire_layer");
  auto const roi_layer = config.get<std::string>("roi_layer");

  // ---------------------------------------------------------------
  // Outer unfold:  spill -> wire
  // ---------------------------------------------------------------
  m.unfold<examples::unfold_wire_vector_design4>("unfold_wire_vector_design4",
                                                 &examples::unfold_wire_vector_design4::predicate,
                                                 &examples::unfold_wire_vector_design4::unfold,
                                                 wire_layer,
                                                 concurrency::unlimited)
    .input_family(
      product_selector{.creator = "wires", .layer = vector_of_wires_layer, .suffix = ""});

  // ---------------------------------------------------------------
  // Inner unfold:  wire -> roi
  // ---------------------------------------------------------------
  m.unfold<examples::unfold_wire_design4>("unfold_wire_design4",
                                          &examples::unfold_wire_design4::predicate,
                                          &examples::unfold_wire_design4::unfold,
                                          roi_layer,
                                          concurrency::unlimited)
    .input_family(product_selector{.creator = "unfold_wire_vector_design4", .layer = wire_layer});
}
