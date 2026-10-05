# errors.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026 Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Declares the failures st reports to its entry point as one line."""

from __future__ import annotations

from sushicore.errors import SushiCoreError


class SushiTrackCliError(SushiCoreError):
    """Reports a failure of the st CLI that the user can act on."""


class UnknownBuildTypeError(SushiTrackCliError):
    """Reports a build type that CMake does not know."""


class UnknownTrackerError(SushiTrackCliError):
    """Reports a tracker name the registry does not hold."""
