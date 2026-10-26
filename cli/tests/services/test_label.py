# test_label.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""What label's benchmark run hands to its build environment."""

from __future__ import annotations

import pytest

import json
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


def test_parse_perf_warns_on_a_line_it_cannot_read(recorder):
    """Skips a malformed PERF line, keeps the good one and says which line it skipped."""
    spy = recorder(label)
    out = ("[ PERF sushitrack ] S1: not-a-number us/frame\n"
           "[ PERF sushitrack ] S2: 12.5 us/frame  (3 frames)")
    assert label._parse_perf(out) == {"sushitrack": {"S2": 12.5}}
    assert spy.said("Skipped a PERF line that does not parse")
    assert spy.said("S1")


def test_benchmark_args_pass_the_profiles_folder_when_given(tmp_path):
    paths = SimpleNamespace(bridge=tmp_path / "bridge")
    args = label._benchmark_args(paths, tmp_path / "data", ["S1"], ["sushitrack"],
                                 tmp_path / "profiles")
    assert args[:4] == ["--data", str(tmp_path / "data"), "--settings", str(tmp_path / "bridge")]
    assert ["--profiles", str(tmp_path / "profiles")] == args[4:6]


def _record(path, lengths_ns):
    """Write a record holding one frame period of each length in *lengths_ns*."""
    periods, start = [], 0
    for index, length in enumerate(lengths_ns):
        periods.append({"index": index, "timeline": 1, "warmup": False,
                        "begin_ns": start, "end_ns": start + length})
        start += length
    path.write_text(json.dumps({"sushiprofiler_record": 2, "names": ["", "sushitrack.frame"],
                                "periods": periods}), encoding="utf-8")


def test_a_profile_record_replaces_the_perf_figure_and_prints_its_quartiles(tmp_path, recorder):
    """The median of the record wins over the PERF line; a pair with no record keeps its figure."""
    spy = recorder(label)
    _record(tmp_path / "sushitrack_S1.sprec.json", [10_000, 20_000, 30_000, 40_000, 50_000])
    perf = {"sushitrack": {"S1": 99.0, "S2": 7.0}}
    result = label._use_records(tmp_path, ["sushitrack"], ["S1", "S2"], perf)
    assert result == {"sushitrack": {"S1": 30.0, "S2": 7.0}}
    assert spy.said("median 30.0 us/frame (quartiles 20.0 to 40.0, 5 frames)")


def test_a_pair_with_neither_record_nor_perf_has_no_timing(tmp_path, recorder):
    """Leaves the pair out, so the report prints N/A and not a zero."""
    recorder(label)
    assert label._use_records(tmp_path, ["sushitrack"], ["S1"], {}) == {}
