# test_diag.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""Tests diag's git probe and env dump on the shared bricks."""

from __future__ import annotations

import dataclasses
from types import SimpleNamespace

import pytest

from sushitrack_cli import library_packages
from sushitrack_cli.config import Paths, load_config
from sushitrack_cli.services import diag

pytestmark = pytest.mark.medium


def test_git_missing_answers_none(tmp_path, session):
    session.runner.script(127)

    assert diag._git_describe(session.workspace, SimpleNamespace(root=tmp_path)) is None


def test_git_describe_joins_branch_commit_and_dirt(tmp_path, session):
    session.runner.script(0, ("main",)).script(0, ("abc123",)).script(0, (" M x",))

    described = diag._git_describe(session.workspace, SimpleNamespace(root=tmp_path))

    assert described == "main @ abc123 (dirty)"


def test_env_dump_shows_the_run_dirs_of_the_tree_first_on_the_search_path(tmp_path, session):
    library_packages.record_run_dirs(tmp_path, ["/store/pkg/lib"])
    workspace = dataclasses.replace(session.workspace, environment=None)
    tool = SimpleNamespace(is_windows=False)

    assert diag.env_dump(SimpleNamespace(tool=tool), SimpleNamespace(build=tmp_path),
                         show_all=True, workspace=workspace) == 0

    (call,) = [call for call in session.console.calls if call.kind == "table"]
    shown = dict(map(tuple, call.args[1]))
    assert shown["LD_LIBRARY_PATH"].startswith("/store/pkg/lib")


def test_diagnostics_reads_the_context_once_per_report(tmp_path, session):
    cfg = load_config(tmp_path)
    paths = Paths(cfg, tmp_path)
    reads = []
    diagnostics = diag.Diagnostics(lambda: reads.append(1) or (cfg, paths), session.workspace)

    assert diagnostics.config_show() == 0
    assert diagnostics.env_dump(show_all=True) == 0

    assert len(reads) == 2
