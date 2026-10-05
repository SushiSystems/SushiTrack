# console.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026 Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
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


# PEP 562: an attribute resolves on first access, so importing this module reads no file.
__getattr__ = _lazy.attribute
