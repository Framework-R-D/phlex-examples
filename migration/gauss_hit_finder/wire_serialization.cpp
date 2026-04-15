#include <algorithm>
#include <stdexcept>
#include "copied_from_larsoft_minor_edits/sparse_vector.h"
#include "wire_serialization.hpp"

bool write_wires_to_file(std::vector<recob::Wire> const& wires,
                         std::string const& filename) {
    std::ofstream out(filename, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Failed to open file for writing: " + filename);
    }

    // Write number of wires
    size_t num_wires = wires.size();
    out.write(reinterpret_cast<const char*>(&num_wires), sizeof(num_wires));

    for (const auto& wire : wires) {
        // Write channel ID
        raw::ChannelID_t channel = wire.Channel();
        out.write(reinterpret_cast<const char*>(&channel), sizeof(channel));

        // Write view
        geo::View_t view = wire.View();
        out.write(reinterpret_cast<const char*>(&view), sizeof(view));

        // Write sparse vector data
        const auto& signal_roi = wire.SignalROI();
        size_t nominal_size = signal_roi.size();
        out.write(reinterpret_cast<const char*>(&nominal_size), sizeof(nominal_size));

        // Write number of ranges
        size_t num_ranges = std::distance(signal_roi.begin_range(), signal_roi.end_range());
        out.write(reinterpret_cast<const char*>(&num_ranges), sizeof(num_ranges));

        // Write each range
        for (auto range_it = signal_roi.begin_range(); range_it != signal_roi.end_range(); ++range_it) {
            // Write range offset and last
            size_t offset = range_it->begin_index();
            size_t last = range_it->end_index();
            out.write(reinterpret_cast<const char*>(&offset), sizeof(offset));
            out.write(reinterpret_cast<const char*>(&last), sizeof(last));

            // Write number of values
            size_t num_values = range_it->size();
            out.write(reinterpret_cast<const char*>(&num_values), sizeof(num_values));

            // Write values
            if (num_values > 0) {
                // Create a temporary vector to hold the values
                std::vector<float> values(range_it->begin(), range_it->end());
                out.write(reinterpret_cast<const char*>(values.data()),
                         num_values * sizeof(float));
            }
        }
    }
    out.close();
    return out.good();
}

std::optional<std::vector<recob::Wire>> read_wires_from_file(std::string const& filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Failed to open file for reading: " + filename);
    }

    std::vector<recob::Wire> wires;

    // Read number of wires
    size_t num_wires;
    in.read(reinterpret_cast<char*>(&num_wires), sizeof(num_wires));
    if (!in) return std::nullopt;

    wires.reserve(num_wires);

    for (size_t i = 0; i < num_wires; ++i) {
        // Read channel ID
        raw::ChannelID_t channel;
        in.read(reinterpret_cast<char*>(&channel), sizeof(channel));
        if (!in) return std::nullopt;

        // Read view
        geo::View_t view;
        in.read(reinterpret_cast<char*>(&view), sizeof(view));
        if (!in) return std::nullopt;

        // Read sparse vector data
        size_t nominal_size;
        in.read(reinterpret_cast<char*>(&nominal_size), sizeof(nominal_size));
        if (!in) return std::nullopt;

        // Read number of ranges
        size_t num_ranges;
        in.read(reinterpret_cast<char*>(&num_ranges), sizeof(num_ranges));
        if (!in) return std::nullopt;

        // Create sparse vector
        lar::sparse_vector<float> signal_roi;
        signal_roi.resize(nominal_size);

        // Read each range
        for (size_t j = 0; j < num_ranges; ++j) {
            // Read range offset and last
            size_t offset, last;
            in.read(reinterpret_cast<char*>(&offset), sizeof(offset));
            in.read(reinterpret_cast<char*>(&last), sizeof(last));
            if (!in) return std::nullopt;

            // Read number of values
            size_t num_values;
            in.read(reinterpret_cast<char*>(&num_values), sizeof(num_values));
            if (!in) return std::nullopt;

            // Read values
            std::vector<float> values(num_values);
            if (num_values > 0) {
                in.read(reinterpret_cast<char*>(values.data()),
                       num_values * sizeof(float));
                if (!in) return std::nullopt;
            }

            // Add range to sparse vector
            signal_roi.add_range(offset, values);
        }

        // Create wire
        wires.emplace_back(std::move(signal_roi), channel, view);
    }
    return wires;
}
