# Proxies for framework-agnostic experiment libraries
# N.B. The specified library type should be SHARED.
add_library(find_hits_with_gaussians SHARED
    find_hits_with_gaussians.cpp
    find_hits_with_gaussians_design1.cpp
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

# Register experiment libraries with Phlex
add_library(find_hits_with_gaussians_hof MODULE
            register_find_hits_with_gaussians.cpp)
target_link_libraries(find_hits_with_gaussians_hof PRIVATE
                      phlex::module find_hits_with_gaussians)

add_library(find_hits_with_gaussians_cell_id_hof MODULE
            register_find_hits_with_gaussians_cell_id.cpp)
target_link_libraries(find_hits_with_gaussians_cell_id_hof PRIVATE
                      phlex::module)

add_library(print_hits_to_file_hof MODULE
            register_print_hits_to_file.cpp)
target_link_libraries(print_hits_to_file_hof PRIVATE
                      phlex::module find_hits_with_gaussians)

add_library(find_hits_with_gaussians_design1_hof MODULE
            register_find_hits_with_gaussians_design1.cpp)
target_link_libraries(find_hits_with_gaussians_design1_hof PRIVATE
                      phlex::module find_hits_with_gaussians)

file(GLOB test_files ${CMAKE_CURRENT_SOURCE_DIR}/*.dat)
file(COPY ${test_files} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

file(GLOB test_files_art ${CMAKE_CURRENT_SOURCE_DIR}/art_hits_*.txt)
file(COPY ${test_files_art} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_test(NAME GausHitFinderTest
         COMMAND phlex -c ${CMAKE_CURRENT_SOURCE_DIR}/test_find_hits_with_gaussians.jsonnet
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# iterates 0 to 4 inclusive, creating a test to compare the output hits of
# the phlex job to the output hits of the art job for each event (5 events).
# In this test, the art "events" are called "spills" in phlex.

# The reference files were generated on a Linux machine
# so we expect identical output on Linux
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  foreach(n RANGE 4)
    add_test(
      NAME GausHitFinderCompare_${n}
      COMMAND diff -q -s
      ${CMAKE_CURRENT_SOURCE_DIR}/art_hits_${n}.txt
      ${CMAKE_CURRENT_BINARY_DIR}/hits_${n}.txt
    )
    set_tests_properties(GausHitFinderCompare_${n} PROPERTIES DEPENDS GausHitFinderTest)
  endforeach()
# On a Mac, we expect the output to be slightly
# different because the reference files were
# generated on a Linux machine.
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  foreach(n RANGE 4)
    add_test(
      NAME GausHitFinderCompare_${n}
      COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/compare_hits.py
      ${CMAKE_CURRENT_SOURCE_DIR}/art_hits_${n}.txt
      ${CMAKE_CURRENT_BINARY_DIR}/hits_${n}.txt
    )
    set_tests_properties(GausHitFinderCompare_${n} PROPERTIES DEPENDS GausHitFinderTest)
  endforeach()
# Always fail on other platforms
# This will warn us to add a new comparison
# method if we ever run on a new platform.
else()
  foreach(n RANGE 4)
    add_test(
      NAME GausHitFinderCompare_${n}
      COMMAND ${CMAKE_COMMAND} -E false
    )
    set_tests_properties(GausHitFinderCompare_${n} PROPERTIES DEPENDS GausHitFinderTest)
  endforeach()
endif()

# Repeat for the version with unfold-transform-fold instead of a single transform.
add_test(NAME GausHitFinderTestWithUnfold
         COMMAND phlex -c ${CMAKE_CURRENT_SOURCE_DIR}/test_find_hits_with_gaussians_design1.jsonnet
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# The tests must not be run concurrently because they read and write to the same file.
set_tests_properties(GausHitFinderTestWithUnfold PROPERTIES DEPENDS
                     "GausHitFinderCompare_0;GausHitFinderCompare_1;GausHitFinderCompare_2;GausHitFinderCompare_3;GausHitFinderCompare_4")

# The reference files were generated on a Linux machine
# so we expect identical output on Linux
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  foreach(n RANGE 4)
    add_test(
      NAME GausHitFinderCompareWithUnfold_${n}
      COMMAND diff -q -s
      ${CMAKE_CURRENT_SOURCE_DIR}/art_hits_${n}.txt
      ${CMAKE_CURRENT_BINARY_DIR}/hits_${n}.txt
     )
    set_tests_properties(GausHitFinderCompareWithUnfold_${n} PROPERTIES DEPENDS GausHitFinderTestWithUnfold)
  endforeach()
# On a Mac, we expect the output to be slightly
# different because the reference files were
# generated on a Linux machine.
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  foreach(n RANGE 4)
    add_test(
      NAME GausHitFinderCompareWithUnfold_${n}
      COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/compare_hits.py
      ${CMAKE_CURRENT_SOURCE_DIR}/art_hits_${n}.txt
      ${CMAKE_CURRENT_BINARY_DIR}/hits_${n}.txt
    )
    set_tests_properties(GausHitFinderCompareWithUnfold_${n} PROPERTIES DEPENDS GausHitFinderTestWithUnfold)
  endforeach()
# Always fail on other platforms
# This will warn us to add a new comparison
# method if we ever run on a new platform.
else()
  foreach(n RANGE 4)
    add_test(
      NAME GausHitFinderCompareWithUnfold_${n}
      COMMAND ${CMAKE_COMMAND} -E false
    )
    set_tests_properties(GausHitFinderCompareWithUnfold_${n} PROPERTIES DEPENDS GausHitFinderTestWithUnfold)
  endforeach()
endif()

# We need to include the library directory for 'generate_layers' on the PHLEX_PLUGIN_PATH.
# Unfortunately, phlex does not have a target that easily does this (plugins are not intended
# to be accessible downstream), so we have to work around that.  Right now, the library directory
# for phlex::core is the same as the library directory for the plugins.  So we use the library
# directory of phlex::core as a proxy for the plugin directory.

set_tests_properties(
  GausHitFinderTest
  GausHitFinderTestWithUnfold
  PROPERTIES
    ENVIRONMENT_MODIFICATION
      "PHLEX_PLUGIN_PATH=path_list_prepend:${CMAKE_CURRENT_BINARY_DIR}:$<TARGET_FILE_DIR:phlex::core>"
)
