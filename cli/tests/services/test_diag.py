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


def _status_row(session, tmp_path, item):
    """Run the status report over an empty tree and return its row named *item*."""
    paths = SimpleNamespace(root=tmp_path, build=tmp_path / "build", package=tmp_path / "package",
                            benchmark_names=lambda: [], data_root=lambda name: tmp_path / name)
    assert diag.status(SimpleNamespace(), paths, workspace=session.workspace) == 0
    (call,) = [call for call in session.console.calls if call.kind == "table"]
    (row,) = [row for row in call.args[1] if row[0] == item]
    return row


def _repository_detail(session, tmp_path):
    """Return the detail cell of the repository row of an empty tree's status."""
    return _status_row(session, tmp_path, "repository")[2]


def test_status_shows_a_missing_build_tree_as_absent(tmp_path, session):
    """Marks the build tree absent and still returns 0."""
    assert "absent" in _status_row(session, tmp_path, "build tree")[1]


def test_status_names_the_checkout_folder_when_git_is_missing(tmp_path, session):
    """Shows the repository path in place of a revision when git is not installed."""
    session.runner.script(127)

    assert str(tmp_path) in _repository_detail(session, tmp_path)


def test_status_joins_branch_commit_and_dirt(tmp_path, session):
    """Shows "branch @ commit (dirty)" for a branch with a changed file."""
    session.runner.script(0, ("main",)).script(0, ("abc123",)).script(0, (" M x",))

    assert "main @ abc123 (dirty)" in _repository_detail(session, tmp_path)


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
