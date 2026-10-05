# __init__.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026 Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Service layer: all CLI behaviour lives here.

Commands in :mod:`sushitrack_cli.cli` only parse arguments and delegate to these
modules, each of which returns a process exit code. Nothing here imports
argparse, so every operation is callable from a script or a test.
"""
