// Design4: registration of both fold algorithms.
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
  // Inner fold:  roi -> wire  (collects hits from ROIs of one wire)
  // ---------------------------------------------------------------
  m.fold("fold_roi_hits_design4", examples::fold_roi_hits_design4, concurrency::serial, wire_layer)
    .input_family(
      product_selector{.creator = "find_hits_with_gaussians_design4", .layer = roi_layer});

  // ---------------------------------------------------------------
  // Outer fold:  wire -> spill  (collects hits from all wires)
  // ---------------------------------------------------------------
  m.fold("fold_hits_into_vector_design4",
         examples::fold_hits_into_vector_design4,
         concurrency::serial,
         vector_of_wires_layer)
    .input_family(product_selector{.creator = "fold_roi_hits_design4", .layer = wire_layer});
}
