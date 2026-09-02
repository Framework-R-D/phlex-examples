#include "waveform_helper.hpp"

#include <algorithm>

namespace examples {

  namespace waveform_helper {

    template <std::floating_point T>
    std::vector<T> first_derivatives(std::vector<T> const& input)
    {
      std::vector<T> derivatives;
      derivatives.resize(input.size(), 0.);

      for (std::size_t idx = 1; idx < derivatives.size() - 1; ++idx) {
        derivatives[idx] = 0.5 * (input[idx + 1] - input[idx - 1]);
      }
      return derivatives;
    }

    template <std::floating_point T>
    std::vector<T> triangle_smooth(std::vector<T> const& input, std::size_t lowest_bin)
    {
      std::vector<T> smoothed;
      smoothed.resize(input.size());

      // Watch for edge condition
      if (input.size() > 4) {
        std::copy(input.begin(), input.begin() + 2 + lowest_bin, smoothed.begin());
        std::copy(input.end() - 2, input.end(), smoothed.end() - 2);

        auto cur_out = smoothed.begin() + 2 + lowest_bin;
        auto cur_in = input.begin() + 1 + lowest_bin;
        auto const stop_in = input.end() - 3;

        while (cur_in++ != stop_in) {
          // Take the weighted average of five consecutive points centered on current point
          T new_val = (*(cur_in - 2) + 2. * *(cur_in - 1) + 3. * *cur_in + 2. * *(cur_in + 1) +
                       *(cur_in + 2)) /
                      9.;
          *cur_out++ = new_val;
        }
      } else {
        std::copy(input.begin(), input.end(), smoothed.begin());
      }
      return smoothed;
    }

    // Explicit instantiations
    template std::vector<float> first_derivatives<float>(std::vector<float> const&);
    template std::vector<double> first_derivatives<double>(std::vector<double> const&);

    template std::vector<float> triangle_smooth<float>(std::vector<float> const&, std::size_t);
    template std::vector<double> triangle_smooth<double>(std::vector<double> const&, std::size_t);

  } // namespace waveform_helper

} // namespace examples
