# test_tool_settings.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""Tests which compiler st's tool settings name when two sources give one."""

from __future__ import annotations

from pathlib import Path

import pytest

from sushitrack_cli import config

pytestmark = pytest.mark.small


def test_the_compiler_the_environment_names_outranks_the_configured_one():
    """Resolves to the stack compiler when both it and a configured compiler are set."""
    tool = config.ToolSettings(cxx="/pinned/clang++", stack_cxx="/stack/clang++")

    assert tool.resolved_compiler(Path(".")) == "/stack/clang++"
