# test_evaluate.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""The evaluator front reports a missing official script and starts nothing."""

from __future__ import annotations

from types import SimpleNamespace

import pytest

from sushitrack_cli.services import evaluate

pytestmark = pytest.mark.medium


def test_a_missing_official_script_fails_and_names_the_submodule(tmp_path, session):
    paths = SimpleNamespace(third_party=tmp_path)

    code = evaluate.run_official(paths, tmp_path, ["S1"], ["sushitrack"],
                                 workspace=session.workspace)

    assert code == 1 and session.commands == []
    assert session.said("git submodule update --init")
