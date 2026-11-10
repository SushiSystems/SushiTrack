# console.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""Exposes the sushicore console this CLI prints through, built on first use.

The reason it is lazy is in docs/architecture/CLI_PACKAGE.md.
"""

from __future__ import annotations

from sushicore.cli_console import LazyConsole

from .config import config_dir

_lazy = LazyConsole(config_dir)


def current():
    """Return the sushicore Console this run prints through, building it on first use."""
    return _lazy.get()


def is_built() -> bool:
    """Return whether the console has been built yet; importing this module does not build it."""
    return _lazy.built


def config_sources():
    """Return the config files the console layers, lowest first; none outside a checkout."""
    return _lazy.sources()


# PEP 562: an attribute resolves on first access, so importing this module reads no file.
__getattr__ = _lazy.attribute
