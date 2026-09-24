# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License, Version 2.0. See LICENSE.
"""Test execution through CTest labels."""

import enum

from .. import console, proc
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
    return cfg.expand(cfg.tool.ctest_exe) or "ctest"


def test(cfg, paths, suite=DEFAULT_SUITE, filter=None, repeat=0, jobs=None):
    """Run a suite via CTest. Returns its exit code."""
    console.header("Test")
    build_dir = paths.build
    if not build_dir.is_dir():
        console.error(f"{build_dir} not found. Run `st build` first.")
        return 1
    if build_svc.stale_tree(build_dir, paths.root):
        console.error(
            f"{build_dir} was configured for a different source path "
            f"(e.g. a Docker volume build reused on this host).\n"
            f"  - run `st build --clean` to reconfigure it cleanly first.")
        return 1

    command = [ctest_exe(cfg), "--test-dir", str(build_dir),
               "--output-on-failure"]
    if suite != "all":
        command += ["-L", SUITE_LABELS[suite]]
    if filter:
        # gtest_discover_tests registers tests as "Suite.Case"; ctest -R filters those nam
        command += ["-R", filter]
    if repeat > 0:
        command += ["--repeat", f"until-fail:{repeat}"]
        console.info(f"Repeating each test up to {repeat}x "
                     f"(stops on the first failure).")
    if jobs and jobs > 1:
        command += ["--parallel", str(jobs)]
    # A multi-config generator (Visual Studio, Xcode, Ninja Multi-Config)
    # leaves CMAKE_BUILD_TYPE unset in the cache -- the configuration is
    # chosen at build/test time instead, via -C, which every generator kind
    # accepts harmlessly. Keyed on the cache, not the host OS, so a
    # multi-config generator used anywhere (Ninja Multi-Config on Linux,
    # Visual Studio on Windows) is handled the same way; single-config
    # generators (Ninja, Unix Makefiles, ...) always populate the cache
    # value, so this never fires for them regardless of platform.
    build_type = build_svc.cache_value(build_dir, "CMAKE_BUILD_TYPE")
    if not build_type:
        command += ["-C", build_svc.normalize_build_type(cfg.build_type)]

    console.info(f"Suite: {suite}")
    code = proc.run_drained(command, cwd=build_dir,
                            env=load_build_env(build_dir))
    if code != 0:
        console.error(f"Suite '{suite}' failed (exit {code}).")
    return code
