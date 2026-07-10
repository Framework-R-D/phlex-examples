// This is an algorithm that exists for testing purposes.
// It is not part of the migrated GausHitFinder algorithm,
// but instead is designed to print the contents of a
// std::vector<recob::Hit> to a file. This is useful for
// comparing the output of the phlex version GausHitFinder
// with the output of the art version of GausHitFinder.

#include <algorithm>
#include <fmt/os.h>
#include <string>

#include "print_hits_to_file.hpp"

namespace examples {

  void print_hits_to_file(int cell_id, std::vector<recob::Hit> const& input_hits)
  {

    std::vector<recob::Hit> hits = input_hits;

    // Sort hits by PeakTime ascending, then by PeakAmplitude ascending
    // I added the sorting when trying to compare the output Hits with
    // the output from the art version of GausHitFinder. The parallel_for
    // loops and unfold-transform-fold execution map both scramble the
    // order and make comparisons difficult.
    std::sort(hits.begin(), hits.end(), [](recob::Hit const& a, recob::Hit const& b) {
      if (a.PeakTime() != b.PeakTime()) {
        return a.PeakTime() < b.PeakTime();
      }
      if (a.PeakAmplitude() != b.PeakAmplitude()) {
        return a.PeakAmplitude() < b.PeakAmplitude();
      }
      if (a.StartTick() != b.StartTick()) {
        return a.StartTick() < b.StartTick();
      }
      return a.EndTick() < b.EndTick();
    });

    std::string filename = std::string("hits_") + std::to_string(cell_id) + ".txt";

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
}
