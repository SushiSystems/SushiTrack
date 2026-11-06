# __init__.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""Service layer: all CLI behaviour lives here.

Commands in :mod:`sushitrack_cli.cli` only parse arguments and delegate to these
modules, each of which returns a process exit code. Nothing here imports
argparse, so every operation is callable from a script or a test.
"""
