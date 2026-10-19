# test_test_label.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""The selections `st test --label` makes."""

from __future__ import annotations

from sushitrack_cli.services.test_label import TestLabel, select_labels


def test_no_label_selects_small_and_medium_and_leaves_the_quarantine_out():
    """None selects the default sizes and excludes quarantined tests."""
    selection = select_labels(None)
    assert selection.ctest_include() == "^(small|medium)$"
    assert selection.ctest_exclude() == "^quarantined$"


def test_sizes_keep_the_declared_order():
    """Labels given out of order come back in small, medium, large order."""
    selection = select_labels([TestLabel.LARGE, TestLabel.SMALL])
    assert selection.ctest_include() == "^(small|large)$"


def test_quarantined_alone_selects_only_the_quarantine():
    """`--label quarantined` selects the quarantined tests and excludes nothing."""
    selection = select_labels([TestLabel.QUARANTINED])
    assert selection.ctest_include() == "^quarantined$"
    assert selection.ctest_exclude() is None
