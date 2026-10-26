# profile_record.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Reads the frame timing of a SushiProfiler record that `mot_engine` wrote."""

from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path

#: The record format version this reader understands.
RECORD_FORMAT_VERSION = 2

#: The timeline `mot_engine` marks once per frame.
FRAME_TIMELINE = "sushitrack.frame"


@dataclass(frozen=True)
class Latency:
    """The per-frame cost one record holds, in microseconds.

    Attributes:
        median_us: The median period length.
        low_us: The 25th percentile of the period lengths.
        high_us: The 75th percentile of the period lengths.
        periods: How many periods the figures come from.
    """

    median_us: float
    low_us: float
    high_us: float
    periods: int


def _percentile(sorted_values: list[float], fraction: float) -> float:
    """Returns the *fraction* percentile of an ascending list by linear interpolation."""
    position = fraction * (len(sorted_values) - 1)
    below = int(position)
    above = min(below + 1, len(sorted_values) - 1)
    weight = position - below
    return sorted_values[below] * (1.0 - weight) + sorted_values[above] * weight


def read_latency(path: Path) -> Latency | None:
    """Returns the frame latency of the record at *path*, or None when it holds none.

    Args:
        path: A `.sprec.json` file written by `mot_engine`.

    Returns:
        The median and quartiles of the `sushitrack.frame` periods that are not warm-up, or None
        when the file is missing, is not a version 2 record or holds no such period.
    """
    try:
        document = json.loads(Path(path).read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return None
    if not isinstance(document, dict):
        return None
    if document.get("sushiprofiler_record") != RECORD_FORMAT_VERSION:
        return None
    names = document.get("names", [])
    lengths = []
    for period in document.get("periods", []):
        timeline = period.get("timeline", 0)
        if timeline >= len(names) or names[timeline] != FRAME_TIMELINE or period.get("warmup"):
            continue
        lengths.append((period["end_ns"] - period["begin_ns"]) / 1000.0)
    if not lengths:
        return None
    lengths.sort()
    return Latency(median_us=_percentile(lengths, 0.5), low_us=_percentile(lengths, 0.25),
                   high_us=_percentile(lengths, 0.75), periods=len(lengths))
