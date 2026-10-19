# test_aliases.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Tests st's alias table: its rows, their date and that none has expired."""

from __future__ import annotations

from datetime import date

import pytest

from sushitrack_cli import aliases

pytestmark = pytest.mark.small


@pytest.mark.parametrize("old, new", [
    ("st docker build", "st container build"),
    ("st docker run", "st container run"),
])
def test_each_old_spelling_names_its_replacement_and_the_shared_date(old, new):
    """Maps each old spelling to its new one with the estate's removal date."""
    row = aliases.ALIASES.alias_for(old)
    assert row.new == new
    assert row.removed_after == date(2027, 3, 25)


def test_no_alias_has_expired_today():
    """Holds no row whose last day has passed."""
    assert aliases.ALIASES.expired(date.today()) == []
