#ifndef MIGRATION_GAUSS_HIT_FINDER_WAVEFORM_HELPER_HPP
#define MIGRATION_GAUSS_HIT_FINDER_WAVEFORM_HELPER_HPP

////////////////////////////////////////////////////////////////////////
/// \file   waveform_helper.hpp
///
/// \brief  This is the interface class for tools/algorithms that
///         perform various operations on waveforms. Examples include
///         smoothing, differentiation, filtering, etc.
///
/// \author T. Usher
////////////////////////////////////////////////////////////////////////

#include <concepts>
#include <cstddef>
#include <vector>

namespace examples {

  namespace waveform_helper {

    template <std::floating_point T>
    std::vector<T> first_derivatives(std::vector<T> const& input);

    template <std::floating_point T>
    std::vector<T> triangle_smooth(std::vector<T> const& input, std::size_t lowest_bin = 0);

  } // namespace waveform_helper

} // namespace examples
#endif // MIGRATION_GAUSS_HIT_FINDER_WAVEFORM_HELPER_HPP
