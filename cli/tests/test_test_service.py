# test_test_service.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""What `st test` hands ctest."""

from __future__ import annotations

from types import SimpleNamespace

import pytest
from sushicore.cmake_driver import CMakeDriver

from conftest import RecordingConsole
from sushitrack_cli.services import build as build_svc
from sushitrack_cli.services import tests as test_svc
from sushitrack_cli.services.test_label import TestLabel


class DrainRecorder:
    """Record drained argv instead of running it."""

    def __init__(self):
        """Start with no recorded calls."""
        self.calls = []

    def run_drained(self, cmd, cwd, env=None):
        """Record *cmd* and succeed."""
        self.calls.append(list(cmd))
        return 0


def _tree(tmp_path, cache):
    """Create a configured tree whose cache holds *cache*."""
    build = tmp_path / "build"
    build.mkdir()
    (build / "CMakeCache.txt").write_text(
        f"CMAKE_HOME_DIRECTORY:INTERNAL={tmp_path}\n{cache}", encoding="utf-8")
    return SimpleNamespace(root=tmp_path, build=build, config=tmp_path / "sushitrack.json")


@pytest.fixture
def rec(monkeypatch, recorder):
    """Swap in a recording runner."""
    recorder(test_svc)
    runner = DrainRecorder()
    monkeypatch.setattr(build_svc, "DRIVER", CMakeDriver(RecordingConsole(), runner))
    monkeypatch.setattr(test_svc, "load_build_env", lambda tool, d, config: {})
    return runner


_CFG = SimpleNamespace(build_type="Release",
                       tool=SimpleNamespace(ctest_exe="", expand=lambda v: v))


def test_the_default_runs_small_and_medium_without_the_quarantine(tmp_path, rec):
    paths = _tree(tmp_path, "CMAKE_BUILD_TYPE:STRING=Release\n")
    assert test_svc.test(_CFG, paths, jobs=2) == 0
    assert rec.calls == [["ctest", "--test-dir", str(paths.build), "--output-on-failure",
                          "-L", "^(small|medium)$", "-LE", "^quarantined$",
                          "--parallel", "2"]]


def test_naming_the_quarantine_runs_it_too(tmp_path, rec):
    paths = _tree(tmp_path, "CMAKE_BUILD_TYPE:STRING=Release\n")
    test_svc.test(_CFG, paths, labels=[TestLabel.LARGE, TestLabel.QUARANTINED])
    assert rec.calls[0][4:6] == ["-L", "^(large)$"]
    assert "-LE" not in rec.calls[0]


def test_multi_config_tree_gets_a_config(tmp_path, rec):
    paths = _tree(tmp_path, "")
    test_svc.test(_CFG, paths, filter="A.B", repeat=3)
    assert rec.calls[0][-2:] == ["-C", "Release"]
    assert rec.calls[0][8:12] == ["-R", "A.B", "--repeat", "until-fail:3"]


def test_a_stale_tree_is_refused(tmp_path, rec):
    build = tmp_path / "build"
    build.mkdir()
    (build / "CMakeCache.txt").write_text(
        "CMAKE_HOME_DIRECTORY:INTERNAL=/workspace\n", encoding="utf-8")
    paths = SimpleNamespace(root=tmp_path, build=build, config=tmp_path)
    assert test_svc.test(_CFG, paths) == 1
    assert rec.calls == []

