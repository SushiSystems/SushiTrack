# test_build_values.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""What the build service's generator choice and build type names say, without a tree."""

from __future__ import annotations

from types import SimpleNamespace

import pytest

from sushitrack_cli.services import build as build_svc

pytestmark = pytest.mark.small


def _cfg(generator="", windows=True):
    """Return a config holding only the tool settings the generator choice reads."""
    tool = SimpleNamespace(generator=generator, is_windows=windows, expand=lambda value: value)
    return SimpleNamespace(tool=tool)


def test_the_default_generator_follows_the_host():
    """Defaults to Ninja on Windows and CMake's choice elsewhere."""
    assert build_svc.generator(_cfg(windows=True)) == "Ninja"
    assert build_svc.generator(_cfg(windows=False)) == ""


def test_a_configured_generator_wins():
    """Uses the generator the config names."""
    assert build_svc.generator(_cfg(generator="Unix Makefiles")) == "Unix Makefiles"


def test_build_type_values_match_build_types():
    """Names each BuildType member as the lower-cased entry of BUILD_TYPES."""
    assert {member.value for member in build_svc.BuildType} == \
        {name.lower() for name in build_svc.BUILD_TYPES}
