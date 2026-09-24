# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License, Version 2.0. See LICENSE.
"""Locates what a SushiTrack build tree produced: libraries and executables."""

from __future__ import annotations

import os
from pathlib import Path
from typing import Iterator

from sushicore.discovery import ExecutableIndex

from ..config import IS_WINDOWS

_CONFIG_DIRS = ("", "Release", "Debug", "RelWithDebInfo")
_BIN_DIRS = ("bin", "lib", "")

#: Executables under the build tree; depth 16 stands in for st's unbounded walk.
EXECUTABLES = ExecutableIndex(skip_dirs=(".git",), max_depth=16)


def executable_name(name: str) -> str:
    """Return *name* with this platform's executable suffix."""
    return f"{name}.exe" if IS_WINDOWS else name


def shared_lib_name() -> str:
    """Return the runtime library's file name on this platform."""
    return "sushitrack.dll" if IS_WINDOWS else "libsushitrack.so"


def import_lib_name() -> str:
    """Return the library a C++ consumer links against on this platform."""
    return "sushitrack.lib" if IS_WINDOWS else "libsushitrack.so"


def _candidates(build_dir: Path, filename: str) -> Iterator[Path]:
    """Yield the conventional locations of *filename*, most likely first."""
    for bin_dir in _BIN_DIRS:
        for config in _CONFIG_DIRS:
            yield Path(build_dir, *(p for p in (bin_dir, config, filename) if p))


def find_file(build_dir: str | os.PathLike, filename: str) -> Path | None:
    """Return the path to *filename* under the build tree, or None.

    Args:
        build_dir: The build tree.
        filename: The exact file name.

    Returns:
        The first conventional hit, else the first recursive hit, else None.
    """
    build_dir = Path(build_dir)
    if not build_dir.is_dir():
        return None
    for candidate in _candidates(build_dir, filename):
        if candidate.is_file():
            return candidate
    found = sorted(build_dir.rglob(filename))
    return found[0] if found else None


def find_binary(build_dir: str | os.PathLike, name: str) -> Path | None:
    """Return the built executable *name*, adding the suffix on Windows."""
    return find_file(build_dir, executable_name(name))
