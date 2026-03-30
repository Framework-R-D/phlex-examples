#include "my_add.hpp"
#include "my_geometry.hpp"

#include "phlex/module.hpp"

#include <cassert>

using namespace phlex;

PHLEX_REGISTER_ALGORITHMS(m, config)
{
  auto const layer = config.get<std::string>("layer");

  m.transform("add", examples::add, concurrency::unlimited)
    .input_family(product_query{.creator = "input", .layer = layer, .suffix = "i"},
                  product_query{.creator = "input", .layer = layer, .suffix = "j"})
    .output_product_suffixes("sum");
  m.observe(
     "verify",
     [](examples::geometry const& geom, int const sum) {
       auto const [minimum, maximum] = geom.x_bounds();
       assert(minimum < sum);
       assert(sum < maximum);
       assert(sum == 0);
     },
     concurrency::unlimited)
    .input_family(product_query{.creator = "input", .layer = "job"},
                  product_query{.creator = "add", .layer = layer});
}
