#include "phlex/configuration.hpp"
#include "phlex/model/data_cell_index.hpp"
#include "phlex/source.hpp"

#include <vector>

using namespace phlex;

PHLEX_REGISTER_PROVIDERS(m, config)
{
  auto const layer = config.get<std::string>("layer");

  m.provide("provide_numbers",
            [](data_cell_index const& id) -> std::vector<int> {
              auto const n = static_cast<int>(id.number());
              return {n, n + 1, n + 2};
            })
    .output_product("input", "numbers", experimental::identifier{layer});
}
