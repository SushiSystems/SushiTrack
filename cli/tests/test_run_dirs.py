# test_run_dirs.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""Tests how st puts the run-time folders of the library packages on a search path."""

from __future__ import annotations

from types import SimpleNamespace

import pytest

from sushitrack_cli import library_packages

pytestmark = pytest.mark.small


def test_the_windows_run_dirs_lead_path():
    """Puts the package folder before the folders PATH already holds."""
    environment = {"PATH": "C:/old"}

    library_packages.prepend_run_dirs(SimpleNamespace(is_windows=True), environment, ["C:/pkg/bin"])

    assert environment["PATH"].startswith("C:/pkg/bin")
