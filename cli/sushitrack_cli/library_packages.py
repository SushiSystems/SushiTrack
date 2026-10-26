# library_packages.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Takes the Sushi libraries `st` links as built packages and puts their run-time folders on the path."""

from __future__ import annotations

from collections.abc import Callable, Sequence
from pathlib import Path

from sushicore.provision.sushi_packages import PackageSet, RunDirs
from sushicore.provision.sushi_packages import configure_arguments  # noqa: F401  (callers take it from here)
from sushicore.toolchain_args import c_compiler_for

#: The file of a build tree that names the run-time folders of the packages it linked.
K_RUN_DIRS_FILE = ".sushitrack_package_dirs.json"

#: The build types a library CLI does not take, and the one it is built as instead.
K_PACKAGE_BUILD_TYPE = {"minsizerel": "release"}


def package_build_type(build_type: str) -> str:
    """Return the build type the library packages are built in for a consumer's *build_type*.

    A library CLI's ``--type`` takes release, debug and relwithdebinfo, so a MinSizeRel build
    links release packages.
    """
    lowered = build_type.lower()
    return K_PACKAGE_BUILD_TYPE.get(lowered, lowered)


def compiler_arguments(compiler: str) -> list[str]:
    """Return the ``-D`` arguments that make the tree build with *compiler*, none when empty.

    The packages are built with this compiler, so the tree must be too.
    """
    if not compiler:
        return []
    return [f"-DCMAKE_CXX_COMPILER={compiler}", f"-DCMAKE_C_COMPILER={c_compiler_for(compiler)}"]


def resolve(tool, root: Path, build_type: str, compiler: str,
            report: Callable[[str], None], warn: Callable[[str], None]) -> PackageSet:
    """Return a built package of every library the project links, building a missing one.

    SushiTrack passes no option to a library.

    Args:
        tool: The resolved ``[tool]`` settings.
        root: The project root.
        build_type: The build type of this configure, in CMake's casing.
        compiler: The compiler of this build; the packages are built with it.
        report: Receives one ``reused`` or ``built`` line per library.
        warn: Receives the warnings of the resolver.

    Raises:
        sushicore.provision.sushi_packages.PackageError: A package cannot be chosen.
    """
    return tool.sushi_packages(root, package_build_type(build_type), {}, report=report,
                               warn=warn, compiler=compiler)


#: The run-time folders of the packages a tree linked, kept in the tree for the commands that run it.
_RUN_DIRS = RunDirs(K_RUN_DIRS_FILE)

def record_run_dirs(tree: Path, dirs: Sequence[str]) -> None:
    """Write the run-time folders of the packages *tree* linked, for the commands that run it."""
    _RUN_DIRS.save(tree, dirs)


def read_run_dirs(tree: Path) -> list[str]:
    """Return the folders :func:`record_run_dirs` wrote for *tree*, none when it wrote none."""
    return _RUN_DIRS.read(tree)


def prepend_run_dirs(tool, env: dict[str, str], dirs: Sequence[str]) -> None:
    """Put *dirs* first on the search path of *env*, where a binary finds the package libraries."""
    _RUN_DIRS.prepend(env, dirs, tool.is_windows)
