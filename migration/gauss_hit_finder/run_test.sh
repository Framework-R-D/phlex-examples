#!/bin/bash

# Generic wrapper script for gauss hit finder integration tests.
#
# Usage: run_test.sh <phlex_exe> <config_file> <output_prefix>
#
# Steps:
# 1. Removes any pre-existing output files matching the prefix
# 2. Runs the phlex executable with the given configuration
# 3. Verifies all 5 expected output files were produced

phlex_exe="$1"
config_file="$2"
output_prefix="$3"

if [ -z "$phlex_exe" ] || [ -z "$config_file" ] || [ -z "$output_prefix" ]; then
  echo "Usage: $0 <phlex_exe> <config_file> <output_prefix>"
  exit 1
fi

# Remove any pre-existing output files
for n in 0 1 2 3 4; do
  rm -f "${output_prefix}_${n}.txt"
done

# Run phlex
"$phlex_exe" -c "$config_file"
phlex_exit=$?

if [ $phlex_exit -ne 0 ]; then
  echo "ERROR: phlex exited with code $phlex_exit"
  exit $phlex_exit
fi

# Check that all expected output files exist
missing=0
for n in 0 1 2 3 4; do
  if [ ! -f "${output_prefix}_${n}.txt" ]; then
    echo "ERROR: expected output file ${output_prefix}_${n}.txt was not produced"
    missing=1
  fi
done

if [ $missing -ne 0 ]; then
  echo "ERROR: phlex exited successfully but not all output files were produced"
  echo "Listing ${output_prefix}_*.txt files that do exist:"
  ls -la ${output_prefix}_*.txt 2>/dev/null || echo "  (none)"
  exit 1
fi

echo "All 5 output files verified."
