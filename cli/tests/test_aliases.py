# test_aliases.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""Tests st's alias table: its rows, their date and that none has expired."""

from __future__ import annotations

from datetime import date, timedelta

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


@pytest.mark.parametrize("days_after_the_last_day, expired_spellings", [
    (0, []),
    (1, ["st docker build", "st docker run"]),
])
def test_an_alias_expires_the_day_after_its_last_day(days_after_the_last_day, expired_spellings):
    """Lists no row on the last day and every row on the day after it."""
    day = aliases.K_REMOVED_AFTER + timedelta(days=days_after_the_last_day)

    expired = aliases.ALIASES.expired(day)

    assert sorted(row.old for row in expired) == expired_spellings
