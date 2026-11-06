# conftest.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""Provides the fixtures of the SushiTrack CLI's tests; none of them reaches the machine.

The rules of the suite are in docs/guides/TESTING.md.
"""

from __future__ import annotations

import importlib.util
import sys
from dataclasses import dataclass
from pathlib import Path

import pytest

# cli/ goes on the path so the suite runs from the repository root as well as from cli/.
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

if importlib.util.find_spec("sushicore") is None:  # pragma: no cover - import guard
    raise RuntimeError(
        "sushicore is not importable, so the sushitrack CLI cannot be imported. "
        "Install this CLI's dependencies: `pip install -e cli` or "
        "`pip install sushicore`.")

from sushicore.console_memory import RecordingConsole  # noqa: E402  (after the guard)
from sushicore.proc_memory import RecordingRunner  # noqa: E402

from sushitrack_cli.workspace import Workspace  # noqa: E402  (needs cli/ on the path)


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


def said(console: RecordingConsole, fragment: str) -> bool:
    """Returns whether any call recorded by *console* carries *fragment* in an argument."""
    return any(fragment in str(argument) for call in console.calls for argument in call.args)


@dataclass
class Session:
    """Holds a workspace over sushicore's in-memory runner and console, and what they recorded."""

    console: RecordingConsole
    runner: RecordingRunner
    workspace: Workspace

    @property
    def commands(self) -> list[list[str]]:
        """Returns the argv of every command the runner was handed, in order."""
        return [list(command.argv) for command in self.runner.commands]

    def said(self, fragment: str) -> bool:
        """Returns whether any recorded console call carries *fragment* in an argument."""
        return said(self.console, fragment)


@pytest.fixture
def session() -> Session:
    """Returns a workspace whose console and runner record, with an empty environment."""
    console = RecordingConsole()
    runner = RecordingRunner(console)
    return Session(console, runner, Workspace(console, runner, {}))
