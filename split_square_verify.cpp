#include "phlex/module.hpp"

#include <cassert>
#include <cstddef>
#include <utility>
#include <vector>

// Splitter for an unfold. The framework constructs one per parent cell from
// the input product (the constructor signature is inspected to deduce the
// expected number and types of input products), then iterates by threading
// an opaque state value:
//
//   initial_value()  -> first state
//   predicate(state) -> true while more children remain
//   unfold(state)    -> { next state, emitted child product }
class VectorSplitter {
public:
  explicit VectorSplitter(std::vector<int> values) : values_{std::move(values)} {}

  std::size_t initial_value() const { return 0; }
  bool predicate(std::size_t i) const { return i < values_.size(); }
  std::pair<std::size_t, int> unfold(std::size_t i) const { return {i + 1, values_[i]}; }

private:
  std::vector<int> values_;
};

PHLEX_REGISTER_ALGORITHMS(m, config)
{
  using namespace phlex;

  auto const parent_layer = config.get<std::string>("parent_layer");
  auto const child_layer = config.get<std::string>("child_layer");

  m.unfold<VectorSplitter>("split",
                           &VectorSplitter::predicate,
                           &VectorSplitter::unfold,
                           child_layer,
                           concurrency::unlimited)
    .input_family(product_selector{.creator = "input", .layer = parent_layer, .suffix = "numbers"})
    .output_product_suffixes("number");

  m.transform(
     "square", [](int n) { return n * n; }, concurrency::unlimited)
    .input_family(product_selector{.creator = "split", .layer = child_layer, .suffix = "number"})
    .output_product_suffixes("squared");

  m.observe(
     "verify", [](int sq) { assert(sq >= 0); }, concurrency::unlimited)
    .input_family(product_selector{.creator = "square", .layer = child_layer, .suffix = "squared"});
}
