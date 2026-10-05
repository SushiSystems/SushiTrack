# test_deploy_templates.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026 Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Tests the example sources `st deploy` writes into the package."""

from __future__ import annotations

import ast

from sushitrack_cli.services import deploy


def test_the_c_example_includes_each_header_on_its_own_line():
    """Writes one `#include` directive per line in example.c."""
    lines = deploy._CPP_EXAMPLE.splitlines()
    assert "#include <stdio.h>" in lines
    assert '#include "SushiTrack/sushitrack_c.h"' in lines
    assert not [line for line in lines if line.count("include") > 1]


def test_the_python_example_parses_and_its_comments_end_in_a_full_stop():
    """Writes an example.py that parses and whose comments are whole sentences."""
    ast.parse(deploy._PY_EXAMPLE)
    comments = [line for line in deploy._PY_EXAMPLE.splitlines() if line.startswith("#")]
    assert comments
    assert all(line.endswith(".") for line in comments)
