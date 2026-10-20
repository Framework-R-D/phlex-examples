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
  auto const layer_vector_of_wires = config.get<std::string>("layer_vector_of_wires");
  auto const layer_wire = config.get<std::string>("layer_wire");
  auto const layer_roi = config.get<std::string>("layer_roi");

  // ---------------------------------------------------------------
  // Outer unfold:  spill -> wire
  // ---------------------------------------------------------------
  m.unfold<examples::unfold_wire_vector_design4>("unfold_wire_vector_design4",
                                                 &examples::unfold_wire_vector_design4::predicate,
                                                 &examples::unfold_wire_vector_design4::unfold,
                                                 layer_wire,
                                                 concurrency::unlimited)
    .input_family(
      product_selector{.creator = "wires", .layer = layer_vector_of_wires, .suffix = ""});

  // ---------------------------------------------------------------
  // Inner unfold:  wire -> roi
  // ---------------------------------------------------------------
  m.unfold<examples::unfold_wire_design4>("unfold_wire_design4",
                                          &examples::unfold_wire_design4::predicate,
                                          &examples::unfold_wire_design4::unfold,
                                          layer_roi,
                                          concurrency::unlimited)
    .input_family(product_selector{.creator = "unfold_wire_vector_design4", .layer = layer_wire});
}
