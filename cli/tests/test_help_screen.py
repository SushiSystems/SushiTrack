# test_help_screen.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Tests that st's help screen groups its commands under three headings, in order."""

from __future__ import annotations

from typer.testing import CliRunner

import pytest

from sushitrack_cli.cli import app, main

K_GROUPS = {
    "Project": ["build", "test", "run", "clean", "deploy", "container"],
    "Benchmark": ["eval", "infer"],
    "Diagnostics": ["status", "config", "paths", "env", "setup", "doctor", "link", "unlink"],
}

K_EXAMPLES = {
    "build": "st build --type debug",
    "test": "st test --suite unit",
    "run": "st run unit_test",
    "clean": "st clean",
    "deploy": "st deploy cpp",
    "eval": "st eval --compare",
    "config": "st config",
    "paths": "st paths",
    "env": "st env --all",
}

# Commands whose --help is forwarded to the program they run, so Typer draws no page.
K_FORWARDING = {"infer"}


def _run(*args: str) -> str:
    result = CliRunner().invoke(app, list(args))
    assert result.exit_code == 0, result.output
    return result.output


def _commands_by_heading(page: str) -> dict[str, list[str]]:
    """The first word of each command row, keyed by the heading printed above it."""
    grouped: dict[str, list[str]] = {}
    heading = ""
    for line in page.splitlines():
        stripped = line.strip()
        if line and not line.startswith(" "):
            heading = stripped
            grouped[heading] = []
        elif line.startswith("  ") and stripped and heading:
            grouped[heading].append(stripped.split()[0])
    return grouped


def test_root_help_lists_the_three_groups_in_order():
    lines = [line.rstrip() for line in _run("--help").splitlines()]
    positions = [lines.index(heading) for heading in K_GROUPS if heading in lines]
    assert positions == sorted(positions)
    assert len(positions) == len(K_GROUPS)


def test_every_command_sits_under_its_own_group():
    grouped = _commands_by_heading(_run("--help"))
    for heading, names in K_GROUPS.items():
        assert set(grouped.get(heading, [])) >= set(names)


def test_a_bare_invocation_prints_the_same_page():
    result = CliRunner().invoke(app, [])
    assert result.output.strip() == _run("--help").strip()


@pytest.mark.parametrize("word", ["container", "docker"])
def test_the_container_sub_app_draws_its_own_page_under_both_words(word):
    """Draws the sub-application's help page for `container` and for `docker`."""
    out = _run(word, "--help")
    assert "build" in out and "run" in out


def _exit_code(argv: list[str]) -> int:
    """Returns the code main() exits with for *argv*."""
    with pytest.raises(SystemExit) as raised:
        main(argv)
    return raised.value.code or 0


def test_main_exits_zero_on_a_bare_invocation():
    """Exits 0 when no argument is given."""
    assert _exit_code([]) == 0


def test_main_exits_zero_on_help():
    """Exits 0 for `--help`."""
    assert _exit_code(["--help"]) == 0


def test_main_exits_nonzero_on_an_unknown_command():
    """Exits non-zero for a command st does not have."""
    assert _exit_code(["bogus-command"]) != 0


def test_shared_provision_commands_are_listed():
    tokens = set(_run("--help").split())
    for name in ("setup", "doctor", "link", "unlink"):
        assert name in tokens
