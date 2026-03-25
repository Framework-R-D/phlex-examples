#include "phlex/module.hpp"
#include "my_add.hpp"

#include <cassert>

using namespace phlex;

PHLEX_REGISTER_ALGORITHMS(m, config)
{
  auto const layer = config.get<std::string>("layer");

  m.transform("add", examples::add, concurrency::unlimited)
    .input_family(product_query{.creator = "input", .suffix = "i", .layer = layer},
                  product_query{.creator = "input", .suffix = "j", .layer = layer})
    .output_product_suffixes("sum");
  m.observe(
     "verify", [](int actual) { assert(actual == 0); }, concurrency::unlimited)
    .input_family(product_query{.creator = "add", .suffix = "sum", .layer = layer});
}
