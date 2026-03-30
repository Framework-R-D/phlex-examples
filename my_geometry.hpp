#ifndef MY_GEOMETRY_HPP
#define MY_GEOMETRY_HPP

#include <string>
#include <utility>

namespace examples {
  class geometry {
  public:
    struct linear_bounds {
      double minimum;
      double maximum;
    };

    explicit geometry(std::string const& geometry_name);

    std::string const& name() const noexcept;

    linear_bounds x_bounds() const;
    linear_bounds y_bounds() const;
    linear_bounds z_bounds() const;

  private:
    std::string name_;
  };
}

#endif // MY_GEOMETRY_HPP
