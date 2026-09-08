#ifndef MIGRATION_GAUSS_HIT_FINDER_WIRE_SERIALIZATION_HPP
#define MIGRATION_GAUSS_HIT_FINDER_WIRE_SERIALIZATION_HPP

#include "copied_from_larsoft_minor_edits/Wire.h"
#include <fstream>
#include <optional>
#include <string>
#include <vector>

bool write_wires_to_file(std::vector<recob::Wire> const& wires, std::string const& filename);

std::optional<std::vector<recob::Wire>> read_wires_from_file(std::string const& filename);

#endif // MIGRATION_GAUSS_HIT_FINDER_WIRE_SERIALIZATION_HPP
