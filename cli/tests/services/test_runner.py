# test_runner.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""`st run` finds the program through the shared index and runs it."""

from __future__ import annotations

import dataclasses
from types import SimpleNamespace

import pytest
from sushicore.console_memory import RecordingConsole

from sushitrack_cli.services import artifacts, runner

pytestmark = pytest.mark.medium


def test_run_matches_through_the_shared_index(tmp_path, session):
    program = tmp_path / "bin" / artifacts.executable_name("t")
    program.parent.mkdir()
    program.write_text("", encoding="utf-8")
    program.chmod(0o755)
    workspace = dataclasses.replace(session.workspace, environment=None)
    cfg = SimpleNamespace(tool=SimpleNamespace(is_windows=False))
    paths = SimpleNamespace(root=tmp_path, build=tmp_path, config=tmp_path / "sushitrack.json")

    assert runner.run(cfg, paths, target="t", forwarded=("--a",), workspace=workspace) == 0

    (command,) = session.runner.commands
    assert list(command.argv) == [str(program), "--a"]
    assert command.env["SUSHITRACK_CONFIG"] == str(paths.config)


def _built(tmp_path, session, answer=None):
    """Returns a checkout with one registered example beside a test program, and the arguments."""
    examples = tmp_path / "applications" / "examples"
    examples.mkdir(parents=True)
    (examples / "CMakeLists.txt").write_text("add_executable(demo demo.cpp)\n", encoding="utf-8")
    build = tmp_path / "build"
    build.mkdir()
    for name in ("alpha_tests", "demo"):
        (build / artifacts.executable_name(name)).write_text("", encoding="utf-8")
        (build / artifacts.executable_name(name)).chmod(0o755)
    console = RecordingConsole(answer=answer)
    workspace = dataclasses.replace(session.workspace, environment=None, console=console)
    cfg = SimpleNamespace(tool=SimpleNamespace(is_windows=False))
    paths = SimpleNamespace(root=tmp_path, build=build, config=tmp_path / "sushitrack.json")
    return cfg, paths, workspace, console


def test_run_with_no_target_picks_among_the_registered_examples(tmp_path, session):
    cfg, paths, workspace, console = _built(tmp_path, session, answer="2")

    assert runner.run(cfg, paths, workspace=workspace) == 1

    assert session.runner.commands == []
    assert console.texts("error") == ["'2' is not a number from 1 to 1."]


def test_run_pick_with_all_reaches_a_program_that_is_not_an_example(tmp_path, session):
    cfg, paths, workspace, _ = _built(tmp_path, session, answer="1")

    assert runner.run(cfg, paths, pick=True, list_all=True, workspace=workspace) == 0

    assert session.runner.commands[0].argv[0].endswith(artifacts.executable_name("alpha_tests"))


def test_run_pick_beats_a_target_that_is_given(tmp_path, session):
    cfg, paths, workspace, _ = _built(tmp_path, session, answer="1")

    runner.run(cfg, paths, target="alpha", pick=True, workspace=workspace)

    assert session.runner.commands[0].argv[0].endswith(artifacts.executable_name("demo"))
