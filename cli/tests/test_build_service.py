# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License, Version 2.0. See LICENSE.
"""What `st build` hands cmake, pinned through a recording runner."""

from __future__ import annotations

from types import SimpleNamespace

import pytest
from sushicore.cmake_driver import CMakeDriver

from conftest import RecordingConsole
from sushitrack_cli.services import build as build_svc


class RecordingRunner:
    """Record argv instead of running it."""

    def __init__(self):
        self.calls = []

    def run(self, cmd, cwd, env=None):
        """Record *cmd* and succeed."""
        self.calls.append(list(cmd))
        return 0


def _cfg(generator="", windows=True):
    """Build a Config slice with st's tool settings."""
    tool = SimpleNamespace(cmake_exe="", ctest_exe="", generator=generator,
                           is_windows=windows, platform="x", vs_vcvars="",
                           expand=lambda v: v)
    return SimpleNamespace(tool=tool, build_type="Release", jobs=lambda: 4)


@pytest.fixture
def runner(monkeypatch, recorder):
    """Swap in a recording runner and a quiet environment."""
    recorder(build_svc)
    rec = RecordingRunner()
    monkeypatch.setattr(build_svc, "DRIVER", CMakeDriver(RecordingConsole(), rec))
    monkeypatch.setattr(build_svc, "load_build_env", lambda tool, d: {})
    return rec


@pytest.mark.parametrize("windows, expected", [(True, "Ninja"), (False, "")])
def test_the_default_generator_follows_the_host(windows, expected):
    """Defaults to Ninja on Windows and CMake's choice elsewhere."""
    assert build_svc.generator(_cfg(windows=windows)) == expected


def test_a_configured_generator_wins():
    """Uses the generator the config names."""
    assert build_svc.generator(_cfg(generator="Unix Makefiles")) == "Unix Makefiles"


def test_a_fresh_tree_is_configured_then_built(tmp_path, runner):
    """Emits the same configure and build argv st always has."""
    paths = SimpleNamespace(root=tmp_path, build=tmp_path / "build")
    assert build_svc.build(_cfg(), paths, build_type="Debug", jobs=3) == 0
    configure, compile_ = runner.calls
    assert configure == ["cmake", "-S", str(tmp_path), "-B", str(paths.build),
                         "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Debug", "-DBUILD_MOT_TEST=OFF",
                         "-DBUILD_UNIT_TEST=ON", "-DBUILD_REGRESSION_TEST=ON",
                         "-DBUILD_INTEGRATION_TEST=ON"]
    assert compile_ == ["cmake", "--build", str(paths.build), "--config", "Debug",
                        "--parallel", "3", "--target", "sushitrack",
                        "--target", "unit_test", "--target", "integration_test",
                        "--target", "regression_test", "--target", "tracker_bridge"]


def test_a_matching_stamp_skips_configure(tmp_path, runner):
    """Builds without reconfiguring when the stamp matches."""
    build = tmp_path / "build"
    build.mkdir()
    (build / "build.ninja").write_text("", encoding="utf-8")
    (build / "CMakeCache.txt").write_text(
        f"CMAKE_HOME_DIRECTORY:INTERNAL={tmp_path}\n", encoding="utf-8")
    (build / build_svc.STAMP_NAME).write_text("Release|OFF|Ninja", encoding="utf-8")
    paths = SimpleNamespace(root=tmp_path, build=build)
    assert build_svc.build(_cfg(), paths, tests=False) == 0
    assert [c[1] for c in runner.calls] == ["--build"]


def test_a_visual_studio_tree_is_not_reconfigured_for_a_missing_makefile(tmp_path, runner):
    """Needs no sentinel file for a multi-config generator."""
    build = tmp_path / "build"
    build.mkdir()
    (build / "CMakeCache.txt").write_text(
        f"CMAKE_HOME_DIRECTORY:INTERNAL={tmp_path}\n", encoding="utf-8")
    gen = "Visual Studio 17 2022"
    (build / build_svc.STAMP_NAME).write_text(f"Release|OFF|{gen}", encoding="utf-8")
    paths = SimpleNamespace(root=tmp_path, build=build)
    assert build_svc.build(_cfg(generator=gen), paths, tests=False) == 0
    assert len(runner.calls) == 1
