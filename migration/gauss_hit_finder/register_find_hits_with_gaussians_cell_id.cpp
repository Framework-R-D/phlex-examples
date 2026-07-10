#include <string>

#include "phlex/configuration.hpp"
#include "phlex/model/data_cell_index.hpp"
#include "phlex/source.hpp"

using namespace phlex;

PHLEX_REGISTER_PROVIDERS(m, config)
{
  auto const layer = config.get<std::string>("layer");

  m.provide("cell_id", [](data_cell_index const& id) -> int { return id.number(); })
    .output_product("cell_info", "", experimental::identifier{layer});
}
