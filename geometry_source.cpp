#include "phlex/source.hpp"

#include "my_geometry.hpp"

PHLEX_REGISTER_PROVIDERS(s, config)
{
  auto geometry_name = config.get<std::string>("geometry_name");
  s.provide("provide_geometry",
            [geometry_name](phlex::data_cell_index const& /* job */) -> examples::geometry {
              return examples::geometry{geometry_name};
            })
    .output_product({.creator = "input", .layer = "job", .suffix = "geometry"});
}
