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
  auto const layer_vector_of_wires = config.get<std::string>("layer_vector_of_wires");
  auto const layer_wire = config.get<std::string>("layer_wire");
  auto const layer_roi = config.get<std::string>("layer_roi");

  // ---------------------------------------------------------------
  // Inner fold:  roi -> wire  (collects hits from ROIs of one wire)
  // ---------------------------------------------------------------
  m.fold("fold_roi_hits_design4", examples::fold_roi_hits_design4, concurrency::serial, layer_wire)
    .input_family(
      product_selector{.creator = "find_hits_with_gaussians_design4", .layer = layer_roi});

  // ---------------------------------------------------------------
  // Outer fold:  wire -> spill  (collects hits from all wires)
  // ---------------------------------------------------------------
  m.fold("fold_hits_into_vector_design4",
         examples::fold_hits_into_vector_design4,
         concurrency::serial,
         layer_vector_of_wires)
    .input_family(product_selector{.creator = "fold_roi_hits_design4", .layer = layer_wire});
}
