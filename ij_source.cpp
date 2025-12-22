#include "phlex/model/data_cell_index.hpp"
#include "phlex/source.hpp"

#include <cassert>

using namespace phlex;

PHLEX_REGISTER_PROVIDERS(m)
{
  m.provide("provide_i", [](data_cell_index const& id) -> int { return id.number(); })
    .output_product("i"_in("event"));
  m.provide("provide_j", [](data_cell_index const& id) -> int { return -id.number(); })
    .output_product("j"_in("event"));
}
