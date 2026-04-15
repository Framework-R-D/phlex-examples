#ifndef PHLEX_EXAMPLES_WIRE_SERIALIZATION_H
#define PHLEX_EXAMPLES_WIRE_SERIALIZATION_H

#include <fstream>
#include <optional>
#include <string>
#include <vector>
#include "copied_from_larsoft_minor_edits/Wire.h"

bool write_wires_to_file(std::vector<recob::Wire> const& wires,
                         std::string const& filename);

std::optional<std::vector<recob::Wire>> read_wires_from_file(std::string const& filename);

#endif // PHLEX_EXAMPLES_WIRE_SERIALIZATION_H
