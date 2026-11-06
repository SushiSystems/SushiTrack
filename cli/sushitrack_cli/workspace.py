# workspace.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""Holds what the st services act through: the console, the runners and the environment."""

from __future__ import annotations

from collections.abc import Mapping, Sequence
from dataclasses import dataclass
from functools import cached_property
from pathlib import Path
from typing import Protocol

from sushicore.cmake_driver import CMakeDriver
from sushicore.proc import CommandRunner, ConsoleLike

from . import console
from .env import load_build_env
from .process import RUNNER


class ServiceConsole(ConsoleLike, Protocol):
    """The console methods the services print through, beyond what the runners use."""

    def info(self, text: str) -> None:
        """Prints an information line."""

    def success(self, text: str) -> None:
        """Prints a success line."""

    def header(self, title: str) -> None:
        """Prints a section header."""

    def table(self, columns: Sequence[str], rows: Sequence[Sequence[str]], title: str = "") -> None:
        """Prints a table."""


@dataclass(frozen=True)
class Workspace:
    """Holds what the services act through; the checkout they act on is passed to each call.

    Attributes:
        console: Where the services print.
        runner: What starts cmake, ctest, docker and the programs, and captures the output of
            the commands a service reads.
        environment: The environment the started commands run under; ``None`` assembles it per
            tree through ``sushitrack_cli.env.load_build_env``.
    """

    console: ServiceConsole
    runner: CommandRunner
    environment: Mapping[str, str] | None = None

    @cached_property
    def driver(self) -> CMakeDriver:
        """Returns the cmake and ctest driver over this workspace's console and runner."""
        return CMakeDriver(self.console, self.runner)

    def build_env(self, tool, build_dir: Path, tracker_config: Path | None = None) -> dict[str, str]:
        """Returns the environment cmake, ctest and the programs run under for *build_dir*."""
        if self.environment is not None:
            return dict(self.environment)
        return load_build_env(tool, build_dir, tracker_config, self.console)


def current_workspace() -> Workspace:
    """Returns the workspace `st` runs in, with the real console and the real runner."""
    return Workspace(console, RUNNER)
