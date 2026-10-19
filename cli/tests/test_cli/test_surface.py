# test_surface.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Tests the surfaces st takes from sushicore: root options, diagnostics, aliases, entry."""

from __future__ import annotations

import json
import subprocess
from importlib.metadata import version
from pathlib import Path

import pytest
import typer.main
from sushicore.cli_console import LazyConsole
from typer.testing import CliRunner

from sushitrack_cli import cli, console
from sushitrack_cli.config import config_dir
from sushitrack_cli.services import docker as docker_svc

pytestmark = pytest.mark.medium

K_GIT_IDENTITY = ("-c", "user.name=st tests", "-c", "user.email=st-tests@example.invalid")
K_PUBLISH_LIST = (
    'name = "sample"\n'
    'title = "Sample"\n'
    'summary = "A sample repository."\n'
    'sections = ["guides"]\n'
)
K_VISIBLE = {
    "build", "test", "run", "clean", "deploy", "container build", "container run",
    "docs bundle", "eval", "infer", "status", "config", "paths", "env",
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


def test_test_with_a_sanitizer_builds_with_it_before_running(monkeypatch):
    """Builds the tree with the named sanitizer, then runs the tests."""
    seen: list[object] = []
    monkeypatch.setattr(cli, "context", lambda: (object(), object()))
    monkeypatch.setattr(cli.build_svc, "build",
                        lambda cfg, paths, **options: seen.append(options["sanitizer"]) or 0)
    monkeypatch.setattr(cli.test_svc, "test",
                        lambda cfg, paths, **options: seen.append("test") or 0)
    result = _invoke("test", "--label", "small", "--sanitize", "address")
    assert result.exit_code == 0, result.output
    assert seen == [cli.build_svc.Sanitizer.address, "test"]


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


def _published_checkout(root: Path) -> None:
    """Writes a SushiTrack root marker, a one-page docs/ tree and a git commit that holds them."""
    marker = root / "include" / "SushiTrack" / "c_api" / "sushitrack_c.h"
    marker.parent.mkdir(parents=True)
    marker.write_text("", encoding="utf-8")
    guides = root / "docs" / "guides"
    guides.mkdir(parents=True)
    (root / "docs" / "publish.toml").write_text(K_PUBLISH_LIST, encoding="utf-8")
    (root / "docs" / "README.md").write_text(
        "# Sample\n\n- [Using](guides/USING.md)\n", encoding="utf-8")
    (guides / "USING.md").write_text("# Using\n\nRun it.\n", encoding="utf-8")
    for arguments in (("init", "--quiet"), ("add", "--all"),
                      (*K_GIT_IDENTITY, "commit", "--quiet", "--message", "docs")):
        subprocess.run(["git", *arguments], cwd=root, check=True, capture_output=True)


def test_docs_bundle_without_a_release_is_a_usage_error():
    """Exits 2 and names `--release` when `st docs bundle` is given none."""
    result = _invoke("docs", "bundle")
    assert result.exit_code == 2
    assert "--release" in result.output


def test_bare_docs_prints_the_group_help_and_exits_two():
    """Prints the group's help and exits 2, because st builds no API reference."""
    result = _invoke("docs")
    assert result.exit_code == 2
    assert "bundle" in result.output


def test_docs_bundle_writes_the_archive_of_the_checkout_it_runs_in(
        tmp_path, monkeypatch, recorder):
    """Writes `docs-bundle-1.2.3.tar.gz` and its digest file, and reports both lines."""
    _published_checkout(tmp_path)
    monkeypatch.chdir(tmp_path)
    spy = recorder(cli)
    result = _invoke("docs", "bundle", "--release", "1.2.3")
    assert result.exit_code == 0, result.output
    bundle = tmp_path / "build" / "docs" / "bundle"
    assert (bundle / "docs-bundle-1.2.3.tar.gz").is_file()
    assert (bundle / "docs-bundle-1.2.3.tar.gz.sha256").is_file()
    assert spy.said("docs-bundle-1.2.3.tar.gz (1 pages)")
    assert spy.said("sha256 ")


def _broken_checkout(root) -> None:
    """Writes a SushiTrack root marker and a cli/config.toml that is not valid TOML."""
    marker = root / "include" / "SushiTrack" / "c_api" / "sushitrack_c.h"
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
