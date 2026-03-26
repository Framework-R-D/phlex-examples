/** ****************************************************************************
 * @file Wire.cxx
 * @brief Definition of basic channel signal object.
 * @author brebel@fnal.gov
 * @see  Wire.h
 *
 * ****************************************************************************/

#include "Wire.h"

// C/C++ standard libraries
#include <utility> // std::move()

namespace recob {

  //----------------------------------------------------------------------
  Wire::Wire() : fChannel(raw::InvalidChannelID), fView(geo::kUnknown), fSignalROI() {}

  //----------------------------------------------------------------------
  Wire::Wire(RegionsOfInterest_t const& sigROIlist, raw::ChannelID_t channel, geo::View_t view)
    : fChannel(channel), fView(view), fSignalROI(sigROIlist)
  {}

  //----------------------------------------------------------------------
  Wire::Wire(RegionsOfInterest_t&& sigROIlist, raw::ChannelID_t channel, geo::View_t view)
    : fChannel(channel), fView(view), fSignalROI(std::move(sigROIlist))
  {}

  //----------------------------------------------------------------------
  std::vector<float> Wire::Signal() const
  {
    return {fSignalROI.begin(), fSignalROI.end()};
  } // Wire::Signal()

}
////////////////////////////////////////////////////////////////////////
