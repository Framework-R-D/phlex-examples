#include <algorithm>
#include <iterator>

#include "cand_hit_standard.hpp"
#include "copied_from_larsoft_minor_edits/sparse_vector.h"

namespace examples {

  merge_hit_candidate_vec
  cand_hit_standard::find_and_merge_hit_candidates(
      wire_roi_data const& roi_data,
      std::vector<float> const& roi_thresholds) {
    hit_candidate_vec hit_candidates;

    find_hit_candidates(roi_data.range.begin(),
                        roi_data.range.end(),
                        0,
                        roi_thresholds.at(roi_data.plane),
                        hit_candidates);

    merge_hit_candidate_vec merged_hit_candidates;

    // If no hits then nothing to do here
    if (hit_candidates.empty()) return merged_hit_candidates;

    // The idea is to group hits that "touch" so they can be part of common fit, those that
    // don't "touch" are fit independently. So here we build the output vector to achieve that
    hit_candidate_vec grouped_hit_candidates;
    int last_tick = hit_candidates.front().stop_tick;

    // Step through the input hit candidates and group them by proximity
    for (const auto& hit_candidate : hit_candidates) {
      // Check condition that we have a new grouping
      if (int(hit_candidate.start_tick) - last_tick > 1) {
        merged_hit_candidates.emplace_back(grouped_hit_candidates);
        grouped_hit_candidates.clear();
      }

      // Add the current hit to the current group
      grouped_hit_candidates.emplace_back(hit_candidate);
      last_tick = hit_candidate.stop_tick;
    }

    // Check end condition
    if (!grouped_hit_candidates.empty()) merged_hit_candidates.emplace_back(grouped_hit_candidates);

    return merged_hit_candidates;
  }

  void cand_hit_standard::find_hit_candidates(std::vector<float>::const_iterator start,
                           std::vector<float>::const_iterator stop,
                           const std::size_t roi_start_tick,
                           const float roi_threshold,
                           hit_candidate_vec& hit_candidates) {
    // Need a minimum number of ticks to do any work here
    if (std::distance(start, stop) > 4) {
      // Find the highest peak in the range given
      auto max = std::max_element(start, stop);

      float max_value = *max;
      int max_time = std::distance(start, max);

      if (max_value > roi_threshold) {
        // backwards to find first bin for this candidate hit
        auto first = std::distance(start, max) > 2 ? max - 1 : start;

        while (first != start) {
          // Check for pathology where waveform goes too negative
          if (*first < -roi_threshold) break;

          // Check both sides of first and look for min/inflection point
          if (*first < *(first + 1) && *first <= *(first - 1)) break;

          --first;
        }

        int first_time = std::distance(start, first);

        // Recursive call to find all candidate hits earlier than this peak
        find_hit_candidates(start, first + 1, roi_start_tick, roi_threshold, hit_candidates);

        // forwards to find last bin for this candidate hit
        auto last = std::distance(max, stop) > 2 ? max + 1 : stop - 1;

        while (last != stop - 1) {
          // Check for pathology where value goes too negative
          if (*last < -roi_threshold) break;

          // Check both sides of last and look for min/inflection point
          if (*last <= *(last + 1) && *last < *(last - 1)) break;

          ++last;
        }

        int last_time = std::distance(start, last);

        // Now save this candidate's start and max time info
        hit_candidate new_hit_candidate;
        new_hit_candidate.start_tick = roi_start_tick + first_time;
        new_hit_candidate.stop_tick = roi_start_tick + last_time;
        new_hit_candidate.max_tick = roi_start_tick + first_time;
        new_hit_candidate.min_tick = roi_start_tick + last_time;
        new_hit_candidate.max_derivative = *(start + first_time);
        new_hit_candidate.min_derivative = *(start + last_time);
        new_hit_candidate.hit_center = roi_start_tick + max_time;
        new_hit_candidate.hit_sigma = std::max(2., float(last_time - first_time) / 6.);
        new_hit_candidate.hit_height = max_value;

        hit_candidates.push_back(new_hit_candidate);

        // Recursive call to find all candidate hits later than this peak
        find_hit_candidates(last + 1,
                            stop,
                            roi_start_tick + std::distance(start, last + 1),
                            roi_threshold,
                            hit_candidates);
      }
    }
  }
}
