# conftest.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026 Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Fixtures for the SushiTrack CLI's tests.

``sushitrack_cli`` imports :mod:`sushicore` at module load, so the suite cannot
be collected without it. Since 2026-09-22 sushicore is an ordinary PyPI
dependency of this CLI rather than a checkout to be found on disk, so there is
nothing to search for: it is installed or it is not.

Nothing here reaches the machine. A test that runs a real tool, prints through
the shared console, or reads the developer's own project is a test that says
something different on someone else's machine.
"""

from __future__ import annotations

import importlib.util
import sys
from pathlib import Path

import pytest

# The package under test sits in cli/, one level above this file, and the suite
# is run from the repository root as often as from cli/. Putting cli/ on the
# path here means the same command works from either.
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

if importlib.util.find_spec("sushicore") is None:  # pragma: no cover - import guard
    raise RuntimeError(
        "sushicore is not importable, so the sushitrack CLI cannot be imported. "
        "Install this CLI's dependencies: `pip install -e cli` or "
        "`pip install sushicore`.")


class RecordingConsole:
    """Collect what a module printed instead of printing it.

    The shared console is a Rich console bound to a stream, and under pytest's
    capture that stream can already be closed by the time a test writes to it.
    A test asserting on output wants the text anyway, so it records.
    """

    def __init__(self) -> None:
        self.lines: list[tuple[str, str]] = []
        self.console = self

    def _record(self, level: str):
        def say(text: object = "", *args: object, **kwargs: object) -> None:
            self.lines.append((level, str(text)))
        return say

    def __getattr__(self, name: str):
        """Answer any console call by recording it under its own name."""
        return self._record(name)

    def said(self, fragment: str) -> bool:
        """Whether any recorded line contains *fragment*."""
        return any(fragment in text for _level, text in self.lines)


@pytest.fixture
def recorder(monkeypatch):
    """Patch a module's ``console`` with a recorder and hand it back.

    Used as ``recorder(proc)``: the module is named by the test, because a
    module that holds its own ``console`` reference does not see a patch applied
    to a neighbour's.
    """
    def patch(module) -> RecordingConsole:
        spy = RecordingConsole()
        monkeypatch.setattr(module, "console", spy)
        return spy
    return patch
