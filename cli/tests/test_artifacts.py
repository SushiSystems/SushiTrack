# test_artifacts.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""What the build-tree locator finds."""

from __future__ import annotations

from sushitrack_cli.services import artifacts


def _plant(path):
    """Create an empty executable file at *path*."""
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("", encoding="utf-8")
    path.chmod(0o755)
    return path


def test_find_file_prefers_the_conventional_bin_dirs(tmp_path):
    """Finds bin/Release before a deeper copy."""
    name = artifacts.shared_lib_name()
    first = _plant(tmp_path / "bin" / "Release" / name)
    _plant(tmp_path / "a" / "b" / name)
    assert artifacts.find_file(tmp_path, name) == first


def test_find_file_falls_back_to_a_recursive_search(tmp_path):
    """Finds a file outside the conventional directories."""
    name = artifacts.shared_lib_name()
    deep = _plant(tmp_path / "x" / "y" / name)
    assert artifacts.find_file(tmp_path, name) == deep


def test_find_file_answers_none_without_a_build_tree(tmp_path):
    """Returns None for a missing build tree."""
    assert artifacts.find_file(tmp_path / "absent", "x") is None


def test_the_index_skips_git_and_walks_deeper_than_four(tmp_path):
    """Keeps st's unbounded-enough walk and its .git prune."""
    exe = artifacts.executable_name("deep_tool")
    deep = _plant(tmp_path / "a" / "b" / "c" / "d" / "e" / exe)
    _plant(tmp_path / ".git" / artifacts.executable_name("hook"))
    assert artifacts.EXECUTABLES.find(tmp_path) == [deep]


def test_match_prefers_an_exact_name(tmp_path):
    """Matches the exact stem before a substring."""
    exact = _plant(tmp_path / "bin" / artifacts.executable_name("unit_test"))
    _plant(tmp_path / "bin" / artifacts.executable_name("unit_test_extra"))
    assert artifacts.EXECUTABLES.match(tmp_path, "unit_test") == exact


def test_find_binary_adds_the_platform_suffix(tmp_path):
    """Finds an executable by its bare name."""
    exe = _plant(tmp_path / "bin" / artifacts.executable_name("tool"))
    assert artifacts.find_binary(tmp_path, "tool") == exe


def test_import_lib_name_names_the_link_library():
    """Names the library a C++ consumer links against."""
    expected = "sushitrack.lib" if artifacts.IS_WINDOWS else "libsushitrack.so"
    assert artifacts.import_lib_name() == expected
