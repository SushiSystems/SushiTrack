# test_config.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""Tests the config layering: defaults, config.toml, config.local.toml, ST_* env, platform."""

from __future__ import annotations

import platform

import pytest

from sushitrack_cli import config as config_module
from sushitrack_cli.config import load_config

pytestmark = pytest.mark.medium


def _cli_folder(root, name):
    """Return the path of *name* in the cli/ folder of the checkout at *root*, made if needed."""
    folder = root / "cli"
    folder.mkdir(exist_ok=True)
    return folder / name


@pytest.fixture
def st_checkout(monkeypatch, tmp_path):
    """Return this checkout's root with tool env cleared and an empty workspace."""
    for var in config_module.PROFILE.env_overrides().values():
        monkeypatch.delenv(var, raising=False)
    monkeypatch.setenv("SUSHISTACK_HOME", str(tmp_path))
    return config_module.find_project_root()


def test_defaults_are_used_when_no_file_or_env_overrides(tmp_path, monkeypatch):
    for var in config_module.ENV_OVERRIDES.values():
        monkeypatch.delenv(var, raising=False)

    cfg = config_module.load_config(tmp_path)

    assert cfg.build_type == "Debug"
    assert cfg.sources["build_type"] == "default"


def test_config_toml_overrides_a_default(tmp_path):
    _cli_folder(tmp_path, "config.toml").write_text(
        "[build]\ntype = \"Debug\"\n", encoding="utf-8")

    cfg = config_module.load_config(tmp_path)

    assert cfg.build_type == "Debug"
    assert cfg.sources["build_type"] == "config.toml"


def test_config_local_toml_wins_over_config_toml(tmp_path):
    _cli_folder(tmp_path, "config.toml").write_text(
        "[build]\ntype = \"Debug\"\n", encoding="utf-8")
    _cli_folder(tmp_path, "config.local.toml").write_text(
        "[build]\ntype = \"RelWithDebInfo\"\n", encoding="utf-8")

    cfg = config_module.load_config(tmp_path)

    assert cfg.build_type == "RelWithDebInfo"
    assert cfg.sources["build_type"] == "config.local.toml"


def test_an_env_override_wins_over_every_file(tmp_path, monkeypatch):
    _cli_folder(tmp_path, "config.toml").write_text(
        "[build]\ntype = \"Debug\"\n", encoding="utf-8")
    monkeypatch.setenv("ST_BUILD_TYPE", "MinSizeRel")

    cfg = config_module.load_config(tmp_path)

    assert cfg.build_type == "MinSizeRel"
    assert cfg.sources["build_type"] == "env:ST_BUILD_TYPE"


@pytest.mark.parametrize(("current", "other"), [
    ("Windows", "Linux"),
    ("Linux", "Windows"),
    ("Darwin", "Linux"),
])
def test_a_platform_table_overlays_the_current_platform_only(tmp_path, monkeypatch, current, other):
    """Applies the table of the platform the process runs on and not the table of another."""
    monkeypatch.setattr(platform, "system", lambda: current)
    _cli_folder(tmp_path, "config.toml").write_text(
        '[build]\ntype = "Debug"\n'
        f'[platform.{current.lower()}]\nbuild.type = "MinSizeRel"\n'
        f'[platform.{other.lower()}]\nbuild.type = "RelWithDebInfo"\n',
        encoding="utf-8")

    cfg = config_module.load_config(tmp_path)

    assert cfg.build_type == "MinSizeRel"


@pytest.mark.parametrize(("spelling", "expected"), [
    ("1", True), ("true", True), ("YES", True), (" on ", True),
    ("0", False), ("no", False), ("off", False), ("maybe", False),
])
def test_a_bool_field_coerces_common_truthy_env_spellings(
        tmp_path, monkeypatch, spelling, expected):
    """Assert each truthy spelling of a bool variable reads as True and anything else as False."""
    monkeypatch.setenv("ST_DOCKER_GPUS", spelling)

    cfg = config_module.load_config(tmp_path)

    assert cfg.docker_gpus is expected


@pytest.mark.parametrize(("system", "generator"), [
    ("Windows", "Ninja"),
    ("Linux", ""),
    ("Darwin", ""),
])
def test_tool_settings_carry_the_platform_generator(st_checkout, monkeypatch, system, generator):
    """Reads Ninja from the shipped config.toml on Windows and CMake's default elsewhere."""
    monkeypatch.setattr(platform, "system", lambda: system)

    cfg = load_config(st_checkout)

    assert cfg.tool.generator == generator


def test_legacy_environment_names_still_override(st_checkout, monkeypatch):
    """Assert ST_CMAKE, ST_CTEST and ST_GENERATOR still reach the tool settings."""
    monkeypatch.setenv("ST_CMAKE", "C:/x/cmake.exe")
    monkeypatch.setenv("ST_CTEST", "C:/x/ctest.exe")
    monkeypatch.setenv("ST_GENERATOR", "Unix Makefiles")
    tool = load_config(st_checkout).tool
    assert (tool.cmake_exe, tool.ctest_exe, tool.generator) == (
        "C:/x/cmake.exe", "C:/x/ctest.exe", "Unix Makefiles")


@pytest.mark.parametrize("field", [
    "vcpkg_root", "vs_vcvars", "ninja_exe", "pkgconf_exe", "doxygen_exe", "vcpkg_triplet",
])
def test_tool_settings_carry_the_fields_provision_reads(st_checkout, monkeypatch, field):
    """Loads each provision field from the variable the profile names for it."""
    monkeypatch.setenv(config_module.PROFILE.env_overrides()[field], "value-from-the-environment")

    tool = load_config(st_checkout).tool

    assert getattr(tool, field) == "value-from-the-environment"


def test_load_config_warns_once_per_legacy_key(tmp_path):
    _cli_folder(tmp_path, "config.local.toml").write_text(
        '[build]\ncmake = "x"\ngenerator = "y"\n', encoding="utf-8")
    said = []

    config_module.load_config(tmp_path, warn=said.append)

    assert len(said) == 2
    assert all("config.local.toml" in line for line in said)
    assert "[tool] cmake_exe" in said[0]


def test_a_key_no_setting_reads_is_warned_and_ignored(tmp_path):
    _cli_folder(tmp_path, "config.local.toml").write_text(
        '[build]\nno_such_key = 1\n', encoding="utf-8")
    said = []

    config_module.load_config(tmp_path, warn=said.append)

    assert len(said) == 1
    assert "build.no_such_key" in said[0]
    assert "config.local.toml" in said[0]


def test_an_integer_field_that_cannot_read_its_value_keeps_it_and_warns(tmp_path, monkeypatch):
    """Keeps the default of a field whose variable is not an integer and says so."""
    monkeypatch.setenv("ST_JOBS", "many")
    said = []

    cfg = config_module.load_config(tmp_path, warn=said.append)

    assert cfg.build_jobs == 0
    assert said == ["env:ST_JOBS: 'many' is not an integer; keeping 0"]
