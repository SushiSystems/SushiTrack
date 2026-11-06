# test_infer.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""`st infer` spawns its children through the shared runner."""

from __future__ import annotations

import pytest

import importlib

from sushitrack_cli.services import infer

pytestmark = pytest.mark.small


def test_the_service_uses_the_shared_runner():
    assert infer.RUNNER is importlib.import_module("sushitrack_cli.process").RUNNER
