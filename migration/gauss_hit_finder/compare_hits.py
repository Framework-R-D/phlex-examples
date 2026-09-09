#!/usr/bin/env python3
"""Compare two files, requiring the same number of lines and that less than 3% differ."""

import sys


def compare_files(reference, test):
    # read the contents of both files into lists of lines
    # OK because the files are small, a few MB
    with open(reference) as f:
        ref_lines = f.readlines()
    with open(test) as f:
        test_lines = f.readlines()

    if len(ref_lines) != len(test_lines):
        print(
            f"FAIL: Line count differs: {reference} has {len(ref_lines)} lines, "
            f"{test} has {len(test_lines)} lines"
        )
        return False

    total = len(ref_lines)
    if total == 0:
        print("PASS: Both files are empty")
        return True

    diff_count = sum(1 for r, t in zip(ref_lines, test_lines) if r != t)
    pct = 100.0 * diff_count / total

    if pct >= 3.0:
        print(f"FAIL: {diff_count}/{total} lines differ ({pct:.2g}%) — threshold is 3%")
        return False

    print(f"PASS: {diff_count}/{total} lines differ ({pct:.2g}%) — within 3% threshold")
    return True


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <reference> <test>")
        sys.exit(2)

    if not compare_files(sys.argv[1], sys.argv[2]):
        sys.exit(1)
