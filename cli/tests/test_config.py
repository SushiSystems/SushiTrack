"""The sectioned config layering: defaults -> config.toml -> config.local.toml
-> ST_* env, with a per-platform overlay. Mirrors the sibling CLIs' own
test_config.py, adapted to this CLI's sectioned schema (see config.py's
module docstring for why it is sectioned rather than flat)."""

from __future__ import annotations

import platform

from sushitrack_cli import config as config_module


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
        f"[build]\ncmake = \"cmake-default\"\n"
        f"[platform.{platform.system().lower()}]\nbuild.cmake = \"cmake-here\"\n"
        f"[platform.{other}]\nbuild.cmake = \"cmake-elsewhere\"\n",
        encoding="utf-8")

    cfg = config_module.load_config(tmp_path)

    assert cfg.build_cmake == "cmake-here"


def test_a_bool_field_coerces_common_truthy_env_spellings(tmp_path, monkeypatch):
    monkeypatch.setattr(config_module, "config_dir", lambda root=None: tmp_path)
    monkeypatch.setenv("ST_DOCKER_GPUS", "no")

    cfg = config_module.load_config(tmp_path)

    assert cfg.docker_gpus is False
