# Purpose of this directory

This directory holds an example showing how to convert a
module that works with the `art` framework into
algorithms that work with the `phlex` framework. This
particular example converts the module defined in
the file GausHitFinder_module.cc from the LArSoft larreco
repository.

This is only an example and a prototype to be used in
testing. It is not intended to be the version of this
module used by DUNE or any other experiment
for production or physics. That belongs in some other
repository.

This example is based on `main` branch of `phlex` as of the
last time the example was updated. It might need modifications
to work with the newest version of `phlex`. The migration to
phlex started with version of GausHitFinder_module.cc in
v10_05_00 of larreco which was the version in use with DUNE
software at the time the migration work was started. If there
is a request, I can update the example to a more recent version.

# Work in Progress

This is work in progress. It is not final. It is intended
that this example will change as we learn more about using
`phlex` and as we receive feedback from DUNE. Currently, this
is more like a rough first draft of our first attempts
to use `phlex`. Please don't expect stability. Many features
of `phlex` have not been implemented and the design of `phlex`
itself is changing. Those changes will also cause this example
to change.

As we study this example, it will guide how phlex is used.
As we run tests, it may point to problems in phlex and this
may guide which areas developers should focus on to fix
issues and remove performance bottlenecks.

# Files in this directory

## Interesting files that are the heart of the example

There are multiple prototypes to explore different
approaches to migrating GausHitFinder to `phlex`.

Design 0 is an example where one large `phlex`
algorithm replaces GausHitFinder.

1. find_hits_with_gaussians_design0.hpp
2. find_hits_with_gaussians_design0.cpp
3. register_find_hits_with_gaussians_design0.cpp
4. test_find_hits_with_gaussians_design0.jsonnet

Design 1 an example where there is an unfold algorithm,
followed by a transform algorithm, and finally a fold
algorithm. The outer parallel_for iteration is replaced
by that sequence of algorithms.

1. find_hits_with_gaussians_design1.hpp
2. find_hits_with_gaussians_design1.cpp
3. register_find_hits_with_gaussians_design1.cpp
4. test_find_hits_with_gaussians_design1.jsonnet

Design 2 extends design 1 by also replacing the inner
parallel_for (over ROIs) with a second unfold-transform-fold.
The resulting layer hierarchy is:
spill -> wire -> roi. A `wire_roi_data` struct bundles
a single ROI (`datarange_t`) with the wire-level context
(channel, view, plane) needed by the transform.

1. find_hits_with_gaussians_design2.hpp
2. find_hits_with_gaussians_design2.cpp
3. register_find_hits_with_gaussians_design2.cpp
4. test_find_hits_with_gaussians_design2.jsonnet
5. wire_roi_data.hpp

Design 3 extends design 2 by extracting the
candidate-hit-finding step (`cand_hit_standard`) into a
separate transform that runs after the second unfold.
Both `cand_hit_standard` and the Gaussian hit-fitting
transform receive the `wire_roi_data` directly from the
second unfold. The `cand_hit_standard` transform outputs
`merge_hit_candidate_vec`, which is then provided as a
second input to the Gaussian hit-fitting transform.

1. find_hits_with_gaussians_design3.hpp
2. find_hits_with_gaussians_design3.cpp
3. register_find_hits_with_gaussians_design3.cpp
4. test_find_hits_with_gaussians_design3.jsonnet
5. cand_hit_standard.hpp
6. cand_hit_standard.cpp
7. hit_candidate.hpp
8. wire_roi_data.hpp

We plan to implement more prototype migrations
of GausHitFinder in the future to explore the
possibilities and execute tests.

## These files exist only for test purposes

There is one algorithm that will print out reconstructed hits.
This printout can be used to compare results between an
`art` process and a `phlex` process running GausHitFinder.
There are functions that can be used to persistently store
the input from an `art` process (a vector of `Wire`) so
it can be used in the `phlex` process. And there is a `phlex` provider
to read this input. This persistence mechanism is necessary
because there is not an alternative yet. These files are all
temporary and not part of the migration example.

1. print_hits_to_file.hpp
2. print_hits_to_file.cpp
3. wire_serialization.hpp
4. wire_serialization.cpp
5. wires_source.cpp
6. register_print_hits_to_file.cpp
7. art_hits_*.txt
8. wires_*.dat
9. register_find_hits_with_gaussians_cell_id.cpp
10. examples_generate_layers.cpp
11. run_test.sh
12. compare_hits.py

## Files copied from LArSoft

The infrastructure does not yet exist to link the
code here with LArSoft code or include headers.
Necessary files are copied in with minimal changes.
Some changes are necessary because the files are in
a different location and some remove unwanted dependences,
but the modifications are small. When it is possible
to depend on LArSoft code in some other repository,
these files should be deleted to remove duplication.

These files are in the subdirectory:

  ```phlex-examples/migration/gauss_hit_finder/copied_from_larsoft_minor_edits```

# Where more work is needed

The first example was implemented as a single `phlex`
transform. Does it make sense to split it with unfolds
and folds? Does it make sense to have multiple transforms?
We don't know the answers to these questions yet.
Getting the single transform to work was a good first step.

The next thing we implemented was replacing the
outer `parallel_for` with an `unfold`, `transform`, and `fold`
sequence of algorithms (design 1). After that, we created
design 2 which also replaces the inner `parallel_for` (over ROIs)
with a second `unfold`, `transform`, and `fold` sequence,
resulting in a three-layer hierarchy (spill -> wire -> roi).
Design 3 goes further by extracting the candidate-hit-finding
step (`cand_hit_standard`) into its own transform, separate from
the Gaussian hit-fitting transform. Both transforms receive
`wire_roi_data` from the second unfold, and the merged hit
candidates flow from `cand_hit_standard` into the fitting
transform as a second input. A future design 4 will move the
`cand_hit_standard` registration into a separate module so that
alternative candidate-hit-finding implementations can be
swapped in independently.
We plan to run tests and compare the different versions.

`Phlex` will not support the `Tools` feature that existed in
`art`. One possibility is that algorithm nodes scheduled
by `tbb::flow_graph` offer sufficient configurability that we
don't need a separate plugin system like `Tools`.
Another possibility is that we eventually implement a
plugin system for `phlex`. In design 0, the two `Tool` types
used in `GausHitFinder` are replaced with direct instantiations
of a type similar to one of the plugin types (a vector of objects of type
`CandHitStandard` and one object of type `PeakFitterMrqdt`). They
needed some modification but are as close as reasonably possible
to the original versions. In design 3, the candidate-hit-finding
step is factored out as a separate `phlex` transform using a
new standalone implementation (`cand_hit_standard` namespace)
that uses new types (`hit_candidate`, `merge_hit_candidate_vec`)
rather than the legacy `ICandidateHitFinder` types. A conversion
to legacy types is done where `PeakFitterMrqdt` still requires them.

I do not know yet how to deal with the fact that `GausHitFinder`
can be configured to produce 1 or 2 output data products.
For now, the example can only produce one `std::vector<recob::Hit>`.
It's configurable whether to filter hits or not, but it cannot
produce both outputs at the same time.

For now, this ignores the fact that `GausHitFinder` also can
produce `art::Assns<recob::Wire, recob::Hit>` and
`art::Assns<raw::RawDigit, recob::Hit>`. In the `art` version,
it is configurable whether to produce those or not.
We are ignoring this because `phlex` does not yet support `art::Assns`
or whatever will be used to replace that type. For now, the
example can only produce one `std::vector<recob::Hit>`.

Here is a list of the files that were used as a basis
for this example, but significantly modified from the
LArSoft version:

```
https://github.com/LArSoft/larreco/blob/develop/larreco/HitFinder/GausHitFinder_module.cc
```

Some files were copied with minimal modifications. The "tool"
classes were split into .h and .cxx files and are no longer implemented
as `art Tools`, but otherwise the changes are minimal.
Here is a list of the files that were copied from LArSoft
with minimal modifications:

```
    https://github.com/LArSoft/larreco/blob/develop/larreco/HitFinder/HitFinderTools/CandHitStandard_tool.cc
    https://github.com/LArSoft/larreco/blob/develop/larreco/HitFinder/HitFinderTools/PeakFitterMrqdt_tool.cc
    https://github.com/LArSoft/larreco/blob/develop/larreco/HitFinder/HitFilterAlg.h
    https://github.com/LArSoft/larreco/blob/develop/larreco/HitFinder/HitFilterAlg.cxx
    https://github.com/LArSoft/lardataobj/blob/develop/lardataobj/RecoBase/Wire.h
    https://github.com/LArSoft/lardataobj/blob/develop/lardataobj/RecoBase/Wire.cxx
    https://github.com/LArSoft/lardataobj/blob/develop/lardataobj/RecoBase/Hit.h
    https://github.com/LArSoft/lardataobj/blob/develop/lardataobj/RecoBase/Hit.cxx
    https://github.com/LArSoft/larcoreobj/blob/develop/larcoreobj/SimpleTypesAndConstants/geo_types.h
    https://github.com/LArSoft/larcoreobj/blob/develop/larcoreobj/SimpleTypesAndConstants/geo_types.cxx
    https://github.com/LArSoft/larreco/blob/develop/larreco/HitFinder/HitFinderTools/ICandidateHitFinder.h
    https://github.com/LArSoft/larcoreobj/blob/develop/larcoreobj/SimpleTypesAndConstants/RawTypes.h
    https://github.com/LArSoft/lardataobj/blob/develop/lardataobj/Utilities/sparse_vector.h
    https://github.com/LArSoft/larvecutils/blob/develop/larvecutils/MarqFitAlg/MarqFitAlg.h
    https://github.com/LArSoft/larvecutils/blob/develop/larvecutils/MarqFitAlg/MarqFitAlg.cxx
    https://github.com/LArSoft/larreco/blob/develop/larreco/HitFinder/HitFinderTools/ICandidateHitFinder.h
    https://github.com/LArSoft/larreco/blob/develop/larreco/HitFinder/HitFinderTools/IPeakFitter.h
```

For now, the part of `GausHitFinder` that fills 2 histograms is removed.
Why? Because `phlex` does not yet have a histogramming facility like `TFileService`
yet. Also those histograms were booked in `beginJob`. That entire method
was removed. Booking the histograms was the only thing that was done in `beginJob`.
I'm not sure how to support `beginJob` activity in `phlex` either.

Dropped `fEventCount`. It was not used. Seems like not a good thing
to keep track of in a `phlex` algorithm anyway. Reconstruction algorithms
should not depend on event count. Also removed the related count argument
from the findHitCandidates method of ICandidateHitFinder and its implementations.
It was not used in CandHitStandard anyway.

MessageLogger calls are commented out for now but not deleted. Eventually
we will want to replace those with calls to whatever logging system `phlex`
eventually uses or delete those lines entirely.

For now, calls to the Geometry service are commented out but not deleted.
The current plan is to replace those with a `phlex` provider that provides the geometry
data as data products and these providers will be scheduled by the flow graph.
In the main algorithm the geometry is used to get the plane number for each
wire. For now, I just hard coded the plane number to 0. The signal type in
the `recob::Hit` objects is also from the geometry and for now always set
to 0. Similarly, the `WireID` in the `recob::Hit` objects is from the geometry
and for now set to a default value. `FillOutHitParameterVector` depends
on the geometry also, but it only checks the size of a configuration
vector against the number of planes. If this is skipped but the
configuration vector is sized to match the number of planes, then things
should still work ok. For now, I just skip FillOutHitParameterVector.

I copied in the content of the `TMath::Gaus` function from ROOT to
avoid depending on ROOT in `phlex` code for now (slightly edited).
Are we allowed to depend on ROOT in `phlex` algorithm code? That could
easily be restored if the build system allows it.

# Testing

This was tested by running `GausHitFinder` with the `art` framework and running the modified `GausHitFinder` with the `phlex` framework. The two processes used identical input (the `std::vector<Wire>`). The output `std::vector<recob::Hit>` objects were compared using a text file printed during each process. The output was identical except for the following:

Two output data members of `Hit` were ignored because they depend on the `Geometry` and that is not implemented yet for `phlex`. In the `phlex` version, these data members are filled with default values.
```
    geo::SigType_t fSignalType; ///< signal type for the plane of the hit
    geo::WireID fWireID;        ///< WireID for the hit (Cryostat, TPC, Plane, Wire)
```

This was done for all `phlex` versions of GausHitFinder. The output of all versions are identical with each other and with the `art` version.

The `phlex` process was run with multithreading and that causes the order of `Hit` objects to vary from one execution to the next and also the order of events to vary. In the comparison the `Hit` objects were sorted and we had to be careful to compare matching events.

Contact the `phlex` group if you are interested in details related
to running `art` side of this test or want the input files (or
to regenerate them).
