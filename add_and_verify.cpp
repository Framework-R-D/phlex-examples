#include "my_add.hpp"
#include "my_geometry.hpp"

#include "phlex/module.hpp"

#include <cassert>

namespace {
  // An algorithm that uses a data product from the job and another from a different layer
  void verify_sum(examples::geometry const& geom, int const sum)
  {
    auto const [minimum, maximum] = geom.x_bounds();
    assert(minimum < sum);
    assert(sum < maximum);
    assert(sum == 0);
  }
}

PHLEX_REGISTER_ALGORITHMS(m, config)
{
  using namespace phlex;

  auto const layer = config.get<std::string>("layer");

  m.transform("add", examples::add, concurrency::unlimited)
    .input_family(product_selector{.creator = "input", .layer = layer, .suffix = "i"},
                  product_selector{.creator = "input", .layer = layer, .suffix = "j"})
    .output_product_suffixes("sum");

  m.observe("verify", verify_sum, concurrency::unlimited)
    .input_family(product_selector{.creator = "input", .layer = "job"},
                  product_selector{.creator = "add", .layer = layer});
}
