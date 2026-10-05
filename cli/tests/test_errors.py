# test_errors.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026 Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Tests that a bad build type or tracker raises an error the entry point prints."""

from __future__ import annotations

import pytest
from sushicore.errors import SushiCoreError

from sushitrack_cli import errors
from sushitrack_cli.services import build as build_svc
from sushitrack_cli.services import label


def test_every_cli_error_derives_from_the_base_the_entry_point_catches():
    """Derives every st error from the class sushicore's entry point catches."""
    assert issubclass(errors.SushiTrackCliError, SushiCoreError)
    assert issubclass(errors.UnknownBuildTypeError, errors.SushiTrackCliError)
    assert issubclass(errors.UnknownTrackerError, errors.SushiTrackCliError)


def test_an_unknown_build_type_names_the_value_and_the_choices():
    """Raises with the rejected build type and the accepted ones."""
    with pytest.raises(errors.UnknownBuildTypeError, match="'fast'.*Release"):
        build_svc.normalize_build_type("fast")


def test_an_unknown_tracker_names_the_value_and_the_choices():
    """Raises with the rejected tracker and the registered ones."""
    with pytest.raises(errors.UnknownTrackerError, match="'sort'.*ByteTrack"):
        label.resolve_tracker("sort")
