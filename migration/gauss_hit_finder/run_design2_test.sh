#!/bin/bash

# Wrapper script for the design2 test that:
# 1. Removes any pre-existing output files
# 2. Runs the phlex executable
# 3. Verifies all expected output files were produced

phlex_exe="$1"
config_file="$2"

# Remove any pre-existing output files
for n in 0 1 2 3 4; do
  rm -f "hits_design2_${n}.txt"
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
  if [ ! -f "hits_design2_${n}.txt" ]; then
    echo "ERROR: expected output file hits_design2_${n}.txt was not produced"
    missing=1
  fi
done

if [ $missing -ne 0 ]; then
  echo "ERROR: phlex exited successfully but not all output files were produced"
  echo "Listing hits_design2_*.txt files that do exist:"
  ls -la hits_design2_*.txt 2>/dev/null || echo "  (none)"
  exit 1
fi

echo "All 5 output files verified."
