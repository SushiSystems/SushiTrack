# test_runner.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""`st run` finds the program through the shared index and runs it."""

from __future__ import annotations

import pytest

from types import SimpleNamespace

from sushitrack_cli.services import runner

pytestmark = pytest.mark.medium


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
