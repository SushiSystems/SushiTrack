# test_run_label_deploy.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""run, label and deploy reach the build tree through artifacts and process."""

from __future__ import annotations

import sys
from types import SimpleNamespace

from sushitrack_cli.services import deploy, label, runner


def test_run_matches_through_the_shared_index(tmp_path, monkeypatch, recorder):
    recorder(runner)
    seen = {}
    monkeypatch.setattr(runner, "EXECUTABLES",
                        SimpleNamespace(match=lambda build, q: tmp_path / "t.exe"))
    monkeypatch.setattr(runner, "load_build_env", lambda tool, d, config: {"X": "1"})
    monkeypatch.setattr(runner, "RUNNER", SimpleNamespace(
        run=lambda cmd, cwd, env=None: seen.update(cmd=cmd, env=env) or 0))
    cfg = SimpleNamespace(tool=SimpleNamespace())
    paths = SimpleNamespace(root=tmp_path, build=tmp_path, config=tmp_path)
    assert runner.run(cfg, paths, target="t", forwarded=("--a",)) == 0
    assert seen == {"cmd": [str(tmp_path / "t.exe"), "--a"], "env": {"X": "1"}}


def test_regression_capture_keeps_streams_apart(tmp_path, monkeypatch, recorder):
    recorder(label)
    monkeypatch.setattr(label, "find_binary", lambda build, name: sys.executable)
    monkeypatch.setattr(label, "load_build_env", lambda tool, d, config: None)
    out = "[ PERF bytetrack ] S: 5 us/frame"
    monkeypatch.setattr(label, "RUNNER",
                        SimpleNamespace(capture=lambda cmd, cwd=None, env=None: (0, out, "")))
    paths = SimpleNamespace(root=tmp_path, build=tmp_path, config=tmp_path)
    assert label._run_regression(SimpleNamespace(), paths, "*") == (0, {"bytetrack": {"S": 5.0}})


def test_deploy_no_longer_imports_discovery():
    assert not hasattr(deploy, "discovery")
    assert deploy.shared_lib_name() in ("sushitrack.dll", "libsushitrack.so")


def test_eval_fails_when_no_sequence_matches(tmp_path, recorder):
    """Returns 1 and says so when the scenario filter matches no sequence."""
    spy = recorder(label)
    paths = SimpleNamespace(data_root=lambda: tmp_path)
    assert label.evaluate_sequences(SimpleNamespace(), paths, scenario="MOT99") == 1
    assert spy.said("No sequence matched")
