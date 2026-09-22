"""st's help screen groups its commands under three headings, in the same
order the removed app.py's app.section(...) calls declared them."""

from __future__ import annotations

from typer.testing import CliRunner

from sushitrack_cli.cli import app

K_GROUPS = {
    "Project": ["build", "test", "run", "clean", "deploy", "docker"],
    "Benchmark": ["eval", "infer"],
    "Diagnostics": ["status", "doctor", "config", "paths", "env"],
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


def test_docker_sub_app_draws_its_own_page():
    out = _run("docker", "--help")
    assert "build" in out and "run" in out
