# test_diag_service.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Tests diag's git probe and env dump on the shared bricks."""

from __future__ import annotations

from types import SimpleNamespace

from sushitrack_cli.services import diag


def test_git_missing_answers_none(tmp_path, monkeypatch):
    fake = SimpleNamespace(capture=lambda cmd, cwd=None, env=None: (127, "", ""))
    monkeypatch.setattr(diag, "RUNNER", fake)
    assert diag._git_describe(SimpleNamespace(root=tmp_path)) is None


def test_git_describe_joins_branch_commit_and_dirt(tmp_path, monkeypatch):
    answers = iter([(0, "main\n", ""), (0, "abc123\n", ""), (0, " M x\n", "")])
    fake = SimpleNamespace(capture=lambda cmd, cwd=None, env=None: next(answers))
    monkeypatch.setattr(diag, "RUNNER", fake)
    assert diag._git_describe(SimpleNamespace(root=tmp_path)) == "main @ abc123 (dirty)"


def test_env_dump_reads_the_tool_settings(tmp_path, monkeypatch, recorder):
    recorder(diag)
    seen = {}

    def fake_load(tool, build_dir):
        seen["tool"] = tool
        return {"PATH": "x"}

    monkeypatch.setattr(diag, "load_build_env", fake_load)
    tool = SimpleNamespace(name="tool")
    assert diag.env_dump(SimpleNamespace(tool=tool), SimpleNamespace(build=tmp_path)) == 0
    assert seen["tool"] is tool


def test_diagnostics_reads_the_context_once_per_report(monkeypatch):
    """Passes the context pair to each report and returns the report's code."""
    calls = []
    monkeypatch.setattr(diag, "config_show", lambda cfg, paths: calls.append((cfg, paths)) or 0)
    monkeypatch.setattr(diag, "env_dump",
                        lambda cfg, paths, show_all=False: calls.append(show_all) or 3)
    diagnostics = diag.Diagnostics(lambda: ("cfg", "paths"))
    assert diagnostics.config_show() == 0
    assert diagnostics.env_dump(show_all=True) == 3
    assert calls == [("cfg", "paths"), True]
