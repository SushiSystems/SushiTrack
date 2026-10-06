# test_process.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""What st's process runner does, pinned against sushicore's Runner."""

from __future__ import annotations

import os
import subprocess
import sys

from conftest import RecordingConsole
from sushitrack_cli import process

MISSING = "definitely-not-a-program-4b7e"


def _runner():
    """Build a runner that records instead of printing."""
    spy = RecordingConsole()
    return process.make_runner(spy), spy


def test_run_returns_the_child_exit_code(tmp_path):
    runner, _ = _runner()
    assert runner.run([sys.executable, "-c", "raise SystemExit(7)"], tmp_path) == 7


def test_run_echoes_the_command(tmp_path):
    runner, spy = _runner()
    runner.run([sys.executable, "-c", "pass"], tmp_path)
    assert spy.said('-c pass')


def test_a_missing_executable_keeps_st_exit_code(tmp_path):
    runner, spy = _runner()
    assert runner.run([MISSING], tmp_path) == 127
    assert spy.said("Executable not found")


def test_an_interrupted_child_returns_130(tmp_path, monkeypatch):
    def interrupt(*_args, **_kwargs):
        raise KeyboardInterrupt

    monkeypatch.setattr(subprocess, "run", interrupt)
    runner, spy = _runner()
    assert runner.run([sys.executable, "-c", "pass"], tmp_path) == 130
    assert ("warn", "Interrupted.") in spy.lines


def test_run_drained_returns_the_child_exit_code(tmp_path):
    runner, _ = _runner()
    assert runner.run_drained([sys.executable, "-c", "raise SystemExit(3)"], tmp_path) == 3


def test_run_drained_reports_a_missing_executable(tmp_path):
    runner, spy = _runner()
    assert runner.run_drained([MISSING], tmp_path) == 127
    assert spy.said("Executable not found")


def test_capture_returns_the_triple_and_stays_quiet():
    runner, spy = _runner()
    code, out, err = runner.capture([sys.executable, "-c", "print('hello')"])
    assert (code, out.strip(), err) == (0, "hello", "")
    assert spy.lines == []


def test_capture_reports_a_missing_executable_in_its_stderr():
    runner, _ = _runner()
    code, out, err = runner.capture([MISSING])
    assert (code, out) == (127, "")
    assert "Executable not found" in err


def test_resolve_prefers_the_path_the_caller_handed_over(tmp_path):
    name = "st_probe.exe" if os.name == "nt" else "st_probe"
    planted = tmp_path / name
    planted.write_text("", encoding="utf-8")
    planted.chmod(0o755)
    found = process.RUNNER.resolve_exe(planted.stem, {"PATH": str(tmp_path)})
    assert os.path.samefile(found, planted)
