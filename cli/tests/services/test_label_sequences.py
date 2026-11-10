# test_label_sequences.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""Which sequences `st eval` selects, over an in-memory data folder."""

from __future__ import annotations

from types import SimpleNamespace

import pytest

from sushitrack_cli.services import label

pytestmark = pytest.mark.small


class _Sequence:
    """Stands in for a sequence folder that holds gt/gt.txt."""

    def __init__(self, name):
        self.name = name

    def is_dir(self):
        """Return True: the entry is a folder."""
        return True

    def __truediv__(self, part):
        """Return this folder for any path below it."""
        return self

    def exists(self):
        """Return True: the ground truth file is there."""
        return True


class _DataRoot:
    """Stands in for a data folder that holds the sequences named at construction."""

    def __init__(self, *names):
        self._names = names

    def is_dir(self):
        """Return True: the data folder exists."""
        return True

    def iterdir(self):
        """Return the sequences, unsorted as a disk gives them."""
        return [_Sequence(name) for name in reversed(self._names)]


def _paths(*names):
    """Return the paths whose one benchmark holds the sequences *names*."""
    return SimpleNamespace(data_root=lambda benchmark: _DataRoot(*names))


K_MIXED = ("MOT17-02-DPM", "MOT17-02-YOLOX", "dancetrack0001")


def test_private_keeps_the_yolox_sequences_and_the_non_mot_ones():
    """Drops the public MOT sequence; the sorted names are DPM, YOLOX, then the lower-case one."""
    assert label.sequences(_paths(*K_MIXED), "mot17", private=True) == [
        "MOT17-02-YOLOX", "dancetrack0001"]


def test_public_keeps_the_sequences_that_do_not_end_in_yolox():
    """Drops the YOLOX sequence and keeps the public MOT one and the non-MOT one."""
    assert label.sequences(_paths(*K_MIXED), "mot17", public=True) == [
        "MOT17-02-DPM", "dancetrack0001"]


@pytest.mark.parametrize("scenario, expected", [
    ("A", ["A"]),
    ("a", ["A", "AB"]),
    ("ab", ["AB"]),
])
def test_a_scenario_matches_an_exact_name_first_then_a_case_insensitive_substring(
        scenario, expected):
    """Returns "A" alone for "A", and every name holding "a" or "ab" in any case otherwise."""
    paths = _paths("A", "AB", "MOT17-04-DPM")
    assert label.sequences(paths, "mot17", scenario=scenario) == expected
