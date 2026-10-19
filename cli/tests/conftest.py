# conftest.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Provides the fixtures of the SushiTrack CLI's tests; none of them reaches the machine.

The rules of the suite are in docs/guides/TESTING.md.
"""

from __future__ import annotations

import importlib.util
import sys
from pathlib import Path

import pytest

# cli/ goes on the path so the suite runs from the repository root as well as from cli/.
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

if importlib.util.find_spec("sushicore") is None:  # pragma: no cover - import guard
    raise RuntimeError(
        "sushicore is not importable, so the sushitrack CLI cannot be imported. "
        "Install this CLI's dependencies: `pip install -e cli` or "
        "`pip install sushicore`.")


#: The timeout of each size, in seconds; see docs/guides/TESTING.md.
K_SIZE_TIMEOUTS = {"small": 60, "medium": 300, "large": 900}


def _sizes_of(item: pytest.Item) -> list[str]:
    """Return the size labels *item* carries, one entry per label."""
    return [mark.name for mark in item.iter_markers() if mark.name in K_SIZE_TIMEOUTS]


def pytest_collection_modifyitems(items: list[pytest.Item]) -> None:
    """Fail collection when a test carries no size or more than one, and give each its timeout."""
    wrong = [f"{item.nodeid}: {_sizes_of(item) or 'no size'}"
             for item in items if len(_sizes_of(item)) != 1]
    if wrong:
        raise pytest.UsageError(f"every test carries exactly one size: {', '.join(wrong)}")
    for item in items:
        item.add_marker(pytest.mark.timeout(K_SIZE_TIMEOUTS[_sizes_of(item)[0]]))


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
