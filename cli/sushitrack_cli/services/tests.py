"""Test execution through CTest labels.

Which tests exist and how they are grouped is declared once, in
tests/*/CMakeLists.txt, as a ``LABELS`` property on each ``gtest_discover_tests``
call. This module only turns a suite name into a label selection, so adding a
test file — or a whole target — needs no change here.

The MOT metric pipeline and the video inference pipeline are not CTest suites and
live in :mod:`label` and :mod:`infer`.
"""

from .. import console, proc
from ..config import IS_WINDOWS
from ..env import load_build_env
from . import build as build_svc

# Suite -> CTest label-selection regex (`ctest -L`). The single categories anchor
# an exact match; "functional" is an alternation over its sub-labels rather than a
# label of its own, because a multi-element LABELS list is not reliably preserved
# by gtest_discover_tests across CMake versions. "all" selects everything (no -L).
SUITE_LABELS = {
    "unit": "^unit$",
    "integration": "^integration$",
    "regression": "^regression$",
    "functional": "^(unit|integration|regression)$",
}
SUITES = (*SUITE_LABELS, "all")
DEFAULT_SUITE = "functional"


def ctest_exe(cfg):
    return cfg.expand(cfg.build_ctest) or "ctest"


def test(cfg, paths, suite=DEFAULT_SUITE, filter=None, repeat=0, jobs=None):
    """Run a suite via CTest. Returns its exit code."""
    console.header("Test")
    build_dir = paths.build
    if not build_dir.is_dir():
        console.error(f"{build_dir} not found. Run `ds build` first.")
        return 1
    if build_svc.stale_tree(build_dir, paths.root):
        console.error(
            f"{build_dir} was configured for a different source path "
            f"(e.g. a Docker volume build reused on this host).\n"
            f"  - run `ds build --clean` to reconfigure it cleanly first.")
        return 1

    command = [ctest_exe(cfg), "--test-dir", str(build_dir),
               "--output-on-failure"]
    if suite != "all":
        command += ["-L", SUITE_LABELS[suite]]
    if filter:
        # gtest_discover_tests registers tests as "Suite.Case"; ctest -R filters
        # those names directly, which also reaches tests in any target.
        command += ["-R", filter]
    if repeat > 0:
        command += ["--repeat", f"until-fail:{repeat}"]
        console.info(f"Repeating each test up to {repeat}x "
                     f"(stops on the first failure).")
    if jobs and jobs > 1:
        command += ["--parallel", str(jobs)]
    if IS_WINDOWS:
        # A multi-config generator needs the configuration named explicitly;
        # harmless on a single-config tree, which ignores it.
        build_type = build_svc.cache_value(build_dir, "CMAKE_BUILD_TYPE")
        if not build_type:
            command += ["-C", build_svc.normalize_build_type(cfg.build_type)]

    console.info(f"Suite: {suite}")
    code = proc.run_drained(command, cwd=build_dir,
                            env=load_build_env(build_dir))
    if code != 0:
        console.error(f"Suite '{suite}' failed (exit {code}).")
    return code
