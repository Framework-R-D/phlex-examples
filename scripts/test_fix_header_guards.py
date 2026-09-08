"""Regression tests for header guard naming."""

from pathlib import Path
import tempfile
import unittest

from scripts.fix_header_guards import (
    check_header_guard,
    compute_expected_guard,
    fix_header_guard,
)


class ComputeExpectedGuardTests(unittest.TestCase):
    def test_top_level_header_guard(self) -> None:
        root = Path("/project")

        self.assertEqual(
            compute_expected_guard(root / "my_geometry.hpp", root), "MY_GEOMETRY_HPP"
        )

    def test_nested_header_guard(self) -> None:
        root = Path("/project")

        self.assertEqual(
            compute_expected_guard(
                root / "migration" / "geometry" / "my-geometry.hpp", root
            ),
            "MIGRATION_GEOMETRY_MY_GEOMETRY_HPP",
        )

    def test_fixes_malformed_top_level_guard(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            header = root / "my_geometry.hpp"
            header.write_text(
                "#ifndef MY_GEOMETRY.HPP_MY_GEOMETRY_HPP\n"
                "#define MY_GEOMETRY .HPP_MY_GEOMETRY_HPP\n"
                "#endif // MY_GEOMETRY.HPP_MY_GEOMETRY_HPP\n",
                encoding="utf-8",
            )

            self.assertFalse(check_header_guard(header, root)[0])
            self.assertTrue(fix_header_guard(header, root))
            self.assertTrue(check_header_guard(header, root)[0])
            self.assertEqual(
                header.read_text(encoding="utf-8"),
                "#ifndef MY_GEOMETRY_HPP\n"
                "#define MY_GEOMETRY_HPP\n"
                "#endif // MY_GEOMETRY_HPP\n",
            )
