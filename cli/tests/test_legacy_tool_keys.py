# test_legacy_tool_keys.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""Tests that st lists the build-tool keys that moved to the [tool] table, from a document alone."""

from __future__ import annotations

import pytest

from sushitrack_cli import config as config_module

pytestmark = pytest.mark.small


def test_legacy_tool_keys_name_each_old_key_and_its_replacement():
    """Pairs each old [build] and [platform.<os>.build] key with the [tool] key that replaced it."""
    document = {
        "build": {"cmake": "c", "ctest": "t", "type": "Debug"},
        "platform": {"windows": {"build": {"generator": "Ninja"}}},
    }

    found = config_module.legacy_tool_keys(document)

    assert found == [
        ("[build] cmake", "[tool] cmake_exe"),
        ("[build] ctest", "[tool] ctest_exe"),
        ("[platform.windows.build] generator", "[tool] generator"),
    ]
