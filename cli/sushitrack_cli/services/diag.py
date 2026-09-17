"""Read-only diagnostics: status, doctor, config, env, paths."""

import os
import platform
import sys
from dataclasses import fields
from datetime import datetime
from pathlib import Path

from .. import console, proc
from ..config import CONFIG_NAMES, Config, ENV_OVERRIDES, IS_WINDOWS, config_dir
from ..env import find_vcvars, load_build_env
from . import build as build_svc
from . import discovery
from . import tests as test_svc

# Variable-name prefixes worth surfacing by default in `st env`: the ones that act
_ENV_OF_INTEREST = ("PATH", "CC", "CXX", "CMAKE", "NINJA", "INCLUDE", "LIB",
                    "LD_LIBRARY_PATH", "VCPKG", "CUDA", "ST_", "DS_", "PYTHON",
                    "VIRTUAL_ENV", "CONDA", "VSINSTALLDIR", "VSCMD",
                    "VCINSTALLDIR", "VCTOOLS", "WINDOWSSDK")

_TEST_BINARIES = ("unit_test", "integration_test", "regression_test")


def _mtime(path):
    try:
        return datetime.fromtimestamp(path.stat().st_mtime).strftime(
            "%Y-%m-%d %H:%M")
    except OSError:
        return "?"


def _git_describe(paths):
    if proc.which("git") is None:
        return None
    code, out, _ = proc.capture(["git", "rev-parse", "--abbrev-ref", "HEAD"],
                                cwd=paths.root)
    branch = out.strip() if code == 0 else "?"
    code, out, _ = proc.capture(["git", "rev-parse", "--short", "HEAD"],
                                cwd=paths.root)
    commit = out.strip() if code == 0 else "?"
    code, out, _ = proc.capture(["git", "status", "--porcelain"], cwd=paths.root)
    dirty = " (dirty)" if code == 0 and out.strip() else ""
    return f"{branch} @ {commit}{dirty}"


def _state(text, good):
    """A state cell colored by whether it is the healthy value."""
    return (text, console.theme.success if good else console.theme.warn)


def status(cfg, paths):
    console.header("Project status")

    table = console.new_table("Item", "State", "Detail")
    table.column_style(2, console.theme.muted)

    revision = _git_describe(paths)
    table.add_row("repository", paths.root.name, revision or str(paths.root))

    if paths.build.is_dir():
        build_type = build_svc.cache_value(paths.build, "CMAKE_BUILD_TYPE") or "?"
        generator = build_svc.cache_value(paths.build, "CMAKE_GENERATOR") or "?"
        table.add_row("build tree", _state("configured", True),
                      f"{build_type} / {generator} / {paths.build}")
    else:
        table.add_row("build tree", _state("absent", False),
                      f"{paths.build} — run `ds build`")

    library = discovery.find_artifact(paths.build, discovery.shared_lib_name())
    table.add_row("library", _state("built" if library else "missing", library),
                  f"{library} ({_mtime(library)})" if library
                  else discovery.shared_lib_name())

    for name in _TEST_BINARIES:
        binary = discovery.find_binary(paths.build, name)
        table.add_row(name, _state("built" if binary else "missing", binary),
                      f"{binary} ({_mtime(binary)})" if binary else "")

    bridge = discovery.find_artifact(
        paths.build, "tracker_bridge.dll" if IS_WINDOWS
        else "libtracker_bridge.so")
    table.add_row("tracker_bridge",
                  _state("built" if bridge else "missing", bridge),
                  str(bridge or "needed by `ds infer`"))

    data_root = paths.data_root()
    sequences = sorted(d.name for d in data_root.iterdir()
                       if d.is_dir() and (d / "gt" / "gt.txt").exists()) \
        if data_root.is_dir() else []
    table.add_row("datasets", _state(f"{len(sequences)} sequence(s)",
                                    bool(sequences)),
                  f"{data_root}" + (f" — {', '.join(sequences[:4])}"
                                    + (" ..." if len(sequences) > 4 else "")
                                    if sequences else ""))

    table.add_row("package",
                  ("present" if paths.package.is_dir() else "absent", None),
                  str(paths.package))
    console.table(table)
    return 0


def _check(name, ok, detail, required=True):
    return {"name": name, "ok": ok, "detail": detail, "required": required}


def _python_module(name):
    try:
        __import__(name)
        return True
    except ImportError:
        return False


def doctor(cfg, paths):
    """Verify the toolchain and data this project needs. Non-zero if any
    required check fails; optional checks only warn."""
    console.header("Doctor")
    checks = []

    checks.append(_check("python", sys.version_info >= (3, 8),
                         f"{platform.python_version()} at {sys.executable}"))

    # Checked against the same environment the build runs under, not the current shell
    build_env = load_build_env(paths.build)

    cmake = build_svc.cmake_exe(cfg)
    version = proc.tool_version(cmake, env=build_env)
    checks.append(_check("cmake", version is not None,
                         version or f"'{cmake}' not on PATH — required to build"))

    ctest = test_svc.ctest_exe(cfg)
    ctest_version = proc.tool_version(ctest, env=build_env)
    checks.append(_check("ctest", ctest_version is not None,
                         ctest_version or f"'{ctest}' not on PATH — "
                                          f"required by `ds test`"))

    generator = cfg.expand(cfg.build_generator) or ("Ninja" if IS_WINDOWS else "")
    if generator.lower().startswith("ninja"):
        ninja = proc.tool_version("ninja", env=build_env)
        # An already-configured tree pins its make program in the cache, so a build keeps 
        cached = build_svc.cache_value(paths.build, "CMAKE_MAKE_PROGRAM")
        from_cache = cached and Path(cached).is_file()
        checks.append(_check(
            "ninja", ninja is not None or bool(from_cache),
            ninja or (f"not on PATH; build tree uses {cached}" if from_cache
                      else f"not found — required by the '{generator}' generator")))

    if IS_WINDOWS:
        vcvars = os.environ.get("VSINSTALLDIR") or find_vcvars()
        checks.append(_check("msvc", vcvars is not None,
                             str(vcvars or "no Visual Studio C++ toolset found")))
    else:
        compiler = (proc.tool_version("c++", env=build_env)
                    or proc.tool_version("g++", env=build_env))
        checks.append(_check("c++ compiler", compiler is not None,
                             compiler or "no c++/g++ on PATH"))

    eigen = paths.third_party / "eigen" / "Eigen" / "Dense"
    checks.append(_check("eigen", eigen.is_file(),
                         str(eigen if eigen.is_file()
                             else f"{eigen} missing — git submodule update --init")))

    git = proc.tool_version("git")
    checks.append(_check("git", git is not None, git or "not on PATH",
                         required=False))

    docker = proc.tool_version("docker")
    checks.append(_check("docker", docker is not None,
                         docker or "not on PATH — only `st docker` needs it",
                         required=False))

    trackeval = paths.third_party / "TrackEval" / "scripts" / "run_mot_challenge.py"
    checks.append(_check("TrackEval submodule", trackeval.is_file(),
                         str(trackeval if trackeval.is_file()
                             else "missing — needed by `st eval --trackeval`"),
                         required=False))

    data_root = paths.data_root()
    sequences = [d for d in data_root.iterdir()
                 if d.is_dir() and (d / "gt" / "gt.txt").exists()] \
        if data_root.is_dir() else []
    checks.append(_check("datasets", bool(sequences),
                         f"{len(sequences)} sequence(s) under {data_root}"
                         if sequences else f"none found under {data_root}",
                         required=False))

    missing = [m for m in ("numpy", "scipy") if not _python_module(m)]
    checks.append(_check("evaluator deps", not missing,
                         "numpy, scipy available" if not missing
                         else f"missing: {', '.join(missing)} — only "
                              f"`st eval` needs them",
                         required=False))

    table = console.new_table("Check", "Result", "Detail")
    table.column_style(2, console.theme.muted)
    failures = 0
    for check in checks:
        if check["ok"]:
            result = ("ok", console.theme.success)
        elif check["required"]:
            result = ("FAIL", console.theme.error)
            failures += 1
        else:
            result = ("warn", console.theme.warn)
        table.add_row(check["name"], result, check["detail"])
    console.table(table)

    if failures:
        console.error(f"{failures} required check(s) failed; "
                      f"`st build` will not work until they are fixed.")
        return 1
    console.success("All required checks passed.")
    return 0


def config_show(cfg, paths):
    console.header("Resolved configuration")

    table = console.new_table("Setting", "Value", "Source")
    table.column_style(2, console.theme.muted)
    for field in fields(Config):
        if field.name == "sources":
            continue
        table.add_row(field.name, getattr(cfg, field.name),
                      cfg.sources.get(field.name, "default"))
    console.table(table)

    console.plain()
    console.info(f"Project root : {paths.root}")
    console.info(f"Config dir   : {config_dir(paths.root)}")
    for name in CONFIG_NAMES:
        path = config_dir(paths.root) / name
        console.muted(f"  {name:<20} [{'found' if path.is_file() else 'absent'}]")
    console.muted("  overrides: " + ", ".join(sorted(set(ENV_OVERRIDES.values()))))
    return 0


def paths_show(cfg, paths):
    console.header("Resolved paths")
    table = console.new_table("Name", "Path", "State")
    table.column_style(2, console.theme.muted)
    for name, path in paths.as_dict().items():
        table.add_row(name, path, "exists" if path.exists() else "missing")
    console.table(table)
    return 0


def env_dump(cfg, paths, show_all=False):
    console.header("Build environment")
    env = load_build_env(paths.build)

    def keep(key):
        return show_all or key.upper().startswith(_ENV_OF_INTEREST)

    shown = {k: v for k, v in sorted(env.items()) if keep(k)}
    table = console.new_table("Variable", "Value")
    for key, value in shown.items():
        table.add_row(key, value)
    console.table(table)

    console.plain()
    console.info(f"{len(shown)} of {len(env)} variables shown"
                 + ("" if show_all else
                    " (build-relevant; use --all for everything)"))
    return 0
