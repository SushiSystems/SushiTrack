# process.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""The one sushicore Runner every st service spawns children through."""

from __future__ import annotations

from sushicore.proc import Runner

from . import console
from .config import PROFILE

#: Exit code st returns when a tool is missing.
MISSING_EXIT_CODE = 127


def make_runner(output) -> Runner:
    """Build a Runner that prints through *output* with st's failure codes.

    Args:
        output: A console module, or a test's recorder.

    Returns:
        A Runner named after `st` that turns Ctrl+C into exit code 130.
    """
    return Runner(output, PROFILE.program, missing_exit_code=MISSING_EXIT_CODE,
                  catch_interrupt=True)


RUNNER = make_runner(console)
