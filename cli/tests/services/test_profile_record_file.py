# test_profile_record_file.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""What the reader of a profile record does with the file it opens."""

from __future__ import annotations

import json

import pytest

from sushitrack_cli.services import profile_record
from sushitrack_cli.services.profile_record import Latency, read_latency

pytestmark = pytest.mark.medium


def test_a_record_file_reads_as_its_parsed_latency(tmp_path):
    """Reads the one 8 us frame period of a version 2 file."""
    path = tmp_path / "r.json"
    path.write_text(json.dumps({
        "sushiprofiler_record": 2, "names": ["", profile_record.FRAME_TIMELINE],
        "periods": [{"timeline": 1, "warmup": False, "begin_ns": 0, "end_ns": 8000}]}),
        encoding="utf-8")
    assert read_latency(path) == Latency(median_us=8.0, low_us=8.0, high_us=8.0, periods=1)


def test_a_missing_file_gives_none(tmp_path):
    """Gives none for a path that holds no file."""
    assert read_latency(tmp_path / "absent.json") is None


def test_text_that_is_not_json_gives_none(tmp_path):
    """Gives none for a file that does not decode."""
    path = tmp_path / "bad.json"
    path.write_text("{ not json", encoding="utf-8")
    assert read_latency(path) is None
