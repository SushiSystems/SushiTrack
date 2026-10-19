# test_docker.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""`st container` spawns its children through the shared runner."""

from __future__ import annotations

import pytest

import importlib

from sushitrack_cli.services import docker

pytestmark = pytest.mark.small


def test_the_service_uses_the_shared_runner():
    assert docker.RUNNER is importlib.import_module("sushitrack_cli.process").RUNNER
