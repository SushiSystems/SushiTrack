# test_plain_runs.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026 Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""docker, infer and eval spawn through the shared runner."""

from __future__ import annotations

import importlib

import pytest


@pytest.mark.parametrize("name", ["docker", "infer", "evaluate"])
def test_the_service_uses_the_shared_runner(name):
    module = importlib.import_module(f"sushitrack_cli.services.{name}")
    assert not hasattr(module, "proc")
    assert module.RUNNER is importlib.import_module("sushitrack_cli.process").RUNNER
