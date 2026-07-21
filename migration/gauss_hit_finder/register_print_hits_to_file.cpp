// See README.md for some general comments about this example.

#include <string>
#include <vector>

#include "copied_from_larsoft_minor_edits/Hit.h"

#include "phlex/concurrency.hpp"
#include "phlex/configuration.hpp"
#include "phlex/core/product_selector.hpp"
#include "phlex/module.hpp"

#include "print_hits_to_file.hpp"

using namespace phlex;

PHLEX_REGISTER_ALGORITHMS(m, config)
{
  auto const creator = config.get<std::string>("creator");
  auto const layer = config.get<std::string>("layer");
  auto const filename_prefix = config.get<std::string>("filename_prefix");

  m.observe("print_hits_to_file",
            [filename_prefix](int cell_id, std::vector<recob::Hit> const& hits) {
              examples::print_hits_to_file(filename_prefix, cell_id, hits);
            },
            concurrency::unlimited)
      .input_family(
          product_selector{.creator = "cell_info", .layer = layer},
          product_selector{.creator = creator, .layer = layer});
}
