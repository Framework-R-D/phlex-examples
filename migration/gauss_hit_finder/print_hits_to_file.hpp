#ifndef MIGRATION_GAUSS_HIT_FINDER_PRINT_HITS_TO_FILE_HPP
#define MIGRATION_GAUSS_HIT_FINDER_PRINT_HITS_TO_FILE_HPP

#include <string>
#include <vector>

#include "copied_from_larsoft_minor_edits/Hit.h"

namespace examples {

  void print_hits_to_file(std::string const& filename_prefix,
                          int cell_id,
                          std::vector<recob::Hit> const& input_hits);
}

#endif // MIGRATION_GAUSS_HIT_FINDER_PRINT_HITS_TO_FILE_HPP
