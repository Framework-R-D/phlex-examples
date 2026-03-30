#include "my_geometry.hpp"

namespace examples {

  geometry::geometry(std::string const& geometry_name) : name_{geometry_name} {}

  std::string const& geometry::name() const noexcept { return name_; }

  auto geometry::x_bounds() const -> geometry::linear_bounds
  {
    return {.minimum = -20., .maximum = 20};
  }
  auto geometry::y_bounds() const -> geometry::linear_bounds
  {
    return {.minimum = -30., .maximum = 30};
  }
  auto geometry::z_bounds() const -> geometry::linear_bounds
  {
    return {.minimum = -100., .maximum = 100};
  }
}
