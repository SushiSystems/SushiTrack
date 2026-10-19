# tests.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Runs the tests of the build tree through CTest, selected by size label."""

from sushicore import cmake_cache

from .. import console
from ..env import load_build_env
from . import build as build_svc
from .test_label import select_labels


def _config_flag(cfg, build_dir):
    """Return the -C value a multi-config tree needs, or None for a single-config one."""
    if cmake_cache.cached_value(build_dir, "CMAKE_BUILD_TYPE"):
        return None
    return build_svc.normalize_build_type(cfg.build_type)


def test(cfg, paths, labels=None, filter=None, repeat=0, jobs=None):
    """Run the tests whose labels the selection names.

    Args:
        labels: The TestLabel values to run; none runs small and medium.

    Returns:
        ctest's exit code, or 1 when the tree is absent or stale.
    """
    console.header("Test")
    build_dir = paths.build
    if not build_dir.is_dir():
        console.error(f"{build_dir} not found. Run `st build` first.")
        return 1
    if cmake_cache.is_stale(build_dir, paths.root):
        console.error(
            f"{build_dir} was configured for a different source path "
            f"(e.g. a Docker volume build reused on this host).\n"
            f"  - run `st build --clean` to reconfigure it cleanly first.")
        return 1

    selection = select_labels(labels)
    console.info("Labels: " + ", ".join(
        [size.value for size in selection.sizes] + (["quarantined"] if selection.quarantined else [])))
    code = build_svc.DRIVER.ctest_run(
        cfg.tool, build_dir, load_build_env(cfg.tool, build_dir, paths.config),
        label_regex=selection.ctest_include(),
        exclude_label_regex=selection.ctest_exclude(),
        filter=filter, repeat=repeat, jobs=jobs,
        config=_config_flag(cfg, build_dir))
    if code != 0:
        console.error(f"Tests failed (exit {code}).")
    return code
