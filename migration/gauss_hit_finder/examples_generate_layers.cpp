// This duplicates generate_layers.cpp in phlex.
// We do not yet have a way to set PHLEX_PLUGIN_PATH
// in CMakeLists.txt so that the test in phlex_examples
// can find the plugin in phlex without manually setting
// the environment variable. As soon as that becomes
// possible we should delete this file and use the one in
// phlex instead.

// ==============================================================================================
// See notes in plugins/layer_generator.hpp.  To configure the generate_layers driver to
// produce the tree shown in plugins/layer_generator.hpp, the user would configure the driver
// like (e.g.):
//
//   driver: {
//     plugin: "generate_layers",
//     layers: {
//       spill: { parent: "job", total: 16 },
//       CRU: { parent: "spill", total: 256 },
//       run: { parent: "job", total: 16},
//       APA: { parent: "run", total: 150, starting_number: 1 }
//     }
//   }
//
// Note that 'total' refers to the total number of data cells *per* parent.
// ==============================================================================================

#include "phlex/driver.hpp"
#include "plugins/layer_generator.hpp"

#include <memory>
#include <string>
#include <utility>

PHLEX_REGISTER_DRIVER(d, config)
{
  using namespace phlex;

  auto gen = experimental::layer_generator::make();

  auto const layers = config.get<configuration>("layers", {});
  for (auto const& key : layers.keys()) {
    auto const layer_config = layers.get<configuration>(key);
    gen->add_layer(key,
                   {.parent_layer_name = layer_config.get<std::string>("parent", "job"),
                    .total_per_parent_data_cell = layer_config.get<unsigned int>("total"),
                    .starting_value = layer_config.get<unsigned int>("starting_number", 0)});
  }

  return d.driver(std::move(gen));
}
