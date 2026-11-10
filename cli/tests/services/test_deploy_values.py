# test_deploy_values.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""What the deploy service says of its kinds and runtime library names, without a build tree."""

from __future__ import annotations

from types import SimpleNamespace

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


def test_an_unknown_kind_is_refused_with_the_choices_and_builds_nothing(session):
    """Returns 2 and names cpp and python; the paths hold nothing a build would read."""
    code = deploy.deploy(SimpleNamespace(), "rust", workspace=session.workspace)

    assert code == 2
    assert session.said("Unknown deploy kind 'rust'. Choose: cpp, python.")
    assert session.commands == []
