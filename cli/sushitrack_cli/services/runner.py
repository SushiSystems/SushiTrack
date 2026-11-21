# runner.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""Run a built executable, by name or by picking one interactively."""

from sushicore.run_program import run_program

from ..workspace import current_workspace
from .artifacts import EXECUTABLES


def run(cfg, paths, target=None, pick=False, list_all=False, forwarded=(), workspace=None):
    """Run *target* (exact name, then substring match), or the program picked. Returns its exit code.

    With no target the pick runs, as `st run` always did.

    Args:
        pick: Prompts for one built example from a list and ignores *target*.
        list_all: Makes the prompt list every program, not only the examples.
        workspace: What the run prints and starts the program through; none uses the real ones.
    """
    space = workspace or current_workspace()
    return run_program(
        EXECUTABLES, space.console, space.runner, build_dir=paths.build, project_root=paths.root,
        env=space.build_env(cfg.tool, paths.build, paths.config), target=target,
        pick=pick or not target, list_all=list_all, app_args=forwarded)
