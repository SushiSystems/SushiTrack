# messages.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Writes the evaluator's messages: progress and results on stdout, problems on stderr."""

from __future__ import annotations

import sys


def info(message: str) -> None:
    """Prints one line on stdout."""
    print(message)


def success(message: str) -> None:
    """Prints one line on stdout."""
    print(message)


def warn(message: str) -> None:
    """Prints one warning line on stderr."""
    print(f"warning: {message}", file=sys.stderr)


def error(message: str) -> None:
    """Prints one error line on stderr."""
    print(f"error: {message}", file=sys.stderr)


def progress(label: str, done: int, total: int, fraction: float | None = None) -> None:
    """Rewrites one progress line on stdout, with a percentage when the fraction is known."""
    share = f" ({fraction * 100:.0f}%)" if fraction is not None else ""
    end = "\n" if fraction is not None and done >= total else ""
    print(f"\r{label} {done}/{total}{share}", end=end, flush=True)
