"""Locate built binaries and libraries under the build tree.

Replaces the `find`/`dir` searches the old run.sh / run.bat scripts did, and the
duplicated candidate lists in the previous single-file CLI: single-config
generators put artifacts in ``bin/``, multi-config ones (MSBuild) add a
``Release/``/``Debug/`` level, so both shapes are probed before falling back to a
recursive walk.
"""

import os
from pathlib import Path

from ..config import IS_WINDOWS

_CONFIG_DIRS = ("", "Release", "Debug", "RelWithDebInfo")
_BIN_DIRS = ("bin", "lib", "")
_EXCLUDE_DIRS = {"CMakeFiles", "_deps", ".git", "vcpkg_installed"}


def executable_name(name):
    return f"{name}.exe" if IS_WINDOWS else name


def shared_lib_name():
    return "sushitrack.dll" if IS_WINDOWS else "libsushitrack.so"


def import_lib_name():
    # On Windows the runtime DLL and its import library are distinct files; on
    # Linux the single .so serves both roles.
    return "sushitrack.lib" if IS_WINDOWS else "libsushitrack.so"


def _candidates(build_dir, filename):
    for bin_dir in _BIN_DIRS:
        for config in _CONFIG_DIRS:
            yield Path(build_dir, *(p for p in (bin_dir, config, filename) if p))


def find_file(build_dir, filename):
    """Path to *filename* under the build tree, or None."""
    build_dir = Path(build_dir)
    if not build_dir.is_dir():
        return None
    for candidate in _candidates(build_dir, filename):
        if candidate.is_file():
            return candidate
    found = sorted(build_dir.rglob(filename))
    return found[0] if found else None


def find_binary(build_dir, name):
    """Path to the built executable *name* (extension added on Windows)."""
    return find_file(build_dir, executable_name(name))


def find_artifact(build_dir, filename):
    return find_file(build_dir, filename)


def list_executables(build_dir):
    """Executables under the build tree, sorted by name."""
    build_dir = Path(build_dir)
    if not build_dir.is_dir():
        return []
    found = []
    for dirpath, dirnames, filenames in os.walk(build_dir):
        dirnames[:] = [d for d in dirnames if d not in _EXCLUDE_DIRS]
        for filename in filenames:
            path = Path(dirpath) / filename
            if _is_executable(path):
                found.append(path)
    return sorted(found, key=lambda p: p.name.lower())


def _is_executable(path):
    if not path.is_file() or path.name.startswith("."):
        return False
    if IS_WINDOWS:
        return path.suffix.lower() == ".exe"
    if ".so" in path.name or path.suffix in (".a", ".o", ".cmake", ".txt",
                                             ".json", ".ninja", ".log"):
        return False
    return os.access(path, os.X_OK)


def match_by_name(build_dir, query):
    """Exact name/stem match first, then a substring match."""
    executables = list_executables(build_dir)
    for exe in executables:
        if query in (exe.name, exe.stem):
            return exe
    lowered = query.lower()
    for exe in executables:
        if lowered in exe.name.lower():
            return exe
    return None
