# Proxies for framework-agnostic experiment libraries
# N.B. The specified library type should be SHARED.
add_library(find_hits_with_gaussians SHARED
    find_hits_with_gaussians.cpp
    copied_from_larsoft_minor_edits/Hit.cxx
    copied_from_larsoft_minor_edits/geo_types.cxx
    copied_from_larsoft_minor_edits/CandHitStandard.cxx
    copied_from_larsoft_minor_edits/PeakFitterMrqdt.cxx
    copied_from_larsoft_minor_edits/HitFilterAlg.cxx
    copied_from_larsoft_minor_edits/MarqFitAlg.cxx
    copied_from_larsoft_minor_edits/Wire.cxx
    wire_serialization.cpp
    print_hits.cpp
)
target_link_libraries(find_hits_with_gaussians PRIVATE TBB::tbb fmt::fmt)

# Register providers for data products
add_library(wires_source MODULE wires_source.cpp)
target_link_libraries(wires_source PRIVATE phlex::module find_hits_with_gaussians)

# Register experiment libraries with Phlex
add_library(find_hits_with_gaussians_hof MODULE register_find_hits_with_gaussians.cpp)
target_link_libraries(find_hits_with_gaussians_hof PRIVATE phlex::module find_hits_with_gaussians)
