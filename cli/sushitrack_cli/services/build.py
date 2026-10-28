# build.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Configure, build and clean the project."""

import enum
import shutil
from pathlib import Path

from sushicore import build_admission, build_report, cmake_cache
from sushicore.build_tree import refuse_another_build_type, tree_dir
from sushicore.cmake_driver import CMakeDriver
from sushicore.provision.sushi_packages import PackageError

from .. import console, library_packages
from ..env import load_build_env
from ..config import K_DEFAULT_TREE
from ..errors import UnknownBuildTypeError
from ..process import RUNNER

BUILD_TYPES = ("Release", "Debug", "RelWithDebInfo", "MinSizeRel")


class BuildType(str, enum.Enum):
    """The `--type` choices for `st build`, lower-cased for the CLI surface."""

    release = "release"
    debug = "debug"
    relwithdebinfo = "relwithdebinfo"
    minsizerel = "minsizerel"


class Sanitizer(str, enum.Enum):
    """The `--sanitize` choices; each is a SUSHITRACK_SANITIZE value."""

    address = "address"
    thread = "thread"


#: The tree name of each sanitizer build; the plain tree is named K_DEFAULT_TREE.
K_SANITIZER_TREES = {Sanitizer.address: "asan", Sanitizer.thread: "tsan"}


def tree_name(sanitizer: Sanitizer | None) -> str:
    """Returns the name of the tree *sanitizer* builds in, the plain tree's for none."""
    return K_SANITIZER_TREES[sanitizer] if sanitizer is not None else K_DEFAULT_TREE


def tree_for(paths, build_type: str, sanitizer: Sanitizer | None = None) -> Path:
    """Returns the tree of *build_type* for *sanitizer*, `build/<name>-<type>` under the root."""
    return tree_dir(paths.root, tree_name(sanitizer), build_type)


def sanitizer_defines(sanitizer: Sanitizer | None) -> list[str]:
    """Returns the -D option that turns *sanitizer* on, or nothing."""
    return [f"-DSUSHITRACK_SANITIZE={sanitizer.value}"] if sanitizer is not None else []

LIBRARY_TARGET = "sushitrack"
# TEST_TARGETS names every target the test build produces.
TEST_TARGETS = ("sushitrack_tests", "sushitrack_application_tests")
#: The applications every build makes: the inference bridge and the MOT benchmark.
APPLICATION_TARGETS = ("tracker_bridge", "mot_engine")
# The CMake options those targets are gated behind.
TEST_OPTIONS = ("SUSHITRACK_BUILD_TESTS",)

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


def _stamp_key(tests, generator_name, defines):
    """Join the settings that force a reconfigure when they change.

    The build type is not among them: it picks the tree, so a tree never changes it.
    """
    return "|".join(["ON" if tests else "OFF", generator_name, *sorted(defines)])


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


def _configure_args(cfg, paths, build_dir, build_type, tests, generator_name, defines):
    """Return the configure argv st's build policy asks for."""
    args = [DRIVER.cmake(cfg.tool), "-S", str(paths.root), "-B", str(build_dir)]
    if generator_name:
        args += ["-G", generator_name]
    args += [f"-DCMAKE_BUILD_TYPE={build_type}"]
    args += [f"-D{option}={'ON' if tests else 'OFF'}" for option in TEST_OPTIONS]
    return args + list(defines)


def build(cfg, paths, build_type=None, clean=False, jobs=None, tests=True,
          defines=(), sanitizer=None, report=False):
    """Configure when needed, then build st's targets in the tree of the build type.

    Args:
        sanitizer: Builds with this sanitizer, in the tree of its own.
        report: Measures the build and writes `build_report.json` into the tree.

    Returns:
        The exit code of the failing step, else 0.
    """
    console.header("Build")
    build_type = build_type or normalize_build_type(cfg.build_type)
    build_dir = tree_for(paths, build_type, sanitizer)
    generator_name = generator(cfg)
    defines = (*sanitizer_defines(sanitizer), *defines)
    refuse_another_build_type(build_dir, build_type, [item.removeprefix("-D") for item in defines])
    build_admission.refuse_a_stopped_gate(build_dir)

    if clean and build_dir.exists():
        console.info(f"Removing build directory: {build_dir}")
        if not _remove_tree(build_dir):
            return 1

    console.info(f"Tree: {build_dir}")
    console.info(f"Type: {build_type}  |  tests: {'ON' if tests else 'OFF'}"
                 + (f"  |  generator: {generator_name}" if generator_name else "")
                 + (f"  |  sanitizer: {sanitizer.value}" if sanitizer else ""))

    # The libraries are built packages made with this tree's compiler; CMake compiles none.
    compiler = cfg.tool.resolved_compiler(paths.root)
    try:
        packages = library_packages.resolve(
            cfg.tool, paths.root, build_type, compiler, console.info, console.warn)
    except PackageError as error:
        console.error(str(error))
        return 1
    defines = (*defines, *library_packages.compiler_arguments(compiler),
               *library_packages.configure_arguments(cfg.tool, paths.root, packages),
               *(f"-D{entry}" for entry in build_admission.launcher_defines(
                   build_dir, build_admission.compiler_cache(cfg.tool.ccache_exe))))
    stamp_key = _stamp_key(tests, generator_name, defines)

    configure = _needs_configure(build_dir, generator_name, stamp_key, paths.root)
    if configure and build_dir.exists() and not _remove_tree(build_dir):
        return 1
    # Recorded before the environment is read, and after a tree was removed, so it survives.
    library_packages.record_run_dirs(build_dir, packages.run_dirs(cfg.tool.platform))
    env = load_build_env(cfg.tool, build_dir)

    if configure:
        console.info("Configuring...")
        args = _configure_args(cfg, paths, build_dir, build_type, tests, generator_name,
                               defines)
        code = DRIVER.configure(args, paths.root, env)
        if code != 0:
            console.error("CMake configuration failed.")
            return code
        (build_dir / STAMP_NAME).write_text(stamp_key, encoding="utf-8")
    else:
        console.info("Build tree already configured for this configuration; "
                     "skipping cmake configure.")

    targets = [LIBRARY_TARGET, *APPLICATION_TARGETS] + (list(TEST_TARGETS) if tests else [])
    console.info("Building...")
    offset = build_report.ninja_log_offset(build_dir)
    code = DRIVER.compile(cfg.tool, build_dir, paths.root, env, config=build_type,
                          targets=targets, jobs=jobs or cfg.jobs())
    if report:
        measured = build_report.of_build(build_dir, offset, build_admission.state_dir(build_dir))
        for line in measured.lines():
            console.info(line)
        console.info(f"Report: {measured.write(build_dir)}")
    if code != 0:
        console.error("Build failed.")
        return code
    console.success(f"Build completed ({', '.join(targets)}).")
    return 0


def clean(paths):
    """Remove every build tree and the assembled package directory.

    Returns:
        0 when both are gone, 1 when one of them could not be removed.
    """
    console.header("Clean")
    removed = []
    for target in (paths.build_root, paths.package):
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
