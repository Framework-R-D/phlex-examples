#include "phlex/configuration.hpp"
#include "phlex/model/data_cell_index.hpp"
#include "phlex/source.hpp"

using namespace phlex;

PHLEX_REGISTER_PROVIDERS(m, config)
{
  auto const layer = config.get<std::string>("layer");

  m.provide("provide_i", [](data_cell_index const& id) -> int { return id.number(); })
    .output_product({.creator = "input", .layer = layer, .suffix = "i"});
  m.provide("provide_j", [](data_cell_index const& id) -> int { return -id.number(); })
    .output_product({.creator = "input", .layer = layer, .suffix = "j"});
}
