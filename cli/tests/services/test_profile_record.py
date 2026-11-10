# test_profile_record.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""What the reader of a profile record takes from it."""

from __future__ import annotations

import pytest

from sushitrack_cli.services import profile_record
from sushitrack_cli.services.profile_record import Latency, parse_latency

pytestmark = pytest.mark.small


def _record(periods, names=("", profile_record.FRAME_TIMELINE), version=2):
    """Return the decoded record holding *periods*."""
    return {"sushiprofiler_record": version, "names": list(names), "periods": periods}


def _period(length_ns, timeline=1, warmup=False):
    """Return a period of *length_ns* on timeline *timeline*."""
    return {"timeline": timeline, "warmup": warmup, "begin_ns": 1000, "end_ns": 1000 + length_ns}


def test_the_median_and_quartiles_come_from_the_frame_periods():
    """Takes the percentiles of the lengths 10, 20, 30, 40 and 50 us by linear interpolation.

    The position of a fraction f in five values is f * 4: the median sits at 2 (30 us), the
    first quartile at 1 (20 us) and the third at 3 (40 us), each on a value without a blend.
    """
    document = _record([_period(n) for n in (10_000, 20_000, 30_000, 40_000, 50_000)])
    assert parse_latency(document) == Latency(median_us=30.0, low_us=20.0, high_us=40.0, periods=5)


def test_warm_up_periods_and_other_timelines_are_left_out():
    """Keeps the one 4 us frame period; the warm-up one and the period of timeline 2 are dropped."""
    document = _record([_period(10_000, warmup=True), _period(90_000, timeline=2), _period(4_000)],
                       names=("", profile_record.FRAME_TIMELINE, "other"))
    assert parse_latency(document) == Latency(median_us=4.0, low_us=4.0, high_us=4.0, periods=1)


def test_a_record_of_another_version_gives_none():
    """Refuses a version 1 record."""
    assert parse_latency(_record([_period(1000)], version=1)) is None


def test_a_record_without_a_frame_period_gives_none():
    """Gives none for a record whose periods list is empty."""
    assert parse_latency(_record([])) is None


def test_a_document_that_is_not_an_object_gives_none():
    """Gives none for a JSON array."""
    assert parse_latency([_period(1000)]) is None
