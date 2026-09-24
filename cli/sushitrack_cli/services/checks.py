# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License, Version 2.0. See LICENSE.
"""Lists SushiTrack's own doctor checks for the shared `st doctor` and `st setup`."""

from __future__ import annotations

from sushicore.provision.checks import path_check, python_module_check, tool_check
from sushicore.provision.doctor import Check, CheckResult, FunctionCheck, State

_PIP_NAMES = {"cv2": "opencv-python"}

_DATA_FIX = "place MOT sequences as described in docs/README.md#data-layout"


def _submodule(name, paths, folder, marker, group, required):
    """Return a check passing when the submodule under third_party/*folder* holds *marker*."""
    fix = f"git submodule update --init third_party/{folder}"
    return path_check(name, paths.third_party / folder / marker, group, required, fix)


def _python(module, group):
    """Return an optional check for *module* whose fix installs its pip package."""
    fix = f"pip install {_PIP_NAMES.get(module, module)}"
    return python_module_check(module, group, False, fix)


def _datasets(paths):
    """Return an optional check passing when a sequence with ground truth exists."""
    def fn():
        root = paths.data_root()
        found = sorted(root.glob("*/gt/gt.txt")) if root.is_dir() else []
        if found:
            return CheckResult(State.OK, f"{len(found)} sequence(s) under {root}")
        return CheckResult(State.FAIL, f"none found under {root}", _DATA_FIX)
    return FunctionCheck("datasets", "eval", False, fn)


def st_checks(paths) -> list[Check]:
    """Return every SushiTrack-specific check, grouped as build, infer and eval."""
    return [
        _submodule("eigen", paths, "eigen", "Eigen/Dense", "build", True),
        tool_check("docker", "docker", "build", False, "install Docker"),
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
