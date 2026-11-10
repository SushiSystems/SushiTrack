# test_console.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""Tests that importing the console builds nothing, so st --help runs outside a checkout.

The failure these tests guard against is in docs/architecture/CLI_PACKAGE.md.
"""

from __future__ import annotations

import importlib

import pytest

from sushitrack_cli import config

pytestmark = pytest.mark.medium


def _fresh_console_module():
    """Import (or re-import) sushitrack_cli.console with a clean module state."""
    import sushitrack_cli.console as console_mod
    importlib.reload(console_mod)
    return console_mod


def test_import_does_not_build_console():
    """Importing the module must not materialise the console."""
    console_mod = _fresh_console_module()
    assert console_mod.is_built() is False


def test_output_helper_falls_back_outside_project(monkeypatch, tmp_path, capsys):
    """console.info(...) outside a project prints the line and layers no config file."""
    monkeypatch.chdir(tmp_path)
    console_mod = _fresh_console_module()

    console_mod.info("hello from outside a checkout")

    assert "hello from outside a checkout" in capsys.readouterr().out
    assert list(console_mod.config_sources()) == []


def test_sources_inside_project_are_this_repos_config():
    """Inside a checkout the console layers this CLI's own config files, in the config order."""
    console_mod = _fresh_console_module()
    names = [p.name for p in console_mod.config_sources()]
    assert names == list(config.CONFIG_NAMES)


def test_console_attribute_resolves(monkeypatch, tmp_path):
    """The raw Rich `console` attribute resolves lazily too."""
    monkeypatch.chdir(tmp_path)
    console_mod = _fresh_console_module()
    assert console_mod.console is not None


def test_unknown_attribute_raises_attribute_error():
    console_mod = _fresh_console_module()
    with pytest.raises(AttributeError):
        _ = console_mod.does_not_exist


def test_current_builds_the_console_once(monkeypatch, tmp_path):
    """current() builds the console on the first call and hands the same one afterwards."""
    monkeypatch.chdir(tmp_path)
    console_mod = _fresh_console_module()

    first = console_mod.current()

    assert console_mod.current() is first
    assert console_mod.is_built() is True
