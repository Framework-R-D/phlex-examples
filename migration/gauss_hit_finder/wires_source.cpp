#include <exception>
#include <string>
#include <vector>

#include "copied_from_larsoft_minor_edits/Wire.h"

#include "phlex/configuration.hpp"
#include "phlex/model/data_cell_index.hpp"
#include "phlex/source.hpp"
#include "wire_serialization.hpp"

using namespace phlex;

// This provider is for temporary testing purposes only. This
// is NOT intended to be a long term solution for persistence.
// The purpose of this provider is to read input for
// GausHitFinder from files created in an "art" framework
// job, without using the art framework or ROOT.
// One must run an art job that produces these data
// files before running the phlex job that uses this provider.

PHLEX_REGISTER_PROVIDERS(m, config)
{
  auto const layer = config.get<std::string>("layer");

  m.provide("provide_wires",
            [](data_cell_index const& id) -> std::vector<recob::Wire> {
              std::string filename = std::string("wires_") + std::to_string(id.number()) + ".dat";

              if (auto wires = read_wires_from_file(filename)) {
                return *wires;
              }
              throw std::runtime_error("Failure while reading from file: " + filename);
              return {};
            })
    .output_product("wires", "", experimental::identifier{layer});
}
