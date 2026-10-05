# build.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026 Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Configure, build and clean the project."""

import enum
import shutil
from pathlib import Path

from sushicore import cmake_cache
from sushicore.cmake_driver import CMakeDriver

from .. import console
from ..env import load_build_env
from ..errors import UnknownBuildTypeError
from ..process import RUNNER

BUILD_TYPES = ("Release", "Debug", "RelWithDebInfo", "MinSizeRel")


class BuildType(str, enum.Enum):
    """The `--type` choices for `st build`, lower-cased for the CLI surface."""

    release = "release"
    debug = "debug"
    relwithdebinfo = "relwithdebinfo"
    minsizerel = "minsizerel"

LIBRARY_TARGET = "sushitrack"
# TEST_TARGETS names every target the test build produces.
TEST_TARGETS = ("unit_test", "integration_test", "regression_test",
                "tracker_bridge")
# The CMake options those targets are gated behind.
TEST_OPTIONS = ("BUILD_UNIT_TEST", "BUILD_REGRESSION_TEST",
                "BUILD_INTEGRATION_TEST")

# The stamp file records the configuration the build tree was configured with.
STAMP_NAME = ".sushitrack_config"


def normalize_build_type(value):
    """Accept a lowercase build type from the command line, return CMake's casing."""
    for known in BUILD_TYPES:
        if value.lower() == known.lower():
            return known
    raise UnknownBuildTypeError(f"Unknown build type '{value}'. "
                                f"Choose one of: {', '.join(BUILD_TYPES)}.")


#: Generators whose configured tree leaves a build file behind.
_SENTINEL_GENERATORS = ("Ninja", "Unix Makefiles")

DRIVER = CMakeDriver(console, RUNNER)


def generator(cfg) -> str:
    """Return the configured generator, else Ninja on Windows and CMake's default elsewhere."""
    configured = cfg.tool.expand(cfg.tool.generator)
    if configured:
        return configured
    return "Ninja" if cfg.tool.is_windows else ""


def _remove_tree(target: Path) -> bool:
    """Deletes *target* and returns whether it is gone, reporting a tree that survived."""
    shutil.rmtree(target, ignore_errors=True)
    if not target.exists():
        return True
    console.error(f"Could not remove {target}; close whatever holds a file in it and retry.")
    return False


def _stamp_key(build_type, tests, generator_name, defines):
    """Join the settings that force a reconfigure when they change."""
    return "|".join([build_type, "ON" if tests else "OFF", generator_name,
                     *sorted(defines)])


def _needs_configure(build_dir: Path, generator_name: str, stamp_key: str,
                     root: Path) -> bool:
    """Return whether the tree must be configured from scratch."""
    if generator_name in _SENTINEL_GENERATORS:
        if DRIVER.needs_configure(build_dir, generator_name, root=root):
            return True
    elif not build_dir.is_dir():
        return True
    elif cmake_cache.is_stale(build_dir, root):
        console.warn("Build tree was configured for a different source path "
                     "(e.g. a Docker volume build); reconfiguring from scratch.")
        return True
    if not (build_dir / "CMakeCache.txt").is_file():
        return True
    stamp = build_dir / STAMP_NAME
    if not stamp.is_file():
        return True
    return stamp.read_text(encoding="utf-8").strip() != stamp_key


def _configure_args(cfg, paths, build_type, tests, generator_name, defines):
    """Return the configure argv st's build policy asks for."""
    args = [DRIVER.cmake(cfg.tool), "-S", str(paths.root), "-B", str(paths.build)]
    if generator_name:
        args += ["-G", generator_name]
    args += [f"-DCMAKE_BUILD_TYPE={build_type}", "-DBUILD_MOT_TEST=OFF"]
    args += [f"-D{option}={'ON' if tests else 'OFF'}" for option in TEST_OPTIONS]
    return args + list(defines)


def build(cfg, paths, build_type=None, clean=False, jobs=None, tests=True,
          defines=()):
    """Configure when needed, then build st's targets.

    Returns:
        The exit code of the failing step, else 0.
    """
    console.header("Build")
    build_dir = paths.build
    build_type = build_type or normalize_build_type(cfg.build_type)
    generator_name = generator(cfg)

    if clean and build_dir.exists():
        console.info(f"Removing build directory: {build_dir}")
        if not _remove_tree(build_dir):
            return 1

    console.info(f"Type: {build_type}  |  tests: {'ON' if tests else 'OFF'}"
                 + (f"  |  generator: {generator_name}" if generator_name else ""))

    env = load_build_env(cfg.tool, build_dir)
    stamp_key = _stamp_key(build_type, tests, generator_name, defines)

    if _needs_configure(build_dir, generator_name, stamp_key, paths.root):
        if build_dir.exists() and not _remove_tree(build_dir):
            return 1
        build_dir.mkdir(parents=True, exist_ok=True)
        console.info("Configuring...")
        args = _configure_args(cfg, paths, build_type, tests, generator_name, defines)
        code = DRIVER.configure(args, paths.root, env)
        if code != 0:
            console.error("CMake configuration failed.")
            return code
        (build_dir / STAMP_NAME).write_text(stamp_key, encoding="utf-8")
    else:
        console.info("Build tree already configured for this configuration; "
                     "skipping cmake configure.")

    targets = [LIBRARY_TARGET] + (list(TEST_TARGETS) if tests else [])
    console.info("Building...")
    code = DRIVER.compile(cfg.tool, build_dir, paths.root, env, config=build_type,
                          targets=targets, jobs=jobs or cfg.jobs())
    if code != 0:
        console.error("Build failed.")
        return code
    console.success(f"Build completed ({', '.join(targets)}).")
    return 0


def clean(paths):
    """Remove the build tree and the assembled package directory.

    Returns:
        0 when both are gone, 1 when one of them could not be removed.
    """
    console.header("Clean")
    removed = []
    for target in (paths.build, paths.package):
        if target.exists():
            console.info(f"Removing {target}")
            if not _remove_tree(Path(target)):
                return 1
            removed.append(Path(target).name)
    if not removed:
        console.info("Nothing to remove; the tree is already clean.")
    else:
        console.success(f"Removed: {', '.join(removed)}.")
    return 0
