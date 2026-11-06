# test_infer.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""`st infer` launches the demo through the workspace's runner."""

from __future__ import annotations

import sys
from types import SimpleNamespace

import pytest

from sushitrack_cli.services import infer

pytestmark = pytest.mark.medium


def _args(source):
    """Return the options `st infer` parsed, all at their defaults, for *source*."""
    return SimpleNamespace(
        source=source, tracker=None, name="yolox-s", ckpt=None, device="gpu", conf=0.1, nms=0.45,
        tsize=None, num_classes=80, legacy=False, stress=False, no_save=False, save_name=None,
        mot_label=False, no_screen=False, reid=False, reid_model="m.onnx", forwarded=[])


def _checkout(tmp_path):
    """Return the paths of a checkout whose inference folder holds a demo.py."""
    inference = tmp_path / "inference"
    inference.mkdir()
    (inference / "demo.py").write_text("", encoding="utf-8")
    return SimpleNamespace(inference=inference, root=tmp_path, build=tmp_path,
                           config=tmp_path / "sushitrack.json")


def test_infer_launches_the_demo_on_the_source(tmp_path, session):
    cfg = SimpleNamespace(test_tracker="sushitrack", tool=SimpleNamespace())
    paths = _checkout(tmp_path)

    code = infer.run(cfg, paths, _args("clip.mp4"), workspace=session.workspace)

    assert code == 0
    (command,) = session.commands
    assert command[:2] == [sys.executable, str(paths.inference / "demo.py")]
    assert command[command.index("--source") + 1] == "clip.mp4"


def test_infer_without_a_source_exits_two_and_says_so(tmp_path, session):
    cfg = SimpleNamespace(test_tracker="sushitrack", tool=SimpleNamespace())

    code = infer.run(cfg, _checkout(tmp_path), _args(None), workspace=session.workspace)

    assert code == 2 and session.commands == []
    assert session.said("--source")
