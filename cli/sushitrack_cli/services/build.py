# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License, Version 2.0. See LICENSE.
"""Configure, build and clean the project."""

import enum
import os
import shutil
from pathlib import Path

from .. import console, proc
from ..config import IS_WINDOWS
from ..env import load_build_env

BUILD_TYPES = ("Release", "Debug", "RelWithDebInfo", "MinSizeRel")


class BuildType(str, enum.Enum):
    """The `--type` choices for `st build`, lower-cased for the CLI surface."""

    release = "release"
    debug = "debug"
    relwithdebinfo = "relwithdebinfo"
    minsizerel = "minsizerel"

LIBRARY_TARGET = "sushitrack"
# Every target the test build produces. Listed explicitly (rather than building th
TEST_TARGETS = ("unit_test", "integration_test", "regression_test",
                "tracker_bridge")
# The CMake options those targets are gated behind.
TEST_OPTIONS = ("BUILD_UNIT_TEST", "BUILD_REGRESSION_TEST",
                "BUILD_INTEGRATION_TEST")

# Records the configuration a build tree was configured with. A single-config gene
STAMP_NAME = ".sushitrack_config"


def normalize_build_type(value):
    """Accept a lowercase build type from the command line, return CMake's casing."""
    for known in BUILD_TYPES:
        if value.lower() == known.lower():
            return known
    raise SystemExit(f"Unknown build type '{value}'. "
                     f"Choose one of: {', '.join(BUILD_TYPES)}.")


def cmake_exe(cfg):
    return cfg.expand(cfg.tool.cmake_exe) or "cmake"


def generator(cfg):
    configured = cfg.expand(cfg.tool.generator)
    if configured:
        return configured
    # Windows has no default single-config generator worth using here: the old build.b
    return "Ninja" if IS_WINDOWS else ""


def cache_value(build_dir, key):
    """Read *key* from a build tree's CMakeCache.txt, or None."""
    cache = Path(build_dir) / "CMakeCache.txt"
    if not cache.is_file():
        return None
    try:
        for line in cache.read_text(encoding="utf-8",
                                    errors="replace").splitlines():
            name, _, value = line.partition("=")
            if name.split(":")[0] == key:
                return value.strip()
    except OSError:
        return None
    return None


def stale_tree(build_dir, root):
    """True when the tree was configured against a different source directory.

    A build tree carried over from elsewhere (a Docker volume build reused on the
    host) has absolute paths baked into its cache that no longer exist, so CTest's
    EXISTS guards fail and ctest reports "No tests were found" even though the
    sources are fine. Detecting it lets the caller reconfigure instead.
    """
    home = cache_value(build_dir, "CMAKE_HOME_DIRECTORY")
    if not home:
        return False
    return os.path.normcase(os.path.normpath(home)) != \
        os.path.normcase(os.path.normpath(str(root)))


def _stamp_key(build_type, tests, generator_name, defines):
    return "|".join([build_type, "ON" if tests else "OFF", generator_name,
                     *sorted(defines)])


def _needs_configure(build_dir, generator_name, stamp_key, root):
    if not build_dir.is_dir():
        return True
    sentinel = "build.ninja" if generator_name == "Ninja" else "Makefile"
    if generator_name in ("Ninja", "Unix Makefiles") \
            and not (build_dir / sentinel).is_file():
        return True
    if not (build_dir / "CMakeCache.txt").is_file():
        return True
    if stale_tree(build_dir, root):
        console.warn("Build tree was configured for a different source path "
                     "(e.g. a Docker volume build); reconfiguring from scratch.")
        return True
    stamp = build_dir / STAMP_NAME
    if stamp.is_file():
        return stamp.read_text(encoding="utf-8").strip() != stamp_key
    # No stamp (a pre-upgrade tree, or a hand-run cmake): reconfigure once so the tree
    return True


def build(cfg, paths, build_type=None, clean=False, jobs=None, tests=True,
          defines=()):
    """Configure (when needed) and build. Returns an exit code."""
    console.header("Build")
    build_dir = paths.build
    build_type = build_type or normalize_build_type(cfg.build_type)
    generator_name = generator(cfg)
    cmake = cmake_exe(cfg)

    if clean:
        if build_dir.exists():
            console.info(f"Removing build directory: {build_dir}")
            shutil.rmtree(build_dir, ignore_errors=True)

    console.info(f"Type: {build_type}  |  tests: {'ON' if tests else 'OFF'}"
                 + (f"  |  generator: {generator_name}" if generator_name else ""))

    env = load_build_env(build_dir)
    stamp_key = _stamp_key(build_type, tests, generator_name, defines)

    if _needs_configure(build_dir, generator_name, stamp_key, paths.root):
        # Reconfigured from an empty tree: a cached option flip (tests ON->OFF, or a diffe
        if build_dir.exists():
            shutil.rmtree(build_dir, ignore_errors=True)
        build_dir.mkdir(parents=True, exist_ok=True)

        options = [f"-DCMAKE_BUILD_TYPE={build_type}", "-DBUILD_MOT_TEST=OFF"]
        options += [f"-D{option}={'ON' if tests else 'OFF'}"
                    for option in TEST_OPTIONS]
        options += list(defines)

        configure = [cmake, "-S", str(paths.root), "-B", str(build_dir)]
        if generator_name:
            configure += ["-G", generator_name]
        configure += options

        console.info("Configuring...")
        code = proc.run(configure, cwd=paths.root, env=env)
        if code != 0:
            console.error("CMake configuration failed.")
            return code
        (build_dir / STAMP_NAME).write_text(stamp_key, encoding="utf-8")
    else:
        console.info("Build tree already configured for this configuration; "
                     "skipping cmake configure.")

    targets = [LIBRARY_TARGET] + (list(TEST_TARGETS) if tests else [])
    command = [cmake, "--build", str(build_dir), "--config", build_type,
               "--parallel", str(jobs or cfg.jobs())]
    for target in targets:
        command += ["--target", target]

    console.info("Building...")
    code = proc.run(command, cwd=paths.root, env=env)
    if code != 0:
        console.error("Build failed.")
        return code
    console.success(f"Build completed ({', '.join(targets)}).")
    return 0


def clean(paths):
    """Remove the build tree and the assembled package directory."""
    console.header("Clean")
    removed = []
    for target in (paths.build, paths.package):
        if target.exists():
            console.info(f"Removing {target}")
            shutil.rmtree(target, ignore_errors=True)
            removed.append(Path(target).name)
    if not removed:
        console.info("Nothing to remove; the tree is already clean.")
    else:
        console.success(f"Removed: {', '.join(removed)}.")
    return 0
