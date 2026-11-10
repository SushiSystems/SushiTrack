# test_label.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""What label's benchmark run hands to its build environment."""

from __future__ import annotations

import dataclasses
import json
import sys
from types import SimpleNamespace

import pytest

from sushitrack_cli.services import artifacts, label

pytestmark = pytest.mark.medium


K_PERF_LINE = "[ PERF sushitrack ] S1: 99.0 us/frame"


@pytest.fixture
def checkout(tmp_path):
    """Return the paths of a checkout with a built engine, two sequences and no TrackEval."""
    engine = tmp_path / "build" / "bin" / artifacts.executable_name(label.BENCHMARK)
    engine.parent.mkdir(parents=True)
    engine.write_text("", encoding="utf-8")
    data = tmp_path / "mot17" / "data"
    for name in ("S1", "S2"):
        (data / name / "gt").mkdir(parents=True)
        (data / name / "gt" / "gt.txt").write_text("", encoding="utf-8")
    return SimpleNamespace(root=tmp_path, build=tmp_path / "build", config=tmp_path / "st.json",
                           bridge=tmp_path / "bridge", benchmarks=tmp_path,
                           third_party=tmp_path / "third_party", data=data,
                           benchmark_names=lambda: ["mot17"], data_root=lambda name: data)


def _evaluate(session, checkout, **options):
    """Run the official-scoring route of `st eval`, which ends at the absent TrackEval script."""
    cfg = SimpleNamespace(tool=SimpleNamespace(is_windows=False))
    return label.evaluate_sequences(cfg, checkout, trackeval=True, workspace=session.workspace,
                                    **options)


def test_the_engine_runs_with_the_tool_settings_in_its_environment(checkout, session):
    """Hands the config path to the engine as SUSHITRACK_CONFIG."""
    workspace = dataclasses.replace(session.workspace, environment=None)
    cfg = SimpleNamespace(tool=SimpleNamespace(is_windows=False))

    label.evaluate_sequences(cfg, checkout, trackeval=True, workspace=workspace)

    (command, *_) = session.runner.commands
    assert command.env["SUSHITRACK_CONFIG"] == str(checkout.config)


def test_the_engine_stderr_is_not_read_as_timing(checkout, session):
    """Parses the PERF lines of stdout only, so a PERF-shaped line on stderr warns of nothing."""
    session.runner.script(0, (K_PERF_LINE,), stderr="[ PERF broken ] oops")

    _evaluate(session, checkout)

    assert not session.said("Skipped a PERF line")


def test_a_failed_engine_run_reports_its_stderr_and_scores_nothing(checkout, session):
    """Returns the engine's exit code and names its stderr."""
    session.runner.script(3, (), stderr="no detections")

    assert _evaluate(session, checkout) == 3
    assert session.said("mot_engine failed (exit 3): no detections")
    assert len(session.runner.commands) == 1


def test_the_engine_arguments_name_every_tracker_and_sequence(checkout, session):
    """Passes one --tracker per tracker and one --sequence per sequence, in order."""
    _evaluate(session, checkout, compare=True)

    (command, *_) = session.runner.commands
    assert list(command.argv[7:]) == [
        "--tracker", "sushitrack", "--tracker", "bytetrack", "--tracker", "ocsort",
        "--sequence", "S1", "--sequence", "S2"]


def test_the_engine_arguments_pass_the_data_settings_and_profiles_folders(checkout, session):
    """Starts the engine with --data, --settings and --profiles, in that order."""
    _evaluate(session, checkout)

    (command, *_) = session.runner.commands
    assert list(command.argv[1:7]) == ["--data", str(checkout.data), "--settings",
                                       str(checkout.bridge), "--profiles",
                                       str(checkout.build / "profiles")]


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


def test_a_line_the_engine_prints_that_is_not_timing_is_skipped_with_a_warning(checkout, session):
    """Skips a malformed PERF line, keeps the good one and says which line it skipped."""
    session.runner.script(0, ("[ PERF sushitrack ] S1: not-a-number us/frame", K_PERF_LINE))

    _evaluate(session, checkout)

    assert session.said("Skipped a PERF line that does not parse")
    assert session.said("S1: not-a-number")


def _record(path, lengths_ns):
    """Write a record holding one frame period of each length in *lengths_ns*."""
    periods, start = [], 0
    for index, length in enumerate(lengths_ns):
        periods.append({"index": index, "timeline": 1, "warmup": False,
                        "begin_ns": start, "end_ns": start + length})
        start += length
    path.write_text(json.dumps({"sushiprofiler_record": 2, "names": ["", "sushitrack.frame"],
                                "periods": periods}), encoding="utf-8")


@pytest.fixture
def evaluator(checkout, monkeypatch):
    """Put a stand-in TrackEval wrapper on the checkout; it saves the perf_fps it gets to a file."""
    folder = checkout.root / "evaluator"
    folder.mkdir()
    (folder / "trackeval_runner.py").write_text(
        "import json\n"
        "from pathlib import Path\n"
        "class TrackEvalRunner:\n"
        "    def __init__(self, data_root):\n"
        "        pass\n"
        "    def evaluate(self, trackers, scenarios, perf_fps):\n"
        "        Path(__file__).with_name('perf_fps.json').write_text(json.dumps(perf_fps))\n"
        "        return {'ok': 1}\n", encoding="utf-8")
    (folder / "report_generator.py").write_text(
        "def print_trackeval_table(results, compare=False):\n"
        "    pass\n"
        "def export_trackeval_results(results, directory):\n"
        "    pass\n", encoding="utf-8")
    monkeypatch.setattr(sys, "path", list(sys.path))
    monkeypatch.delitem(sys.modules, "trackeval_runner", raising=False)
    monkeypatch.delitem(sys.modules, "report_generator", raising=False)
    checkout.evaluator = folder
    return folder / "perf_fps.json"


def test_a_profile_record_replaces_the_perf_figure_and_prints_its_quartiles(
        checkout, session, evaluator):
    """The record median wins over the PERF line; a pair with no record keeps its PERF figure.

    The frame periods 10, 20, 30, 40 and 50 us have the median 30 us, so S1 scores 1e6 / 30 fps
    and not 1e6 / 99 from the PERF line. S2 has no record and its PERF line says 7 us: 1e6 / 7.
    """
    profiles = checkout.build / "profiles"
    profiles.mkdir()
    _record(profiles / "sushitrack_S1.sprec.json", [10_000, 20_000, 30_000, 40_000, 50_000])
    session.runner.script(0, (K_PERF_LINE, "[ PERF sushitrack ] S2: 7.0 us/frame"))

    assert label.evaluate_sequences(SimpleNamespace(tool=SimpleNamespace(is_windows=False)),
                                    checkout, workspace=session.workspace) == 0

    fps = json.loads(evaluator.read_text(encoding="utf-8"))["sushitrack"]
    assert fps == {"S1": pytest.approx(1e6 / 30.0), "S2": pytest.approx(1e6 / 7.0)}
    assert session.said("median 30.0 us/frame (quartiles 20.0 to 40.0, 5 frames)")


def test_a_pair_with_neither_record_nor_perf_has_no_timing(checkout, session, evaluator):
    """Leaves the pair out of the scored timing, so the report prints N/A and not a zero."""
    session.runner.script(0, (K_PERF_LINE,))

    assert label.evaluate_sequences(SimpleNamespace(tool=SimpleNamespace(is_windows=False)),
                                    checkout, workspace=session.workspace) == 0

    assert json.loads(evaluator.read_text(encoding="utf-8"))["sushitrack"] == {
        "S1": pytest.approx(1e6 / 99.0)}
