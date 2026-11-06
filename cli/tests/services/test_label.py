# test_label.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""What label's benchmark run hands to its build environment."""

from __future__ import annotations

import dataclasses
import json
from types import SimpleNamespace

import pytest

from sushitrack_cli.services import artifacts, label

pytestmark = pytest.mark.medium


def test_run_benchmark_hands_the_tool_settings_to_the_build_env(tmp_path, session):
    engine = tmp_path / "bin" / artifacts.executable_name(label.BENCHMARK)
    engine.parent.mkdir()
    engine.write_text("", encoding="utf-8")
    workspace = dataclasses.replace(session.workspace, environment=None)
    paths = SimpleNamespace(build=tmp_path, root=tmp_path, config=tmp_path / "sushitrack.json")

    label._run_benchmark(workspace, SimpleNamespace(is_windows=False), paths, [])

    (command,) = session.runner.commands
    environment = command.env
    assert environment["SUSHITRACK_CONFIG"] == str(paths.config)


def test_benchmark_capture_keeps_streams_apart(tmp_path, session):
    engine = tmp_path / "bin" / artifacts.executable_name(label.BENCHMARK)
    engine.parent.mkdir()
    engine.write_text("", encoding="utf-8")
    session.runner.script(0, ("[ PERF bytetrack ] S: 5 us/frame",))
    paths = SimpleNamespace(root=tmp_path, build=tmp_path, config=tmp_path)

    result = label._run_benchmark(session.workspace, SimpleNamespace(), paths, [])

    assert result == (0, {"bytetrack": {"S": 5.0}})


def test_benchmark_args_name_every_tracker_and_sequence(tmp_path):
    paths = SimpleNamespace(bridge=tmp_path / "bridge")
    assert label._benchmark_args(paths, tmp_path / "data", ["S1", "S2"], ["sushitrack"]) == [
        "--data", str(tmp_path / "data"), "--settings", str(tmp_path / "bridge"),
        "--tracker", "sushitrack", "--sequence", "S1", "--sequence", "S2"]


def test_eval_fails_when_no_sequence_matches(tmp_path, session):
    """Returns 1 and says so when the scenario filter matches no sequence."""
    paths = SimpleNamespace(benchmarks=tmp_path, benchmark_names=lambda: ["mot17"],
                            data_root=lambda name: tmp_path)
    assert label.evaluate_sequences(SimpleNamespace(), paths, scenario="MOT99",
                                   workspace=session.workspace) == 1
    assert session.said("No sequence matched")


def test_eval_refuses_an_unknown_benchmark(tmp_path, session):
    """Returns 2 and names the benchmarks there are."""
    paths = SimpleNamespace(benchmarks=tmp_path, benchmark_names=lambda: ["mot17", "mot20"],
                            data_root=lambda name: tmp_path)
    assert label.evaluate_sequences(SimpleNamespace(), paths, benchmark="mot99",
                                   workspace=session.workspace) == 2
    assert session.said("Available: mot17, mot20")


def test_clean_predictions_removes_every_pred_folder(tmp_path, session):
    """Removes each sequence's pred/ folder and keeps its detections."""
    sequence = tmp_path / "mot17" / "data" / "MOT17-02-DPM"
    (sequence / "pred" / "sushitrack").mkdir(parents=True)
    (sequence / "det").mkdir()
    paths = SimpleNamespace(benchmark_names=lambda: ["mot17"],
                            data_root=lambda name: tmp_path / name / "data")
    assert label.clean_predictions(paths, workspace=session.workspace) == 0
    assert not (sequence / "pred").exists()
    assert (sequence / "det").exists()


def test_parse_perf_warns_on_a_line_it_cannot_read(session):
    """Skips a malformed PERF line, keeps the good one and says which line it skipped."""
    out = ("[ PERF sushitrack ] S1: not-a-number us/frame\n"
           "[ PERF sushitrack ] S2: 12.5 us/frame  (3 frames)")
    assert label._parse_perf(out, session.console) == {"sushitrack": {"S2": 12.5}}
    assert session.said("Skipped a PERF line that does not parse")
    assert session.said("S1")


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


def test_a_profile_record_replaces_the_perf_figure_and_prints_its_quartiles(tmp_path, session):
    """The median of the record wins over the PERF line; a pair with no record keeps its figure."""
    _record(tmp_path / "sushitrack_S1.sprec.json", [10_000, 20_000, 30_000, 40_000, 50_000])
    perf = {"sushitrack": {"S1": 99.0, "S2": 7.0}}
    result = label._use_records(tmp_path, ["sushitrack"], ["S1", "S2"], perf, session.console)
    assert result == {"sushitrack": {"S1": 30.0, "S2": 7.0}}
    assert session.said("median 30.0 us/frame (quartiles 20.0 to 40.0, 5 frames)")


def test_a_pair_with_neither_record_nor_perf_has_no_timing(tmp_path, session):
    """Leaves the pair out, so the report prints N/A and not a zero."""
    assert label._use_records(tmp_path, ["sushitrack"], ["S1"], {}, session.console) == {}
