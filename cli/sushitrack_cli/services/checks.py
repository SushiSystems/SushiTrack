# checks.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Lists SushiTrack's own doctor checks for the shared `st doctor` and `st setup`."""

from __future__ import annotations

import platform

from sushicore.provision.checks import path_check, python_module_check, tool_check
from sushicore.provision.doctor import Check, CheckResult, FunctionCheck, State

_PIP_NAMES = {"cv2": "opencv-python"}

_DOCKER_WINDOWS_FIX = "winget install Docker.DockerDesktop"

_DOCKER_DOCS_FIX = "https://docs.docker.com/engine/install/"

_DATA_FIX = "place MOT sequences as docs/reference/DATA_LAYOUT.md describes"


def _submodule(name, paths, folder, marker, group, required):
    """Return a check passing when the submodule under third_party/*folder* holds *marker*."""
    fix = f"git submodule update --init third_party/{folder}"
    return path_check(name, paths.third_party / folder / marker, group, required, fix)


def _docker_fix():
    """Return the Docker install fix for the running platform."""
    return _DOCKER_WINDOWS_FIX if platform.system() == "Windows" else _DOCKER_DOCS_FIX


def _python(module, group):
    """Return an optional check for *module* whose fix installs its pip package."""
    fix = f"pip install {_PIP_NAMES.get(module, module)}"
    return python_module_check(module, group, False, fix)


def _datasets(paths):
    """Return an optional check passing when a sequence with ground truth exists."""
    def fn():
        counts = {name: len(list(paths.data_root(name).glob("*/gt/gt.txt")))
                  for name in paths.benchmark_names()}
        found = {name: count for name, count in counts.items() if count}
        if found:
            return CheckResult(State.OK, ", ".join(f"{name}: {count} sequence(s)"
                                                   for name, count in found.items()))
        return CheckResult(State.FAIL, f"none found under {paths.benchmarks}", _DATA_FIX)
    return FunctionCheck("datasets", "eval", False, fn)


def st_checks(paths) -> list[Check]:
    """Return every SushiTrack-specific check, grouped as build, infer and eval."""
    return [
        _submodule("eigen", paths, "eigen", "Eigen/Dense", "build", True),
        tool_check("docker", "docker", "build", False, _docker_fix()),
        _submodule("yolox", paths, "yolox", "setup.py", "infer", False),
        _python("torch", "infer"),
        _python("torchvision", "infer"),
        _python("cv2", "infer"),
        _python("pycocotools", "infer"),
        _submodule("TrackEval", paths, "TrackEval",
                   "scripts/run_mot_challenge.py", "eval", False),
        _datasets(paths),
        _python("numpy", "eval"),
        _python("scipy", "eval"),
    ]
