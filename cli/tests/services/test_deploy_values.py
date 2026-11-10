# test_deploy_values.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""What the deploy service's kind names and runtime library names say, without a build tree."""

from __future__ import annotations

import pytest

from sushitrack_cli.services import artifacts, deploy

pytestmark = pytest.mark.small


def test_deploy_kind_values_match_kinds():
    """Names each DeployKind member as an entry of KINDS."""
    assert {member.value for member in deploy.DeployKind} == set(deploy.KINDS)


@pytest.mark.parametrize(("is_windows", "expected"), [
    (True, "sushitrack.dll"),
    (False, "libsushitrack.so"),
])
def test_the_runtime_library_names_follow_the_platform(monkeypatch, is_windows, expected):
    """Names the runtime library sushitrack.dll on Windows and libsushitrack.so elsewhere."""
    monkeypatch.setattr(artifacts, "IS_WINDOWS", is_windows)
    assert deploy.shared_lib_name() == expected
