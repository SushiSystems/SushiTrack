# test_service_enums.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""The Typer command layer's selection enums mirror their service's own
string constants exactly — one canonical value set, not two."""

from __future__ import annotations

from sushitrack_cli.services import build as build_svc
from sushitrack_cli.services import deploy as deploy_svc


def test_build_type_values_match_build_types():
    assert {member.value for member in build_svc.BuildType} == \
        {name.lower() for name in build_svc.BUILD_TYPES}


def test_deploy_kind_values_match_kinds():
    assert {member.value for member in deploy_svc.DeployKind} == set(deploy_svc.KINDS)
