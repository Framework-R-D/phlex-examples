# Proxies for framework-agnostic experiment libraries
# N.B. The specified library type should be SHARED.
add_library(find_hits_with_gaussians SHARED
    find_hits_with_gaussians_design0.cpp
    find_hits_with_gaussians_design1.cpp
    find_hits_with_gaussians_design2.cpp
    find_hits_with_gaussians_design3.cpp
    cand_hit_standard.cpp
    copied_from_larsoft_minor_edits/Hit.cxx
    copied_from_larsoft_minor_edits/geo_types.cxx
    copied_from_larsoft_minor_edits/CandHitStandard.cxx
    copied_from_larsoft_minor_edits/PeakFitterMrqdt.cxx
    copied_from_larsoft_minor_edits/HitFilterAlg.cxx
    copied_from_larsoft_minor_edits/MarqFitAlg.cxx
    copied_from_larsoft_minor_edits/Wire.cxx
    wire_serialization.cpp
    print_hits_to_file.cpp
)
target_link_libraries(find_hits_with_gaussians PRIVATE TBB::tbb fmt::fmt)

# Register providers for data products
add_library(wires_source MODULE wires_source.cpp)
target_link_libraries(wires_source PRIVATE phlex::module find_hits_with_gaussians)

# Design variants used for both library registration and testing
set(designs 0 1 2 3)

# Register experiment libraries with Phlex
foreach(d IN LISTS designs)
  add_library(find_hits_with_gaussians_design${d}_hof MODULE
              register_find_hits_with_gaussians_design${d}.cpp)
  target_link_libraries(find_hits_with_gaussians_design${d}_hof PRIVATE
                        phlex::module find_hits_with_gaussians)
endforeach()

add_library(find_hits_with_gaussians_cell_id_hof MODULE
            register_find_hits_with_gaussians_cell_id.cpp)
target_link_libraries(find_hits_with_gaussians_cell_id_hof PRIVATE
                      phlex::module)

add_library(print_hits_to_file_hof MODULE
            register_print_hits_to_file.cpp)
target_link_libraries(print_hits_to_file_hof PRIVATE
                      phlex::module find_hits_with_gaussians)

file(GLOB test_files ${CMAKE_CURRENT_SOURCE_DIR}/*.dat)
file(COPY ${test_files} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

file(GLOB test_files_art ${CMAKE_CURRENT_SOURCE_DIR}/art_hits_*.txt)
file(COPY ${test_files_art} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# -----------------------------------------------------------------------
# Integration tests
#
# Each design variant runs phlex with its jsonnet configuration, then
# compares the output hits against reference files (art_hits_N.txt) for
# 5 events (spills 0-4).  The reference files were generated on Linux,
# so on Linux we use exact diff; on macOS we use compare_hits.py for
# approximate comparison; on other platforms the comparison tests always
# fail to flag the need for a new comparison method.
#
# Design variants:
#   design0 - single transform (no unfold)
#   design1 - unfold-transform-fold replaces the single transform
#   design2 - unfold-unfold-transform-fold-fold
#   design3 - like design2 but with a separate transform that finds
#             and merges hit candidates inserted between the second
#             unfold and the Gaussian hit-fitting transform
#
# All variants use a common wrapper script (run_test.sh) that deletes
# stale output files before the running phlex and then verifies all
# expected files were produced. run_test.sh takes 3 arguments, the
# phlex executable, the jsonnet configuration file, and a prefix
# for the output filenames.
# -----------------------------------------------------------------------

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
endif()

set(all_run_tests "")

foreach(d IN LISTS designs)
  set(run_test   "GausHitFinderTestDesign${d}")
  set(fixture    "GausHitFinderDesign${d}Fixture")
  set(output_prefix "hits_design${d}")

  # --- Run test ---
  add_test(NAME ${run_test}
           COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/run_test.sh
                        $<TARGET_FILE:phlex>
                        ${CMAKE_CURRENT_SOURCE_DIR}/test_find_hits_with_gaussians_design${d}.jsonnet
                        ${output_prefix}
           WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  set_tests_properties(${run_test} PROPERTIES FIXTURES_SETUP ${fixture})
  list(APPEND all_run_tests ${run_test})

  # --- Comparison tests (one per spill, 0-4) ---
  foreach(n RANGE 4)
    set(compare_test "GausHitFinderCompareDesign${d}_${n}")
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
      add_test(
        NAME ${compare_test}
        COMMAND diff -q -s
        ${CMAKE_CURRENT_SOURCE_DIR}/art_hits_${n}.txt
        ${CMAKE_CURRENT_BINARY_DIR}/${output_prefix}_${n}.txt
      )
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
      add_test(
        NAME ${compare_test}
        COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/compare_hits.py
        ${CMAKE_CURRENT_SOURCE_DIR}/art_hits_${n}.txt
        ${CMAKE_CURRENT_BINARY_DIR}/${output_prefix}_${n}.txt
      )
    else()
      add_test(
        NAME ${compare_test}
        COMMAND ${CMAKE_COMMAND} -E false
      )
    endif()
    set_tests_properties(${compare_test} PROPERTIES FIXTURES_REQUIRED ${fixture})
  endforeach()
endforeach()

# We need to include the library directory for 'generate_layers' on the PHLEX_PLUGIN_PATH.
# Unfortunately, phlex does not have a target that easily does this (plugins are not intended
# to be accessible downstream), so we have to work around that.  Right now, the library directory
# for phlex::core is the same as the library directory for the plugins.  So we use the library
# directory of phlex::core as a proxy for the plugin directory.

set_tests_properties(
  ${all_run_tests}
  PROPERTIES
    ENVIRONMENT_MODIFICATION
      "PHLEX_PLUGIN_PATH=path_list_prepend:${CMAKE_CURRENT_BINARY_DIR}:$<TARGET_FILE_DIR:phlex::core>"
)
