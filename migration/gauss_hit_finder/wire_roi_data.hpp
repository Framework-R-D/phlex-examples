#ifndef PHLEX_EXAMPLES_MIGRATION_GAUSS_HIT_FINDER_WIRE_ROI_DATA_HPP
#define PHLEX_EXAMPLES_MIGRATION_GAUSS_HIT_FINDER_WIRE_ROI_DATA_HPP

#include "copied_from_larsoft_minor_edits/geo_types.h"
#include "copied_from_larsoft_minor_edits/RawTypes.h" // raw::ChannelID_t
#include "copied_from_larsoft_minor_edits/sparse_vector.h"
#include "copied_from_larsoft_minor_edits/Wire.h"

namespace examples {

  // ---------------------------------------------------------------
  // A lightweight struct that bundles data for a single region
  // of interest (roi) in an object of type datarange_t
  // together with the wire-level context needed by the transforms
  // that create the recob::Hit objects.
  // ---------------------------------------------------------------
  struct wire_roi_data {
    recob::Wire::RegionsOfInterest_t::datarange_t range;
    raw::ChannelID_t channel;
    // Currently it is not possible to pass an enum between
    // phlex nodes, so we use int as the type for now instead
    // of geo::View_t.
    int view;
    geo::PlaneID::PlaneID_t plane;
  };

} // namespace examples

#endif // PHLEX_EXAMPLES_MIGRATION_GAUSS_HIT_FINDER_WIRE_ROI_DATA_HPP
