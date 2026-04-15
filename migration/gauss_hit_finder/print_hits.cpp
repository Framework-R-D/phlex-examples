#include <fmt/os.h>

#include "print_hits.hpp"

void print_hits_to_file(std::vector<recob::Hit> const& hits,
                        std::string const& filename) {
  auto file = fmt::output_file(filename);
  file.print("Contents of a std::vector<recob::Hit>\n\n");

  int default_precision = 6;

  for (int i = 0; auto const& hit : hits) {
    file.print("i = {}\n", i++);
    file.print("{}\n", hit.Channel());
    file.print("{}\n", hit.StartTick());
    file.print("{}\n", hit.EndTick());
    file.print("{:.{}g}\n", hit.PeakTime(), default_precision);
    file.print("{:.{}g}\n", hit.SigmaPeakTime(), default_precision);
    file.print("{:.{}g}\n", hit.RMS(), default_precision);
    file.print("{:.{}g}\n", hit.PeakAmplitude(), default_precision);
    file.print("{:.{}g}\n", hit.SigmaPeakAmplitude(), default_precision);
    file.print("{:.{}g}\n", hit.ROISummedADC(), default_precision);
    file.print("{:.{}g}\n", hit.HitSummedADC(), default_precision);
    file.print("{:.{}g}\n", hit.Integral(), default_precision);
    file.print("{:.{}g}\n", hit.SigmaIntegral(), default_precision);
    file.print("{}\n", hit.Multiplicity());
    file.print("{}\n", hit.LocalIndex());
    file.print("{:.{}g}\n", hit.GoodnessOfFit(), default_precision);
    file.print("{}\n", hit.DegreesOfFreedom());
    file.print("{}\n", static_cast<int>(hit.View()));

    // These are commented out for now because I am
    // using this print out to make a comparison to
    // the output of the art version of GausHitFinder.
    // These fields depend on Geometry and that is
    // not implemented yet in phlex.
    //
    //     hit.SignalType()
    //     hit.WireID()

  }
}
