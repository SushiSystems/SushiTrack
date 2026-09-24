# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License, Version 2.0. See LICENSE.
"""Reports SushiTrack's status, resolved config, build env and paths."""

from dataclasses import fields
from datetime import datetime

from sushicore import cmake_cache

from .. import console
from ..config import (CONFIG_NAMES, ENV_OVERRIDES, IS_WINDOWS, PROFILE, Config,
                      config_dir)
from ..env import load_build_env
from ..process import RUNNER
from .artifacts import find_binary, find_file, shared_lib_name

# Variable-name prefixes worth surfacing by default in `st env`: the ones that act
_ENV_OF_INTEREST = ("PATH", "CC", "CXX", "CMAKE", "NINJA", "INCLUDE", "LIB",
                    "LD_LIBRARY_PATH", "VCPKG", "CUDA", "ST_", "PYTHON",
                    "VIRTUAL_ENV", "CONDA", "VSINSTALLDIR", "VSCMD",
                    "VCINSTALLDIR", "VCTOOLS", "WINDOWSSDK")

_TEST_BINARIES = ("unit_test", "integration_test", "regression_test")


def _mtime(path):
    """Return the file's modification time as text, or \"?\" when unreadable."""
    try:
        return datetime.fromtimestamp(path.stat().st_mtime).strftime(
            "%Y-%m-%d %H:%M")
    except OSError:
        return "?"


def _git_describe(paths):
    """Return "branch @ commit" with a dirty marker, or None without git."""
    code, out, _ = RUNNER.capture(["git", "rev-parse", "--abbrev-ref", "HEAD"],
                                  cwd=paths.root)
    if code == 127:
        return None
    branch = out.strip() if code == 0 else "?"
    code, out, _ = RUNNER.capture(["git", "rev-parse", "--short", "HEAD"], cwd=paths.root)
    commit = out.strip() if code == 0 else "?"
    code, out, _ = RUNNER.capture(["git", "status", "--porcelain"], cwd=paths.root)
    dirty = " (dirty)" if code == 0 and out.strip() else ""
    return f"{branch} @ {commit}{dirty}"


def _state(text, good):
    """A state cell colored by whether it is the healthy value, as Rich markup
    keyed to the theme sushicore's Console registers ("success" / "warn")."""
    style = "success" if good else "warn"
    return f"[{style}]{text}[/{style}]"


def _muted(text):
    """A de-emphasised detail cell. "dim" is a built-in Rich style, not a
    theme token — sushicore.theme.Theme carries no "muted" field, so this
    reaches for Rich's own vocabulary instead of one the shared theme lacks."""
    return f"[dim]{text}[/dim]"


def status(cfg, paths):
    """Print the project status table and return 0."""
    console.header("Project status")

    rows = []
    revision = _git_describe(paths)
    rows.append(["repository", paths.root.name, _muted(revision or str(paths.root))])

    if paths.build.is_dir():
        build_type = cmake_cache.cached_value(paths.build, "CMAKE_BUILD_TYPE") or "?"
        generator = cmake_cache.cached_value(paths.build, "CMAKE_GENERATOR") or "?"
        rows.append(["build tree", _state("configured", True),
                     _muted(f"{build_type} / {generator} / {paths.build}")])
    else:
        rows.append(["build tree", _state("absent", False),
                     _muted(f"{paths.build} — run `st build`")])

    library = find_file(paths.build, shared_lib_name())
    rows.append(["library", _state("built" if library else "missing", library),
                 _muted(f"{library} ({_mtime(library)})" if library
                        else shared_lib_name())])

    for name in _TEST_BINARIES:
        binary = find_binary(paths.build, name)
        rows.append([name, _state("built" if binary else "missing", binary),
                     _muted(f"{binary} ({_mtime(binary)})" if binary else "")])

    bridge = find_file(
        paths.build, "tracker_bridge.dll" if IS_WINDOWS
        else "libtracker_bridge.so")
    rows.append(["tracker_bridge",
                 _state("built" if bridge else "missing", bridge),
                 _muted(str(bridge or "needed by `st infer`"))])

    data_root = paths.data_root()
    sequences = sorted(d.name for d in data_root.iterdir()
                       if d.is_dir() and (d / "gt" / "gt.txt").exists()) \
        if data_root.is_dir() else []
    detail = f"{data_root}" + (f" — {', '.join(sequences[:4])}"
                               + (" ..." if len(sequences) > 4 else "")
                               if sequences else "")
    rows.append(["datasets", _state(f"{len(sequences)} sequence(s)",
                                    bool(sequences)), _muted(detail)])

    rows.append(["package", "present" if paths.package.is_dir() else "absent",
                 _muted(str(paths.package))])
    console.table(["Item", "State", "Detail"], rows, title="Project status")
    return 0


def config_show(cfg, paths):
    console.header("Resolved configuration")

    rows = [[field.name, str(getattr(cfg, field.name)),
             _muted(cfg.sources.get(field.name, "default"))]
            for field in fields(Config) if field.name not in ("sources", "tool")]
    rows += [[f"tool.{field.name}", str(getattr(cfg.tool, field.name)), _muted("tool")]
             for field in fields(cfg.tool)]
    console.table(["Setting", "Value", "Source"], rows, title="Resolved configuration")

    console.console.print()
    console.info(f"Project root : {paths.root}")
    console.info(f"Config dir   : {config_dir(paths.root)}")
    for name in CONFIG_NAMES:
        path = config_dir(paths.root) / name
        state = "found" if path.is_file() else "absent"
        # Parens, not brackets: Rich would try to parse a literal "[found]" as
        # a markup tag and raise, since "found" names no registered style.
        console.console.print(_muted(f"  {name:<20} ({state})"))
    names = set(ENV_OVERRIDES.values()) | set(PROFILE.env_overrides().values())
    console.console.print(_muted("  overrides: " + ", ".join(sorted(names))))
    return 0


def paths_show(cfg, paths):
    console.header("Resolved paths")
    rows = [[name, str(path), "exists" if path.exists() else "missing"]
            for name, path in paths.as_dict().items()]
    console.table(["Name", "Path", "State"], rows, title="Resolved paths")
    return 0


def env_dump(cfg, paths, show_all=False):
    console.header("Build environment")
    env = load_build_env(cfg.tool, paths.build)

    def keep(key):
        return show_all or key.upper().startswith(_ENV_OF_INTEREST)

    shown = {k: v for k, v in sorted(env.items()) if keep(k)}
    console.table(["Variable", "Value"], [[k, v] for k, v in shown.items()],
                  title="Build environment")

    console.console.print()
    console.info(f"{len(shown)} of {len(env)} variables shown"
                 + ("" if show_all else
                    " (build-relevant; use --all for everything)"))
    return 0
