# test_main.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""`python -m sushitrack_cli` runs the same entry point as `st`."""

from __future__ import annotations

import runpy

import pytest

pytestmark = pytest.mark.medium


def test_running_the_package_runs_the_cli_entry(monkeypatch, capsys):
    monkeypatch.setattr("sys.argv", ["st", "--version"])

    with pytest.raises(SystemExit) as raised:
        runpy.run_module("sushitrack_cli", run_name="__main__")

    assert raised.value.code == 0
    assert capsys.readouterr().out.startswith("sushitrack-cli ")
