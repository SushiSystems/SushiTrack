# test_main.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""`python -m sushitrack_cli` runs the same entry point as `st`."""

from __future__ import annotations

import runpy

import pytest

from sushitrack_cli import cli

pytestmark = pytest.mark.small


def test_running_the_package_calls_the_cli_entry(monkeypatch):
    calls = []
    monkeypatch.setattr(cli, "main", lambda: calls.append("main"))
    runpy.run_module("sushitrack_cli", run_name="__main__")
    assert calls == ["main"]
