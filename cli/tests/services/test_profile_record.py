# test_profile_record.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""What the reader of a profile record takes from it."""

from __future__ import annotations

import json

import pytest

from sushitrack_cli.services import profile_record
from sushitrack_cli.services.profile_record import Latency, read_latency

pytestmark = pytest.mark.small


def _write(path, periods, names=("", profile_record.FRAME_TIMELINE), version=2):
    """Write a record with *periods* and return its path."""
    path.write_text(json.dumps({"sushiprofiler_record": version, "names": list(names),
                                "periods": periods}), encoding="utf-8")
    return path


def _period(length_ns, timeline=1, warmup=False):
    """Return a period of *length_ns* on timeline *timeline*."""
    return {"timeline": timeline, "warmup": warmup, "begin_ns": 1000, "end_ns": 1000 + length_ns}


def test_the_median_and_quartiles_come_from_the_frame_periods(tmp_path):
    lengths = (10_000, 20_000, 30_000, 40_000, 50_000)
    path = _write(tmp_path / "r.json", [_period(n) for n in lengths])
    assert read_latency(path) == Latency(median_us=30.0, low_us=20.0, high_us=40.0, periods=5)


def test_warm_up_periods_and_other_timelines_are_left_out(tmp_path):
    path = _write(tmp_path / "r.json",
                  [_period(10_000, warmup=True), _period(90_000, timeline=2), _period(4_000)],
                  names=("", profile_record.FRAME_TIMELINE, "other"))
    assert read_latency(path) == Latency(median_us=4.0, low_us=4.0, high_us=4.0, periods=1)


def test_a_missing_file_a_wrong_version_and_an_empty_record_give_none(tmp_path):
    assert read_latency(tmp_path / "absent.json") is None
    assert read_latency(_write(tmp_path / "old.json", [_period(1000)], version=1)) is None
    assert read_latency(_write(tmp_path / "empty.json", [])) is None


def test_text_that_is_not_json_gives_none(tmp_path):
    path = tmp_path / "bad.json"
    path.write_text("{ not json", encoding="utf-8")
    assert read_latency(path) is None
