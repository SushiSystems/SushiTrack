"""Build-environment snapshotting (Windows MSVC).

A parent process cannot ``call vcvars64.bat`` and inherit the result, so the
script is run in a child ``cmd``, the resulting environment is dumped, and that
dictionary is reused for every cmake/ctest subprocess. This replaces the vswhere
block that used to live in ``cli/project/build.bat``.

Linux needs none of this — gcc/clang are already on PATH — so ``load_build_env``
returns ``os.environ`` unchanged there. The snapshot is cached inside the build
tree because spawning cmd and sourcing vcvars costs a noticeable second.
"""

import json
import os
from pathlib import Path

from . import console, proc
from .config import IS_WINDOWS

CACHE_NAME = ".sushitrack_env.json"

_VS_EDITIONS = ("BuildTools", "Community", "Professional", "Enterprise")
_VCVARS_REL = Path("VC") / "Auxiliary" / "Build" / "vcvars64.bat"


def _vswhere_path():
    program_files = os.environ.get("ProgramFiles(x86)") or os.environ.get(
        "ProgramFiles", r"C:\Program Files")
    return Path(program_files) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"


def _from_vswhere():
    vswhere = _vswhere_path()
    if not vswhere.is_file():
        return None
    code, out, _ = proc.capture([
        str(vswhere), "-latest", "-prerelease", "-products", "*",
        "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
        "-property", "installationPath",
    ])
    if code != 0:
        return None
    for line in out.splitlines():
        candidate = Path(line.strip()) / _VCVARS_REL
        if line.strip() and candidate.is_file():
            return candidate
    return None


def _from_disk_scan():
    """Scan the canonical install roots for vcvars64.bat.

    Needed when vswhere does not register a freshly installed VS — observed with
    build-tools-only configurations — so a working toolchain is not reported
    missing just because the registry lookup came back empty.
    """
    roots = [Path(os.environ[var]) / "Microsoft Visual Studio"
             for var in ("ProgramFiles", "ProgramFiles(x86)")
             if os.environ.get(var)]
    for root in roots:
        if not root.is_dir():
            continue
        for year in sorted((d for d in root.iterdir() if d.is_dir()),
                           key=lambda d: d.name, reverse=True):
            for edition in _VS_EDITIONS:
                candidate = year / edition / _VCVARS_REL
                if candidate.is_file():
                    return candidate
    return None


def find_vcvars():
    """Locate vcvars64.bat, or None when no Visual Studio install is found."""
    if not IS_WINDOWS:
        return None
    return _from_vswhere() or _from_disk_scan()


def _snapshot_windows():
    if os.environ.get("VSINSTALLDIR"):
        return None  # already inside a VS developer shell
    vcvars = find_vcvars()
    if vcvars is None:
        console.warn("No Visual Studio installation detected; "
                     "assuming the compiler is already on PATH.")
        return None
    console.info(f"Loading Visual Studio environment from {vcvars}")
    # Passed as one string, not a list: subprocess would re-quote the elements
    # and mangle the quotes around the space-containing vcvars path, leaving
    # cmd.exe with an unrecognised token and silently no VS environment.
    code, out, _ = proc.capture(f'cmd /c call "{vcvars}" >nul && set')
    if code != 0:
        console.warn("vcvars64.bat returned non-zero; using the current env.")
        return None
    snapshot = {}
    for line in out.splitlines():
        key, sep, value = line.partition("=")
        if sep and key:
            snapshot[key] = value
    return snapshot


def _merge(base, overlay):
    """Merge *overlay* into *base*, prepending PATH instead of replacing it.

    PATH is written back into whichever case variant *base* already uses
    (Windows' "Path"): adding a second "PATH" key leaves two conflicting entries
    and child processes then honour only one, so directories added to the other
    silently vanish from executable and DLL resolution.
    """
    merged = dict(base)
    path_key = next((k for k in merged if k.upper() == "PATH"), None)
    for key, value in overlay.items():
        if key.upper() == "PATH":
            target = path_key or key
            existing = merged.get(target, "")
            merged[target] = value + os.pathsep + existing if existing else value
            path_key = target
        else:
            merged[key] = value
    return merged


def load_build_env(build_dir):
    """Environment for build/test subprocesses, snapshotting MSVC on Windows."""
    if not IS_WINDOWS:
        return dict(os.environ)

    cache_file = Path(build_dir) / CACHE_NAME
    if cache_file.is_file():
        try:
            cached = json.loads(cache_file.read_text(encoding="utf-8"))
            return _merge(dict(os.environ), cached["env"])
        except (json.JSONDecodeError, OSError, KeyError):
            pass  # stale or unreadable; fall through and rebuild

    snapshot = _snapshot_windows()
    if not snapshot:
        return dict(os.environ)
    try:
        Path(build_dir).mkdir(parents=True, exist_ok=True)
        cache_file.write_text(json.dumps({"env": snapshot}), encoding="utf-8")
    except OSError:
        pass  # a read-only tree only costs us the caching, not the build
    return _merge(dict(os.environ), snapshot)
