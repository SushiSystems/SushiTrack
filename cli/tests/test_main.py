# test_main.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""`python -m sushitrack_cli` runs the same entry point as `st`."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

import pytest

pytestmark = pytest.mark.medium

#: The cli/ folder, where `python -m sushitrack_cli` finds the package.
K_CLI_FOLDER = Path(__file__).resolve().parents[1]


def test_running_the_package_prints_the_help_page():
    """Prints the help page, with its first group heading on a line of its own, and exits 0."""
    result = subprocess.run(
        [sys.executable, "-m", "sushitrack_cli", "--help"],
        cwd=K_CLI_FOLDER, capture_output=True, text=True, timeout=120, check=False)

    assert result.returncode == 0, result.stderr
    assert "Project" in [line.rstrip() for line in result.stdout.splitlines()]
