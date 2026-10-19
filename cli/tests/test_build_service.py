# test_build_service.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
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


def _cfg(generator="", windows=True, sushi=()):
    """Build a Config slice with st's tool settings."""
    tool = SimpleNamespace(cmake_exe="", ctest_exe="", generator=generator,
                           is_windows=windows, platform="x", vs_vcvars="",
                           expand=lambda v: v, sushi_definitions=lambda root: list(sushi))
    return SimpleNamespace(tool=tool, build_type="Release", jobs=lambda: 4)


def test_a_pinned_sushi_library_reaches_the_configure(tmp_path, runner):
    """Hands each pinned Sushi library to CMake as -D<NAME>_DIR and records it in the stamp."""
    paths = SimpleNamespace(root=tmp_path, build=tmp_path / "build")
    cfg = _cfg(sushi=["-DSUSHILOG_DIR=/stack/sushilog"])
    assert build_svc.build(cfg, paths, tests=False) == 0
    assert "-DSUSHILOG_DIR=/stack/sushilog" in runner.calls[0]
    stamp = (paths.build / build_svc.STAMP_NAME).read_text(encoding="utf-8")
    assert "-DSUSHILOG_DIR=/stack/sushilog" in stamp


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
                         "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Debug",
                         "-DSUSHITRACK_BUILD_TESTS=ON", "-DBUILD_REGRESSION_TEST=ON"]
    assert compile_ == ["cmake", "--build", str(paths.build), "--config", "Debug",
                        "--parallel", "3", "--target", "sushitrack",
                        "--target", "sushitrack_tests",
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


def _configured_tree(tmp_path, stamp, sentinel=None, home=None):
    """Lay out a build tree with a cache, a stamp and an optional sentinel file."""
    build = tmp_path / "build"
    build.mkdir()
    if sentinel:
        (build / sentinel).write_text("", encoding="utf-8")
    (build / "CMakeCache.txt").write_text(
        f"CMAKE_HOME_DIRECTORY:INTERNAL={home or tmp_path}\n", encoding="utf-8")
    (build / build_svc.STAMP_NAME).write_text(stamp, encoding="utf-8")
    return SimpleNamespace(root=tmp_path, build=build)


def test_an_empty_generator_needs_no_makefile(tmp_path, runner):
    """Reuses a tree built by CMake's default generator whatever file it left."""
    paths = _configured_tree(tmp_path, "Release|OFF|", sentinel="build.ninja")
    assert build_svc.build(_cfg(windows=False), paths, tests=False) == 0
    assert [c[1] for c in runner.calls] == ["--build"]


def test_a_stamp_mismatch_reconfigures(tmp_path, runner):
    """Reconfigures when the stamp names other settings."""
    paths = _configured_tree(tmp_path, "Debug|OFF|Ninja", sentinel="build.ninja")
    assert build_svc.build(_cfg(), paths, tests=False) == 0
    assert [c[1] for c in runner.calls] == ["-S", "--build"]
    stamp = (paths.build / build_svc.STAMP_NAME).read_text(encoding="utf-8")
    assert stamp == "Release|OFF|Ninja"


@pytest.mark.parametrize("generator, sentinel",
                         [("", "Makefile"), ("Visual Studio 17 2022", None)])
def test_a_stale_tree_reconfigures(tmp_path, runner, generator, sentinel):
    """Reconfigures a tree whose cache names another source directory."""
    paths = _configured_tree(tmp_path, f"Release|OFF|{generator}", sentinel=sentinel,
                             home=tmp_path / "elsewhere")
    assert build_svc.build(_cfg(generator=generator, windows=False), paths,
                           tests=False) == 0
    assert [c[1] for c in runner.calls] == ["-S", "--build"]


def test_clean_removes_the_tree_before_configuring(tmp_path, runner):
    """Deletes the old tree and configures a fresh one."""
    paths = _configured_tree(tmp_path, "Release|OFF|Ninja", sentinel="build.ninja")
    (paths.build / "leftover.o").write_text("", encoding="utf-8")
    assert build_svc.build(_cfg(), paths, clean=True, tests=False) == 0
    assert not (paths.build / "leftover.o").exists()
    assert [c[1] for c in runner.calls] == ["-S", "--build"]


@pytest.mark.parametrize("stamp, clean", [("Release|OFF|Ninja", True),
                                          ("Debug|OFF|Ninja", False)])
def test_build_stops_at_a_tree_it_could_not_remove(tmp_path, monkeypatch, runner,
                                                   stamp, clean):
    """Returns 1 and runs no cmake when the clean or the reconfigure removal fails."""
    monkeypatch.setattr(build_svc.shutil, "rmtree", lambda target, ignore_errors=False: None)
    paths = _configured_tree(tmp_path, stamp, sentinel="build.ninja")
    assert build_svc.build(_cfg(), paths, clean=clean, tests=False) == 1
    assert runner.calls == []


def test_clean_reports_a_tree_it_could_not_remove(tmp_path, monkeypatch, recorder):
    """Returns 1 and names the directory when the build tree survives the removal."""
    spy = recorder(build_svc)
    monkeypatch.setattr(build_svc.shutil, "rmtree", lambda target, ignore_errors=False: None)
    build_dir = tmp_path / "build"
    build_dir.mkdir()
    paths = SimpleNamespace(build=build_dir, package=tmp_path / "package")
    assert build_svc.clean(paths) == 1
    assert spy.said(f"Could not remove {build_dir}")
    assert not spy.said("Removed:")


def test_clean_removes_both_trees(tmp_path, recorder):
    """Returns 0 and leaves neither the build tree nor the package directory."""
    recorder(build_svc)
    paths = SimpleNamespace(build=tmp_path / "build", package=tmp_path / "package")
    paths.build.mkdir()
    paths.package.mkdir()
    assert build_svc.clean(paths) == 0
    assert not paths.build.exists() and not paths.package.exists()
