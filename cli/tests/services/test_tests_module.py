# test_tests_module.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""Which trees and which pytest call `st test` hands the shared test command."""

from __future__ import annotations

import sys
from pathlib import Path
from types import SimpleNamespace

import pytest
from sushicore.testing.labels import LabelSelection, TestLabel

from sushitrack_cli.services import tests as test_svc

pytestmark = pytest.mark.small

K_ROOT = Path("/project")


def _module(session, build_type="Debug"):
    """Return what `st test` hands the shared command over a project at K_ROOT."""
    cfg = SimpleNamespace(build_type=build_type, tool=SimpleNamespace(), jobs=lambda: 2)
    paths = SimpleNamespace(root=K_ROOT, cli=K_ROOT / "cli", build_root=K_ROOT / "build")
    return test_svc.module_tests(lambda: (cfg, paths), workspace=session.workspace)


def test_the_default_tree_is_the_one_of_the_configured_build_type(session):
    """Names build/default-debug for a Debug configuration."""
    assert _module(session).build_tree() == K_ROOT / "build" / "default-debug"


def test_the_sanitizers_on_offer_are_address_and_thread(session):
    """Offers the two sanitizer builds."""
    assert _module(session).sanitized.names == ("address", "thread")


def test_a_sanitizer_names_its_own_tree(session):
    """Names build/asan-debug for the address sanitizer."""
    assert _module(session).sanitized.tree("address") == K_ROOT / "build" / "asan-debug"


def test_the_cli_tests_pass_the_label_selection_to_pytest_in_the_cli_folder(session):
    """Runs pytest -m with the selection's expression, from the cli folder.

    The selection of the small and medium sizes without the quarantined ones reads
    "(small or medium) and not quarantined".
    """
    selection = LabelSelection((TestLabel.SMALL, TestLabel.MEDIUM), False)

    assert _module(session).cli_tests(selection) == 0

    (command,) = session.runner.commands
    assert list(command.argv) == [sys.executable, "-m", "pytest", "-q", "-m",
                                  "(small or medium) and not quarantined"]
    assert command.cwd == K_ROOT / "cli"
