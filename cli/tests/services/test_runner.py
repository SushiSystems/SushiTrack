# test_runner.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""`st run` finds the program through the shared index and runs it."""

from __future__ import annotations

import dataclasses
from types import SimpleNamespace

import pytest

from sushitrack_cli.services import artifacts, runner

pytestmark = pytest.mark.medium


def test_run_matches_through_the_shared_index(tmp_path, session):
    program = tmp_path / "bin" / artifacts.executable_name("t")
    program.parent.mkdir()
    program.write_text("", encoding="utf-8")
    workspace = dataclasses.replace(session.workspace, environment=None)
    cfg = SimpleNamespace(tool=SimpleNamespace(is_windows=False))
    paths = SimpleNamespace(root=tmp_path, build=tmp_path, config=tmp_path / "sushitrack.json")

    assert runner.run(cfg, paths, target="t", forwarded=("--a",), workspace=workspace) == 0

    (command,) = session.runner.commands
    assert list(command.argv) == [str(program), "--a"]
    assert command.env["SUSHITRACK_CONFIG"] == str(paths.config)
