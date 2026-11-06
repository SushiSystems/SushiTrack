# errors.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""Declares the failures st reports to its entry point as one line."""

from __future__ import annotations

from sushicore.errors import SushiCoreError


class SushiTrackCliError(SushiCoreError):
    """Reports a failure of the st CLI that the user can act on."""


class UnknownBuildTypeError(SushiTrackCliError):
    """Reports a build type that CMake does not know."""


class UnknownTrackerError(SushiTrackCliError):
    """Reports a tracker name the registry does not hold."""
