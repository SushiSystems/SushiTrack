# test_cli_surface.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Tests the surfaces st takes from sushicore: root options, diagnostics, aliases, entry."""

from __future__ import annotations

import json
from importlib.metadata import version

import pytest
import typer.main
from sushicore.cli_console import LazyConsole
from typer.testing import CliRunner

from sushitrack_cli import cli, console
from sushitrack_cli.config import config_dir
from sushitrack_cli.services import docker as docker_svc

K_VISIBLE = {
    "build", "test", "run", "clean", "deploy", "container build", "container run",
    "eval", "infer", "status", "config", "paths", "env",
    "setup", "doctor", "link", "unlink",
}


def _invoke(*args: str):
    """Runs st with *args* through Typer's test runner."""
    return CliRunner().invoke(cli.app, list(args))


def _described() -> set[str]:
    """Returns the command names `st --describe` lists."""
    result = _invoke("--describe")
    assert result.exit_code == 0, result.output
    return {command["name"] for command in json.loads(result.output)["commands"]}


def test_version_names_the_distribution_and_its_version():
    """Prints `sushitrack-cli` and its installed version, then exits 0."""
    result = _invoke("--version")
    assert result.exit_code == 0
    assert result.output.strip() == f"sushitrack-cli {version('sushitrack-cli')}"


def test_describe_lists_every_visible_command_and_no_hidden_one():
    """Lists exactly the visible commands in the catalogue."""
    assert _described() == K_VISIBLE


def test_config_and_env_are_the_shared_declarations():
    """Takes `config` and `env` from sushicore's one declaration."""
    group = typer.main.get_command(cli.app)
    for name in ("config", "env"):
        assert group.commands[name].callback.__module__ == "sushicore.diag_commands"


@pytest.mark.parametrize("action", ["build", "run"])
def test_the_docker_word_still_runs_the_container_command(action, monkeypatch):
    """Runs the container service when the old `docker` word is typed."""
    seen: list[str] = []
    monkeypatch.setattr(cli, "context", lambda: (object(), object()))
    monkeypatch.setattr(docker_svc, action,
                        lambda cfg, paths, **options: seen.append(action) or 0)
    result = _invoke("docker", action)
    assert result.exit_code == 0, result.output
    assert seen == [action]


def test_the_docker_word_is_absent_from_help_and_describe():
    """Leaves `docker` out of the help page and the catalogue."""
    page = _invoke("--help").output
    assert "container" in page.split()
    assert "docker" not in page.split()
    assert not any(name.startswith("docker") for name in _described())


def _broken_checkout(root) -> None:
    """Writes a SushiTrack root marker and a cli/config.toml that is not valid TOML."""
    marker = root / "include" / "SushiTrack" / "sushitrack_c.h"
    marker.parent.mkdir(parents=True)
    marker.write_text("", encoding="utf-8")
    (root / "cli").mkdir()
    (root / "cli" / "config.toml").write_text("[build\ntype = ", encoding="utf-8")


@pytest.mark.parametrize("function", ["main", "entry"])
def test_a_malformed_config_gives_one_line_and_exit_one(
        function, tmp_path, monkeypatch, capsys):
    """Prints one `st:` line and exits 1 for a config.toml that is not TOML."""
    _broken_checkout(tmp_path)
    monkeypatch.chdir(tmp_path)
    monkeypatch.setattr("sys.argv", ["st", "paths"])
    fresh = LazyConsole(config_dir)
    monkeypatch.setattr(console, "_lazy", fresh)
    monkeypatch.setattr(console, "__getattr__", fresh.attribute)
    with pytest.raises(SystemExit) as raised:
        getattr(cli, function)()
    assert raised.value.code == 1
    captured = capsys.readouterr()
    lines = [line for line in (captured.out + captured.err).splitlines() if line.strip()]
    assert len(lines) == 1
    assert lines[0].startswith("st:") and "config.toml" in lines[0]
    assert "Traceback" not in lines[0]


def test_main_turns_a_bare_invocation_into_the_help_page(monkeypatch, capsys):
    """Prints the help page and exits 0 when no argument is given."""
    monkeypatch.setattr("sys.argv", ["st"])
    with pytest.raises(SystemExit) as raised:
        cli.main()
    assert raised.value.code == 0
    assert "Usage" in capsys.readouterr().out


def test_main_exits_nonzero_on_an_unknown_command(monkeypatch):
    """Exits non-zero for a command st does not have."""
    monkeypatch.setattr("sys.argv", ["st", "bogus-command"])
    with pytest.raises(SystemExit) as raised:
        cli.main()
    assert raised.value.code != 0
