# test_library_packages.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""What `st` asks of the library packages, and what it takes from them."""

from __future__ import annotations

import os
from pathlib import Path
from types import SimpleNamespace

import pytest
from sushicore.cmake_driver import CMakeDriver
from sushicore.provision.sushi_packages import PackageError

from conftest import RecordingConsole
from sushitrack_cli import config, env, library_packages
from sushitrack_cli.services import build as build_svc

pytestmark = pytest.mark.medium

K_LOG_CONFIG = "/store/sushilog/abc/prefix/lib/cmake/SushiLog"
K_PROFILER_CONFIG = "/store/sushiprofiler/def/prefix/lib/cmake/SushiProfiler"
K_RUN_DIRS = ["/store/sushiprofiler/def/prefix/lib", "/store/sushilog/abc/prefix/lib"]


class Packages:
    """Stands in for the packages of SushiLog and SushiProfiler."""

    def names(self):
        """Return the library names."""
        return ["sushilog", "sushiprofiler"]

    def definitions(self):
        """Return the config folder definitions."""
        return [f"-DSushiLog_DIR={K_LOG_CONFIG}", f"-DSushiProfiler_DIR={K_PROFILER_CONFIG}"]

    def run_dirs(self, platform):
        """Return the run-time folders."""
        return list(K_RUN_DIRS)


class RecordingRunner:
    """Record argv and the environment instead of running them."""

    def __init__(self):
        self.calls = []
        self.environments = []

    def run(self, cmd, cwd, env=None):
        """Record *cmd* and its environment, and succeed."""
        self.calls.append(list(cmd))
        self.environments.append(env)
        return 0


def _tool(asked, windows=False, compiler="clang++"):
    """Build the tool settings of st with a resolver that records what it is asked."""
    def sushi_packages(root, build_type, options=None, report=print, warn=print, compiler=""):
        asked.append({"build_type": build_type, "options": dict(options or {}),
                      "compiler": compiler})
        return Packages()

    return SimpleNamespace(
        cmake_exe="", ctest_exe="", generator="Ninja", is_windows=windows,
        platform="windows" if windows else "linux", vs_vcvars="", expand=lambda v: v,
        sushi_definitions=lambda root, packaged=None: [], sushi_packages=sushi_packages,
        resolved_compiler=lambda root: compiler)


@pytest.fixture
def runner(monkeypatch, recorder):
    """Swap in a recording runner and a console that records."""
    spy = recorder(build_svc)
    rec = RecordingRunner()
    monkeypatch.setattr(build_svc, "DRIVER", CMakeDriver(RecordingConsole(), rec))
    monkeypatch.setattr(build_svc, "load_build_env",
                        lambda tool, directory: {"LD_LIBRARY_PATH": "/old", "seen": directory})
    rec.console = spy
    return rec


def _build(tmp_path, tool, **options):
    """Run the build service over a scratch checkout; return its code and its paths."""
    paths = SimpleNamespace(root=tmp_path, build=tmp_path / "build")
    cfg = SimpleNamespace(tool=tool, build_type="Release", jobs=lambda: 2)
    return build_svc.build(cfg, paths, tests=False, **options), paths


def test_the_build_asks_for_the_closure_with_the_compiler_and_the_type(tmp_path, runner):
    asked = []

    code, _ = _build(tmp_path, _tool(asked), build_type="Debug")

    assert code == 0
    assert asked == [{"build_type": "debug", "options": {}, "compiler": "clang++"}]


def test_a_minimum_size_build_links_release_packages(tmp_path, runner):
    asked = []

    _build(tmp_path, _tool(asked), build_type="MinSizeRel")

    assert asked[0]["build_type"] == "release"


def test_the_configure_passes_each_package_the_compiler_and_no_checkout_directory(tmp_path,
                                                                                runner):
    code, paths = _build(tmp_path, _tool([]))

    assert code == 0
    configure = runner.calls[0]
    assert f"-DSushiLog_DIR={K_LOG_CONFIG}" in configure
    assert f"-DSushiProfiler_DIR={K_PROFILER_CONFIG}" in configure
    assert "-DCMAKE_CXX_COMPILER=clang++" in configure
    assert not [item for item in configure if item.startswith("-DSUSHILOG_DIR")]
    stamp = (paths.build / build_svc.STAMP_NAME).read_text(encoding="utf-8")
    assert f"-DSushiLog_DIR={K_LOG_CONFIG}" in stamp and "-DCMAKE_CXX_COMPILER=clang++" in stamp


def test_the_run_dirs_are_recorded_in_the_tree_after_a_configure(tmp_path, runner):
    code, paths = _build(tmp_path, _tool([]))

    assert code == 0
    assert library_packages.read_run_dirs(paths.build) == K_RUN_DIRS


def test_a_tree_that_is_not_reconfigured_still_records_the_run_dirs(tmp_path, runner):
    _build(tmp_path, _tool([]))
    tree = tmp_path / "build"
    (tree / "build.ninja").write_text("", encoding="utf-8")
    (tree / "CMakeCache.txt").write_text(
        f"CMAKE_HOME_DIRECTORY:INTERNAL={tmp_path}\n", encoding="utf-8")
    (tree / library_packages.K_RUN_DIRS_FILE).unlink()
    runner.calls.clear()

    code, paths = _build(tmp_path, _tool([]))

    assert code == 0 and "-S" not in runner.calls[0]
    assert library_packages.read_run_dirs(paths.build) == K_RUN_DIRS


def test_a_package_that_cannot_be_chosen_ends_the_build_in_one_error_line(tmp_path, runner):
    def refuse(*arguments, **options):
        raise PackageError("SushiLog has no cli and no stored package.")

    tool = _tool([])
    tool.sushi_packages = refuse

    code, _ = _build(tmp_path, tool)

    assert code == 1
    assert runner.console.said("SushiLog has no cli and no stored package.")
    assert runner.calls == []


def test_the_run_dirs_lead_the_search_path_of_every_command_that_runs_the_tree(tmp_path):
    library_packages.record_run_dirs(tmp_path, K_RUN_DIRS)

    environment = env.load_build_env(_tool([]), tmp_path)

    assert environment["LD_LIBRARY_PATH"].split(os.pathsep)[:2] == K_RUN_DIRS


def test_the_windows_run_dirs_lead_path():
    environment = {"PATH": "C:/old"}

    library_packages.prepend_run_dirs(_tool([], windows=True), environment, ["C:/pkg/bin"])

    assert environment["PATH"].startswith("C:/pkg/bin")


def test_a_tree_without_a_record_adds_nothing_to_the_path(tmp_path):
    assert library_packages.read_run_dirs(tmp_path) == []


def test_the_compiler_the_environment_names_outranks_the_configured_one(monkeypatch):
    tool = config.ToolSettings(cxx="/pinned/clang++", stack_cxx="/stack/clang++")

    assert tool.resolved_compiler(Path(".")) == "/stack/clang++"


def test_load_tool_reads_the_stack_compiler_from_the_environment(monkeypatch):
    monkeypatch.setattr(config, "config_dir", lambda root=None: (_ for _ in ()).throw(SystemExit()))
    monkeypatch.setattr(config, "load_tool_config", lambda cls, *args: cls())
    monkeypatch.setenv(config.K_STACK_CXX_ENV, "/env/clang++")

    assert config.load_tool().stack_cxx == "/env/clang++"
