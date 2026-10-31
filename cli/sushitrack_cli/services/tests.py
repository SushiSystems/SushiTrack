# tests.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""What `st test` hands the shared test command: the tree, the CTest call and the pytest suite."""

import sys

from sushicore import cmake_cache
from sushicore.testing import ModuleTests, SanitizedTests
from sushicore.testing.labels import LabelSelection

from .. import console
from ..config import PROFILE
from ..env import load_build_env
from ..process import RUNNER
from . import build as build_svc


def _config_flag(cfg, build_dir):
    """Return the -C value a multi-config tree needs, or None for a single-config one."""
    if cmake_cache.cached_value(build_dir, "CMAKE_BUILD_TYPE"):
        return None
    return build_svc.normalize_build_type(cfg.build_type)


def module_tests(context, panel=None):
    """Return what `st test` hands the shared command, reading the workspace from *context* on use.

    Args:
        context: Returns the configuration and the paths of the project.
        panel: The help panel `test` is listed under.
    """
    def tree():
        cfg, paths = context()
        return build_svc.tree_for(paths, build_svc.normalize_build_type(cfg.build_type))

    def sanitized_tree(name):
        cfg, paths = context()
        return build_svc.tree_for(paths, build_svc.normalize_build_type(cfg.build_type),
                                  build_svc.Sanitizer(name))

    def build_sanitized(name):
        cfg, paths = context()
        return build_svc.build(cfg, paths, sanitizer=build_svc.Sanitizer(name))

    def check_tree(build_dir):
        _, paths = context()
        if cmake_cache.is_stale(build_dir, paths.root):
            return (f"{build_dir} was configured for a different source path "
                    f"(e.g. a Docker volume build reused on this host).\n"
                    f"  - run `st build --clean` to reconfigure it cleanly first.")
        return None

    def run_ctest(build_dir, **options):
        cfg, paths = context()
        return build_svc.DRIVER.ctest_run(
            cfg.tool, build_dir, load_build_env(cfg.tool, build_dir, paths.config),
            config=_config_flag(cfg, build_dir), **options)

    def run_cli_tests(selection: LabelSelection):
        _, paths = context()
        console.header("Test (st package)")
        expression = selection.pytest_expression()
        console.info(f"Markers: {expression}")
        return RUNNER.run([sys.executable, "-m", "pytest", "-q", "-m", expression], paths.cli)

    return ModuleTests(
        profile=PROFILE, project_root=lambda: context()[1].root, console=console.current,
        build_tree=tree, run_ctest=run_ctest, run=RUNNER.run, check_tree=check_tree, panel=panel,
        sanitized=SanitizedTests(names=tuple(item.value for item in build_svc.Sanitizer),
                                 build=build_sanitized, tree=sanitized_tree),
        cli_tests=run_cli_tests)
