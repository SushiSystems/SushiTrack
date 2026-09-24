# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License, Version 2.0. See LICENSE.
"""Test execution through CTest labels."""

import enum

from sushicore import cmake_cache

from .. import console
from ..env import load_build_env
from . import build as build_svc

# Suite -> CTest label-selection regex (`ctest -L`). The single categories anchor
SUITE_LABELS = {
    "unit": "^unit$",
    "integration": "^integration$",
    "regression": "^regression$",
    "functional": "^(unit|integration|regression)$",
}
SUITES = (*SUITE_LABELS, "all")
DEFAULT_SUITE = "functional"


class Suite(str, enum.Enum):
    """The `--suite` choices for `st test`."""

    unit = "unit"
    integration = "integration"
    regression = "regression"
    functional = "functional"
    all = "all"


def ctest_exe(cfg):
    """Return the ctest executable the driver resolves from *cfg*'s tool settings."""
    return build_svc.DRIVER.ctest(cfg.tool)


def _config_flag(cfg, build_dir):
    """Return the -C value a multi-config tree needs, or None for a single-config one."""
    if cmake_cache.cached_value(build_dir, "CMAKE_BUILD_TYPE"):
        return None
    return build_svc.normalize_build_type(cfg.build_type)


def test(cfg, paths, suite=DEFAULT_SUITE, filter=None, repeat=0, jobs=None):
    """Run a suite via CTest.

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

    console.info(f"Suite: {suite}")
    code = build_svc.DRIVER.ctest_run(
        cfg.tool, build_dir, load_build_env(cfg.tool, build_dir),
        label_regex=None if suite == "all" else SUITE_LABELS[suite],
        filter=filter, repeat=repeat, jobs=jobs,
        config=_config_flag(cfg, build_dir))
    if code != 0:
        console.error(f"Suite '{suite}' failed (exit {code}).")
    return code
