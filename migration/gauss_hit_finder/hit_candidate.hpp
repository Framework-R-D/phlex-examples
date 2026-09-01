///////////////////////////////////////////////////////////////////////
///
/// \file   hit_candidate.hpp
///
/// \brief  Definition of the hit_candidate struct and associated
///         type aliases used across the gauss hit finder code.
///
////////////////////////////////////////////////////////////////////////
#ifndef PHLEX_EXAMPLES_MIGRATION_GAUSS_HIT_FINDER_HIT_CANDIDATE_HPP
#define PHLEX_EXAMPLES_MIGRATION_GAUSS_HIT_FINDER_HIT_CANDIDATE_HPP

#include <cstddef>
#include <vector>

namespace examples {

  struct hit_candidate {
    std::size_t start_tick;
    std::size_t stop_tick;
    std::size_t max_tick;
    std::size_t min_tick;
    float max_derivative;
    float min_derivative;
    float hit_center;
    float hit_sigma;
    float hit_height;
  };

  using hit_candidate_vec = std::vector<hit_candidate>;
  using merge_hit_candidate_vec = std::vector<hit_candidate_vec>;
}

#endif // PHLEX_EXAMPLES_MIGRATION_GAUSS_HIT_FINDER_HIT_CANDIDATE_HPP
