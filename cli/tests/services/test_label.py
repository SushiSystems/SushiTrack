# test_label.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""What label's benchmark run hands to its build environment."""

from __future__ import annotations

import pytest

import sys
from types import SimpleNamespace

from sushitrack_cli.services import label

pytestmark = pytest.mark.medium


def test_run_benchmark_hands_the_tool_settings_to_the_build_env(tmp_path, monkeypatch):
    seen = []
    tool = SimpleNamespace()
    monkeypatch.setattr(label, "find_binary", lambda build, name: tmp_path / name)
    monkeypatch.setattr(label, "load_build_env",
                        lambda t, build, config: seen.append((t, build, config)) or {})
    monkeypatch.setattr(label, "RUNNER", SimpleNamespace(capture=lambda *a, **k: (0, "", "")))
    paths = SimpleNamespace(build=tmp_path, root=tmp_path, config=tmp_path / "sushitrack.json")
    label._run_benchmark(tool, paths, [])
    assert seen == [(tool, tmp_path, tmp_path / "sushitrack.json")]


def test_benchmark_capture_keeps_streams_apart(tmp_path, monkeypatch, recorder):
    recorder(label)
    monkeypatch.setattr(label, "find_binary", lambda build, name: sys.executable)
    monkeypatch.setattr(label, "load_build_env", lambda tool, d, config: None)
    out = "[ PERF bytetrack ] S: 5 us/frame"
    monkeypatch.setattr(label, "RUNNER",
                        SimpleNamespace(capture=lambda cmd, cwd=None, env=None: (0, out, "")))
    paths = SimpleNamespace(root=tmp_path, build=tmp_path, config=tmp_path)
    assert label._run_benchmark(SimpleNamespace(), paths, []) == (0, {"bytetrack": {"S": 5.0}})


def test_benchmark_args_name_every_tracker_and_sequence(tmp_path):
    paths = SimpleNamespace(bridge=tmp_path / "bridge")
    assert label._benchmark_args(paths, tmp_path / "data", ["S1", "S2"], ["sushitrack"]) == [
        "--data", str(tmp_path / "data"), "--settings", str(tmp_path / "bridge"),
        "--tracker", "sushitrack", "--sequence", "S1", "--sequence", "S2"]


def test_eval_fails_when_no_sequence_matches(tmp_path, recorder):
    """Returns 1 and says so when the scenario filter matches no sequence."""
    spy = recorder(label)
    paths = SimpleNamespace(benchmarks=tmp_path, benchmark_names=lambda: ["mot17"],
                            data_root=lambda name: tmp_path)
    assert label.evaluate_sequences(SimpleNamespace(), paths, scenario="MOT99") == 1
    assert spy.said("No sequence matched")


def test_eval_refuses_an_unknown_benchmark(tmp_path, recorder):
    """Returns 2 and names the benchmarks there are."""
    spy = recorder(label)
    paths = SimpleNamespace(benchmarks=tmp_path, benchmark_names=lambda: ["mot17", "mot20"],
                            data_root=lambda name: tmp_path)
    assert label.evaluate_sequences(SimpleNamespace(), paths, benchmark="mot99") == 2
    assert spy.said("Available: mot17, mot20")


def test_clean_predictions_removes_every_pred_folder(tmp_path, recorder):
    """Removes each sequence's pred/ folder and keeps its detections."""
    recorder(label)
    sequence = tmp_path / "mot17" / "data" / "MOT17-02-DPM"
    (sequence / "pred" / "sushitrack").mkdir(parents=True)
    (sequence / "det").mkdir()
    paths = SimpleNamespace(benchmark_names=lambda: ["mot17"],
                            data_root=lambda name: tmp_path / name / "data")
    assert label.clean_predictions(paths) == 0
    assert not (sequence / "pred").exists()
    assert (sequence / "det").exists()
