// See REAMDME.md for some general comments about this example.

#include <string>

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

  m.observe("print_hits_to_file", examples::print_hits_to_file, concurrency::unlimited)
    .input_family(product_selector{.creator = "cell_info", .layer = layer},
                  product_selector{.creator = creator, .layer = layer});
}
