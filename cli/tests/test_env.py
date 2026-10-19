# test_env.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""What st's build environment does, now composed from sushicore.build_env."""

from __future__ import annotations

import pytest

import json
import os
from types import SimpleNamespace

from sushitrack_cli import env

pytestmark = pytest.mark.medium


def _tool(windows=True, vcvars="C:/vs/vcvars64.bat"):
    """Build the slice of ToolSettings env.py reads."""
    return SimpleNamespace(platform="windows" if windows else "linux",
                           vs_vcvars=vcvars, is_windows=windows,
                           expand=lambda value: value)


def _no_vcvars_run(monkeypatch):
    """Fail the test if a vcvars snapshot is attempted."""
    monkeypatch.setattr(env, "snapshot_vcvars",
                        lambda *a: (_ for _ in ()).throw(AssertionError("vcvars ran")))


def _detected_vcvars(monkeypatch):
    """Make detection find a vcvars64.bat, as on a machine with Visual Studio."""
    monkeypatch.setattr(env, "find_vcvars", lambda: "C:/vs/detected/vcvars64.bat")


def test_off_windows_the_current_environment_is_used_as_is(tmp_path, monkeypatch):
    """Snapshots nothing off Windows."""
    _no_vcvars_run(monkeypatch)
    assert env.load_build_env(_tool(windows=False), tmp_path) == dict(os.environ)


def test_a_cached_snapshot_is_read_back(tmp_path, monkeypatch):
    """Runs vcvars once per build tree."""
    _no_vcvars_run(monkeypatch)
    tool = _tool()
    (tmp_path / env.CACHE_NAME).write_text(json.dumps(
        {"key": env.cache_key(tool), "env": {"INCLUDE": "C:/from-cache"}}), encoding="utf-8")
    assert env.load_build_env(tool, tmp_path)["INCLUDE"] == "C:/from-cache"


def test_a_cache_from_another_vcvars_is_ignored(tmp_path, monkeypatch):
    """Rejects a snapshot whose key no longer matches."""
    monkeypatch.delenv("VSINSTALLDIR", raising=False)
    _detected_vcvars(monkeypatch)
    monkeypatch.setattr(env, "snapshot_vcvars", lambda path, out: {"INCLUDE": "C:/fresh"})
    (tmp_path / env.CACHE_NAME).write_text(json.dumps(
        {"key": "stale", "env": {"INCLUDE": "C:/gone"}}), encoding="utf-8")
    assert env.load_build_env(_tool(), tmp_path)["INCLUDE"] == "C:/fresh"


def test_an_unreadable_cache_is_rebuilt_rather_than_fatal(tmp_path, monkeypatch):
    """Costs a vcvars run, not the build."""
    monkeypatch.delenv("VSINSTALLDIR", raising=False)
    _detected_vcvars(monkeypatch)
    monkeypatch.setattr(env, "snapshot_vcvars", lambda path, out: {"INCLUDE": "C:/fresh"})
    (tmp_path / env.CACHE_NAME).write_text("{ not json", encoding="utf-8")
    assert env.load_build_env(_tool(), tmp_path)["INCLUDE"] == "C:/fresh"


def test_a_fresh_snapshot_is_written_to_the_cache(tmp_path, monkeypatch):
    """Keeps what was learned under the current key."""
    monkeypatch.delenv("VSINSTALLDIR", raising=False)
    _detected_vcvars(monkeypatch)
    monkeypatch.setattr(env, "snapshot_vcvars", lambda path, out: {"LIB": "C:/libs"})
    tool = _tool()
    env.load_build_env(tool, tmp_path)
    cached = json.loads((tmp_path / env.CACHE_NAME).read_text(encoding="utf-8"))
    assert cached == {"key": env.cache_key(tool), "env": {"LIB": "C:/libs"}}


def test_no_snapshot_means_the_environment_is_left_alone(tmp_path, monkeypatch):
    """Changes nothing when vcvars fails."""
    monkeypatch.delenv("VSINSTALLDIR", raising=False)
    _detected_vcvars(monkeypatch)
    monkeypatch.setattr(env, "snapshot_vcvars", lambda path, out: None)
    assert env.load_build_env(_tool(), tmp_path) == dict(os.environ)
    assert not (tmp_path / env.CACHE_NAME).exists()


def test_a_developer_shell_is_not_snapshotted_again(tmp_path, monkeypatch):
    """Skips vcvars inside a VS developer shell."""
    monkeypatch.setenv("VSINSTALLDIR", "C:/vs")
    _no_vcvars_run(monkeypatch)
    assert env.load_build_env(_tool(), tmp_path) == dict(os.environ)


def test_no_visual_studio_warns_and_uses_the_current_env(tmp_path, monkeypatch, recorder):
    """Warns once and carries on when no vcvars exists."""
    spy = recorder(env)
    monkeypatch.delenv("VSINSTALLDIR", raising=False)
    monkeypatch.setattr(env, "find_vcvars", lambda: None)
    _no_vcvars_run(monkeypatch)
    assert env.load_build_env(_tool(vcvars=""), tmp_path) == dict(os.environ)
    assert spy.said("No Visual Studio installation detected")


def test_a_configured_vcvars_wins_over_detection(monkeypatch, tmp_path):
    """Prefers cfg.tool.vs_vcvars when it names a file."""
    bat = tmp_path / "vcvars64.bat"
    bat.write_text("", encoding="utf-8")
    monkeypatch.setattr(env, "find_vcvars", lambda: tmp_path / "other.bat")
    assert env.vcvars_path(_tool(vcvars=str(bat))) == bat
