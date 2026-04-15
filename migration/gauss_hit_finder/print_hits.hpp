#ifndef PHLEX_EXAMPLES_PRINT_HITS_HPP
#define PHLEX_EXAMPLES_PRINT_HITS_HPP

#include <string>
#include <vector>
#include "copied_from_larsoft_minor_edits/Hit.h"

void print_hits_to_file(std::vector<recob::Hit> const& hits,
                        std::string const& filename);

#endif // PHLEX_EXAMPLES_PRINT_HITS_HPP
