////////////////////////////////////////////////////////////////////////
/// \file   cand_hit_derivative.cpp
/// \author T. Usher
////////////////////////////////////////////////////////////////////////

#include "cand_hit_derivative.hpp"
#include "waveform_helper.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <utility>

namespace examples {

  namespace {

    // Finding the nearest minimum from current point (searching forward)
    std::vector<float>::const_iterator find_nearest_min(std::vector<float>::const_iterator max_itr,
                                                        std::vector<float>::const_iterator stop_itr)
    {
      auto last_itr = max_itr;

      // We are at a maximum so we search forward until we find the lowest negative point
      while ((last_itr + 1) != stop_itr) {
        if (*(last_itr + 1) > *last_itr)
          break;
        last_itr++;
      }

      return last_itr;
    }

    // Finding the nearest maximum from current point (searching backward)
    std::vector<float>::const_iterator find_nearest_max(
      std::vector<float>::const_iterator min_itr, std::vector<float>::const_iterator start_itr)
    {
      auto last_itr = min_itr;

      if (std::distance(start_itr, min_itr) > 0) {
        // Loop backward over ticks looking for the waveform to start decreasing
        while ((last_itr - 1) != start_itr) {
          if (*(last_itr - 1) < *last_itr)
            break;
          last_itr--;
        }
      }

      return last_itr;
    }

    // Find the "start" of a candidate hit by searching backward for a zero crossing
    std::vector<float>::const_iterator find_start_tick(std::vector<float>::const_iterator max_itr,
                                                       std::vector<float>::const_iterator start_itr)
    {
      auto last_itr = max_itr;

      if (std::distance(start_itr, last_itr) > 0) {
        // Starting at a maximum, search backward to find where the derivative waveform
        // crosses zero again. Watch for the case where two peaks are merged together.
        auto loop_itr = last_itr - 1;

        while (loop_itr != start_itr) {
          // Cross zero, encounter a minimum, or an inflection point
          if (*loop_itr < 0. || !(*loop_itr < *last_itr))
            break;
          last_itr = loop_itr--;
        }
      } else {
        last_itr = start_itr;
      }

      return last_itr;
    }

    // Find the "stop" of a candidate hit by searching forward for a zero crossing
    std::vector<float>::const_iterator find_stop_tick(std::vector<float>::const_iterator min_itr,
                                                      std::vector<float>::const_iterator stop_itr)
    {
      auto last_itr = min_itr;

      if (std::distance(min_itr, stop_itr) > 1) {
        auto loop_itr = last_itr + 1;

        while (loop_itr != stop_itr) {
          // Crossed zero coming from a minimum, or encountered a maximum
          if (*loop_itr > 0. || !(*loop_itr > *last_itr))
            break;
          last_itr = loop_itr++;
        }
      }

      return last_itr;
    }

  } // anonymous namespace

  namespace cand_hit_derivative {

    merge_hit_candidate_vec find_and_merge_hit_candidates(cand_hit_derivative_cfg const& cfg,
                                                          wire_roi_data const& roi_data)
    {
      // Recover the actual waveform from the datarange
      auto const& waveform = roi_data.range.data();

      // Compute the derivative of the input waveform, then smooth it
      auto raw_derivative = waveform_helper::first_derivatives(waveform);
      auto derivative = waveform_helper::triangle_smooth(raw_derivative);

      // Find hit candidates in the derivative waveform
      hit_candidate_vec hit_candidates;

      find_hit_candidates(cfg,
                          derivative.begin(),
                          derivative.end(),
                          0,
                          cfg.min_delta_ticks,
                          cfg.min_delta_peaks,
                          hit_candidates);

      // Reset the hit height from the input waveform
      for (auto& hc : hit_candidates) {
        std::size_t center_idx = hc.hit_center;
        hc.hit_height = waveform.at(center_idx);
      }

      merge_hit_candidate_vec merged_hit_candidates;

      // If no hits then nothing to do
      if (hit_candidates.empty())
        return merged_hit_candidates;

      // Group hits that "touch" so they can be part of a common fit
      hit_candidate_vec grouped_hit_candidates;
      std::size_t last_stop_tick = hit_candidates.front().stop_tick;

      for (auto const& hc : hit_candidates) {
        // Small pulse height hits should not be considered
        if (hc.hit_height > cfg.min_hit_height) {
          // Check condition that we have a new grouping
          if (hc.start_tick > last_stop_tick + cfg.num_intervening_ticks &&
              !grouped_hit_candidates.empty()) {
            merged_hit_candidates.emplace_back(grouped_hit_candidates);
            grouped_hit_candidates.clear();
          }

          grouped_hit_candidates.emplace_back(hc);
          last_stop_tick = hc.stop_tick;
        }
      }

      // Check end condition
      if (!grouped_hit_candidates.empty())
        merged_hit_candidates.emplace_back(grouped_hit_candidates);

      return merged_hit_candidates;
    }

    void find_hit_candidates(cand_hit_derivative_cfg const& cfg,
                             std::vector<float>::const_iterator start,
                             std::vector<float>::const_iterator stop,
                             std::size_t const roi_start_tick,
                             int delta_ticks_threshold,
                             float delta_peak_threshold,
                             hit_candidate_vec& hit_candidates)
    {
      // Search for candidate hits...
      // Find the largest deviation in the input derivative waveform as the starting point.
      // Depending on if a maximum or minimum, search forward or backward to find the
      // corresponding minimum or maximum.
      auto [min_itr, max_itr] = std::minmax_element(start, stop);

      // Use the larger of the two as the starting point and recover the nearest max or min
      if (std::fabs(*max_itr) > std::fabs(*min_itr))
        min_itr = find_nearest_min(max_itr, stop);
      else
        max_itr = find_nearest_max(min_itr, start);

      int delta_ticks = std::distance(max_itr, min_itr);
      float range = *max_itr - *min_itr;

      // At some point small rolling oscillations on the waveform need to be ignored...
      if (delta_ticks >= delta_ticks_threshold && range > delta_peak_threshold) {
        // Back up to find zero crossing — starting point of candidate hit
        // and endpoint of the pre sub-waveform to search next
        auto new_end_itr = find_start_tick(max_itr, start);
        int start_tick = std::distance(start, new_end_itr);

        // Go forward to find zero crossing — end point of candidate hit
        // and starting point for the post sub-waveform to search
        auto new_start_itr = find_stop_tick(min_itr, stop);
        int stop_tick = std::distance(start, new_start_itr);

        // Find hits in the section of the waveform leading up to this candidate hit
        if (start_tick > delta_ticks_threshold) {
          // Special handling for merged hits
          if (*(new_end_itr - 1) > 0.) {
            delta_ticks_threshold = 2;
            delta_peak_threshold = 0.;
          } else {
            delta_ticks_threshold = cfg.min_delta_ticks;
            delta_peak_threshold = cfg.min_delta_peaks;
          }

          find_hit_candidates(cfg,
                              start,
                              new_end_itr + 1,
                              roi_start_tick,
                              delta_ticks_threshold,
                              delta_peak_threshold,
                              hit_candidates);
        }

        // Create a new hit candidate and store away
        hit_candidate new_hit_candidate;

        auto peak_itr = std::min_element(max_itr, min_itr, [](auto const& left, auto const& right) {
          return std::fabs(left) < std::fabs(right);
        });

        // Check balance
        if (2 * std::distance(peak_itr, min_itr) < std::distance(max_itr, peak_itr))
          peak_itr--;
        else if (2 * std::distance(max_itr, peak_itr) < std::distance(peak_itr, min_itr))
          peak_itr++;

        new_hit_candidate.start_tick = roi_start_tick + start_tick;
        new_hit_candidate.stop_tick = roi_start_tick + stop_tick;
        new_hit_candidate.max_tick = roi_start_tick + std::distance(start, max_itr);
        new_hit_candidate.min_tick = roi_start_tick + std::distance(start, min_itr);
        new_hit_candidate.max_derivative = max_itr != stop ? *max_itr : 0.;
        new_hit_candidate.min_derivative = min_itr != stop ? *min_itr : 0.;
        new_hit_candidate.hit_center = roi_start_tick + std::distance(start, peak_itr) + 0.5;
        new_hit_candidate.hit_sigma =
          0.5 * float(new_hit_candidate.min_tick - new_hit_candidate.max_tick);
        new_hit_candidate.hit_height =
          new_hit_candidate.hit_sigma *
          (new_hit_candidate.max_derivative - new_hit_candidate.min_derivative) / 1.2130;

        hit_candidates.push_back(new_hit_candidate);

        // Search the section of the waveform following this candidate for more hits
        if (std::distance(new_start_itr, stop) > delta_ticks_threshold) {
          // Special handling for merged hits
          if (*(new_start_itr + 1) < 0.) {
            delta_ticks_threshold = 2;
            delta_peak_threshold = 0.;
          } else {
            delta_ticks_threshold = cfg.min_delta_ticks;
            delta_peak_threshold = cfg.min_delta_peaks;
          }

          find_hit_candidates(cfg,
                              new_start_itr,
                              stop,
                              roi_start_tick + stop_tick,
                              delta_ticks_threshold,
                              delta_peak_threshold,
                              hit_candidates);
        }
      }
    }

  } // namespace cand_hit_derivative

} // namespace examples
