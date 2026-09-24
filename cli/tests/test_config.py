# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License, Version 2.0. See LICENSE.
"""The sectioned config layering: defaults -> config.toml -> config.local.toml
-> ST_* env, with a per-platform overlay. Mirrors the sibling CLIs' own
test_config.py, adapted to this CLI's sectioned schema (see config.py's
module docstring for why it is sectioned rather than flat)."""

from __future__ import annotations

import platform
from dataclasses import fields

import pytest
from sushicore.provision.config import ProvisionSettings

from sushitrack_cli import config as config_module
from sushitrack_cli.config import IS_WINDOWS, Config, ToolSettings, load_config


@pytest.fixture
def st_checkout(monkeypatch):
    """Return this SushiTrack checkout's root with the tool env overrides cleared."""
    for var in config_module.PROFILE.env_overrides().values():
        monkeypatch.delenv(var, raising=False)
    return config_module.find_project_root()


def test_defaults_are_used_when_no_file_or_env_overrides(tmp_path, monkeypatch):
    monkeypatch.setattr(config_module, "config_dir", lambda root=None: tmp_path)
    for var in config_module.ENV_OVERRIDES.values():
        monkeypatch.delenv(var, raising=False)

    cfg = config_module.load_config(tmp_path)

    assert cfg.build_type == "Release"
    assert cfg.sources["build_type"] == "default"


def test_config_toml_overrides_a_default(tmp_path, monkeypatch):
    monkeypatch.setattr(config_module, "config_dir", lambda root=None: tmp_path)
    (tmp_path / "config.toml").write_text(
        "[build]\ntype = \"Debug\"\n", encoding="utf-8")

    cfg = config_module.load_config(tmp_path)

    assert cfg.build_type == "Debug"
    assert cfg.sources["build_type"] == "config.toml"


def test_config_local_toml_wins_over_config_toml(tmp_path, monkeypatch):
    monkeypatch.setattr(config_module, "config_dir", lambda root=None: tmp_path)
    (tmp_path / "config.toml").write_text(
        "[build]\ntype = \"Debug\"\n", encoding="utf-8")
    (tmp_path / "config.local.toml").write_text(
        "[build]\ntype = \"RelWithDebInfo\"\n", encoding="utf-8")

    cfg = config_module.load_config(tmp_path)

    assert cfg.build_type == "RelWithDebInfo"
    assert cfg.sources["build_type"] == "config.local.toml"


def test_an_env_override_wins_over_every_file(tmp_path, monkeypatch):
    monkeypatch.setattr(config_module, "config_dir", lambda root=None: tmp_path)
    (tmp_path / "config.toml").write_text(
        "[build]\ntype = \"Debug\"\n", encoding="utf-8")
    monkeypatch.setenv("ST_BUILD_TYPE", "MinSizeRel")

    cfg = config_module.load_config(tmp_path)

    assert cfg.build_type == "MinSizeRel"
    assert cfg.sources["build_type"] == "env:ST_BUILD_TYPE"


def test_a_platform_table_overlays_the_current_platform_only(tmp_path, monkeypatch):
    monkeypatch.setattr(config_module, "config_dir", lambda root=None: tmp_path)
    other = "linux" if platform.system().lower() == "windows" else "windows"
    (tmp_path / "config.toml").write_text(
        f"[build]\ntype = \"Debug\"\n"
        f"[platform.{platform.system().lower()}]\nbuild.type = \"MinSizeRel\"\n"
        f"[platform.{other}]\nbuild.type = \"RelWithDebInfo\"\n",
        encoding="utf-8")

    cfg = config_module.load_config(tmp_path)

    assert cfg.build_type == "MinSizeRel"


def test_a_bool_field_coerces_common_truthy_env_spellings(tmp_path, monkeypatch):
    monkeypatch.setattr(config_module, "config_dir", lambda root=None: tmp_path)
    monkeypatch.setenv("ST_DOCKER_GPUS", "no")

    cfg = config_module.load_config(tmp_path)

    assert cfg.docker_gpus is False


def test_tool_settings_carry_the_platform_generator(st_checkout, monkeypatch):
    """Assert the split config keeps Ninja on Windows and CMake's default elsewhere."""
    cfg = load_config(st_checkout)
    expected = "Ninja" if IS_WINDOWS else ""
    assert cfg.tool.generator == expected


def test_legacy_environment_names_still_override(st_checkout, monkeypatch):
    """Assert ST_CMAKE, ST_CTEST and ST_GENERATOR still reach the tool settings."""
    monkeypatch.setenv("ST_CMAKE", "C:/x/cmake.exe")
    monkeypatch.setenv("ST_CTEST", "C:/x/ctest.exe")
    monkeypatch.setenv("ST_GENERATOR", "Unix Makefiles")
    tool = load_config(st_checkout).tool
    assert (tool.cmake_exe, tool.ctest_exe, tool.generator) == (
        "C:/x/cmake.exe", "C:/x/ctest.exe", "Unix Makefiles")


def test_config_no_longer_has_build_tool_fields():
    """Assert the tool fields moved out of the sectioned config."""
    names = {f.name for f in fields(Config)}
    assert not names & {"build_cmake", "build_ctest", "build_generator"}


def test_tool_settings_are_a_provision_config():
    """Assert st's tool settings carry every field provision reads."""
    assert issubclass(ToolSettings, ProvisionSettings)
