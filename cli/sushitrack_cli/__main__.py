# __main__.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026 Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Enable `python3 -m sushitrack_cli` as an entry point."""

from __future__ import annotations

from .cli import main

if __name__ == "__main__":
    main()
