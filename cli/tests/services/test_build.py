# test_build.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""What `st build` hands cmake, pinned through a recording runner."""

from __future__ import annotations

from types import SimpleNamespace

import pytest
from sushicore import build_admission
from sushicore.build_tree import K_DEFAULT_BUILD_TYPE
from sushicore.cmake_driver import CMakeDriver
from sushicore.errors import ConfigError

from conftest import RecordingConsole
from sushitrack_cli.config import Config
from sushitrack_cli.services import build as build_svc

pytestmark = pytest.mark.medium

K_THREADS = 12
K_CCACHE = "/tools/ccache"


class RecordingRunner:
    """Record argv instead of running it."""

    def __init__(self):
        self.calls = []

    def run(self, cmd, cwd, env=None):
        """Record *cmd* and succeed."""
        self.calls.append(list(cmd))
        return 0


class NoPackages:
    """Stands in for a package set that holds no library."""

    def names(self):
        """Return no name."""
        return []

    def definitions(self):
        """Return no definition."""
        return []

    def run_dirs(self, platform):
        """Return no folder."""
        return []


def _cfg(generator="", windows=True, sushi=()):
    """Build a Config with st's tool settings, a pin that is no package, and no compiler."""
    tool = SimpleNamespace(cmake_exe="", ctest_exe="", generator=generator, ccache_exe="",
                           is_windows=windows, platform="x", vs_vcvars="",
                           expand=lambda v: v,
                           sushi_definitions=lambda root, packaged=None: list(sushi),
                           sushi_packages=lambda *args, **kwargs: NoPackages(),
                           resolved_compiler=lambda root: "")
    cfg = Config()
    cfg.tool = tool
    return cfg


def _paths(tmp_path, name="default", build_type="debug"):
    """Return the paths a build reads, with the tree of *name* and *build_type*."""
    root = tmp_path / "build"
    return SimpleNamespace(root=tmp_path, build_root=root, build=root / f"{name}-{build_type}",
                           package=tmp_path / "package")


def _launcher_stamp(tmp_path):
    """Return the launcher entries the default tree's stamp holds, sorted, as -D options."""
    return sorted(f"-D{entry}" for entry in build_admission.launcher_defines(
        _paths(tmp_path).build))


@pytest.fixture
def runner(monkeypatch, recorder):
    """Swap in a recording runner, a quiet environment and a fixed machine."""
    recorder(build_svc)
    rec = RecordingRunner()
    monkeypatch.setattr(build_svc, "DRIVER", CMakeDriver(RecordingConsole(), rec))
    monkeypatch.setattr(build_svc, "load_build_env", lambda tool, d: {})
    monkeypatch.setattr(build_admission, "compiler_cache", lambda configured="": "")
    monkeypatch.setattr(build_admission, "hardware_threads", lambda: K_THREADS)
    monkeypatch.setattr("sushitrack_cli.config.hardware_threads", lambda: K_THREADS)
    return rec


def _configured_tree(tmp_path, stamp, sentinel=None, home=None):
    """Lay out the default Debug tree with a cache, a stamp and an optional sentinel file."""
    paths = _paths(tmp_path)
    paths.build.mkdir(parents=True)
    if sentinel:
        (paths.build / sentinel).write_text("", encoding="utf-8")
    (paths.build / "CMakeCache.txt").write_text(
        f"CMAKE_HOME_DIRECTORY:INTERNAL={home or tmp_path}\nCMAKE_BUILD_TYPE:STRING=Debug\n",
        encoding="utf-8")
    (paths.build / build_svc.STAMP_NAME).write_text(stamp, encoding="utf-8")
    return paths


def test_a_pinned_sushi_library_reaches_the_configure(tmp_path, runner):
    """Hands a pinned Sushi library that is no package to CMake as -D<NAME>_DIR."""
    paths = _paths(tmp_path)
    cfg = _cfg(sushi=["-DSUSHILOG_DIR=/stack/sushilog"])
    assert build_svc.build(cfg, paths, tests=False) == 0
    assert "-DSUSHILOG_DIR=/stack/sushilog" in runner.calls[0]
    stamp = (paths.build / build_svc.STAMP_NAME).read_text(encoding="utf-8")
    assert "-DSUSHILOG_DIR=/stack/sushilog" in stamp


@pytest.mark.parametrize("sanitizer, name", [(build_svc.Sanitizer.address, "asan"),
                                             (build_svc.Sanitizer.thread, "tsan")])
def test_a_sanitizer_reaches_the_configure_and_builds_its_own_tree(
        tmp_path, runner, sanitizer, name):
    """Hands the sanitizer to CMake as SUSHITRACK_SANITIZE, in build/<name>-debug."""
    paths = _paths(tmp_path)
    assert build_svc.build(_cfg(), paths, tests=False, sanitizer=sanitizer) == 0
    define = f"-DSUSHITRACK_SANITIZE={sanitizer.value}"
    tree = tmp_path / "build" / f"{name}-debug"
    assert define in runner.calls[0]
    assert runner.calls[0][runner.calls[0].index("-B") + 1] == str(tree)
    assert define in (tree / build_svc.STAMP_NAME).read_text(encoding="utf-8")
    assert not paths.build.exists()


def test_the_default_build_type_is_debug(tmp_path, runner):
    """Configures and compiles Debug when neither the command nor the config names a type."""
    assert Config().build_type == K_DEFAULT_BUILD_TYPE == "Debug"
    assert build_svc.build(_cfg(), _paths(tmp_path), tests=False) == 0
    assert "-DCMAKE_BUILD_TYPE=Debug" in runner.calls[0]
    assert runner.calls[1][runner.calls[1].index("--config") + 1] == "Debug"


@pytest.mark.parametrize("build_type", build_svc.BUILD_TYPES)
def test_each_build_type_has_a_tree_of_its_own(tmp_path, runner, build_type):
    """Names the tree build/default-<type> under the repository root."""
    assert build_svc.build(_cfg(), _paths(tmp_path), build_type=build_type, tests=False) == 0
    tree = tmp_path / "build" / f"default-{build_type.lower()}"
    assert runner.calls[0][runner.calls[0].index("-B") + 1] == str(tree)
    assert runner.calls[1][2] == str(tree)


def test_another_build_type_is_refused_in_an_existing_tree(tmp_path, runner):
    """Raises before any cmake call when the tree was configured as another type."""
    paths = _configured_tree(tmp_path, "OFF|Ninja", sentinel="build.ninja")
    cache = paths.build / "CMakeCache.txt"
    cache.write_text(cache.read_text(encoding="utf-8").replace("Debug", "Release"),
                     encoding="utf-8")
    with pytest.raises(ConfigError, match="Release tree"):
        build_svc.build(_cfg(), paths, tests=False)
    assert runner.calls == []


def test_a_define_cannot_set_the_build_type(tmp_path, runner):
    """Refuses -D CMAKE_BUILD_TYPE, since the type picks the tree."""
    with pytest.raises(ConfigError, match="CMAKE_BUILD_TYPE"):
        build_svc.build(_cfg(), _paths(tmp_path), defines=["-DCMAKE_BUILD_TYPE=Release"],
                        tests=False)
    assert runner.calls == []


def test_a_stopped_gate_refuses_the_build(tmp_path, runner):
    """Raises before any cmake call while the tree's stop file exists."""
    paths = _paths(tmp_path)
    stop = build_admission.stop_file(paths.build)
    stop.parent.mkdir(parents=True)
    stop.write_text("", encoding="utf-8")
    with pytest.raises(build_admission.BuildStoppedError):
        build_svc.build(_cfg(), paths, tests=False)
    assert runner.calls == []


def test_the_configure_carries_both_launcher_entries(tmp_path, runner):
    """Appends the compiler and linker launcher entries of the tree's gate."""
    paths = _paths(tmp_path)
    assert build_svc.build(_cfg(), paths, tests=False) == 0
    entries = build_admission.launcher_defines(paths.build)
    assert len(entries) == 2
    for entry in entries:
        assert f"-D{entry}" in runner.calls[0]


def test_the_launcher_runs_host_compiles_through_the_configured_cache(
        tmp_path, runner, monkeypatch):
    """Names the compiler cache in the launcher when one is found."""
    monkeypatch.setattr(build_admission, "compiler_cache", lambda configured="": K_CCACHE)
    assert build_svc.build(_cfg(), _paths(tmp_path), tests=False) == 0
    launcher = [arg for arg in runner.calls[0] if arg.startswith("-DCMAKE_CXX_COMPILER_LAUNCHER=")]
    assert len(launcher) == 1 and f"--cache;{K_CCACHE}" in launcher[0]


def test_the_compile_takes_the_hardware_threads_without_jobs(tmp_path, runner):
    """Passes the hardware thread count to --parallel when no --jobs is given."""
    assert build_svc.build(_cfg(), _paths(tmp_path), tests=False) == 0
    compile_ = runner.calls[1]
    assert compile_[compile_.index("--parallel") + 1] == str(K_THREADS)


def test_jobs_overrides_the_hardware_threads(tmp_path, runner):
    """Passes --jobs to --parallel in place of the hardware thread count."""
    assert build_svc.build(_cfg(), _paths(tmp_path), jobs=3, tests=False) == 0
    compile_ = runner.calls[1]
    assert compile_[compile_.index("--parallel") + 1] == "3"


def test_a_report_is_printed_and_written(tmp_path, runner, monkeypatch):
    """Measures the compile from the log offset taken before it and writes the report."""
    seen = {}
    monkeypatch.setattr(build_svc.build_report, "ninja_log_offset", lambda tree: 7)

    class Report:
        def lines(self):
            return ["report line"]

        def write(self, tree):
            seen["written"] = tree
            return tree / "build_report.json"

    def of_build(tree, offset, state):
        seen["offset"] = offset
        return Report()

    monkeypatch.setattr(build_svc.build_report, "of_build", of_build)
    paths = _paths(tmp_path)
    assert build_svc.build(_cfg(), paths, tests=False, report=True) == 0
    assert seen == {"offset": 7, "written": paths.build}


def test_the_default_generator_follows_the_host():
    """Defaults to Ninja on Windows and CMake's choice elsewhere."""
    assert build_svc.generator(_cfg(windows=True)) == "Ninja"
    assert build_svc.generator(_cfg(windows=False)) == ""


def test_a_configured_generator_wins():
    """Uses the generator the config names."""
    assert build_svc.generator(_cfg(generator="Unix Makefiles")) == "Unix Makefiles"


def test_a_fresh_tree_is_configured_then_built(tmp_path, runner):
    """Emits the configure and build argv st's policy asks for, launcher entries last."""
    paths = _paths(tmp_path)
    assert build_svc.build(_cfg(), paths, build_type="Debug", jobs=3) == 0
    configure, compile_ = runner.calls
    launchers = [f"-D{entry}" for entry in build_admission.launcher_defines(paths.build)]
    assert configure == ["cmake", "-S", str(tmp_path), "-B", str(paths.build),
                         "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Debug",
                         "-DSUSHITRACK_BUILD_TESTS=ON", *launchers]
    assert compile_ == ["cmake", "--build", str(paths.build), "--config", "Debug",
                        "--parallel", "3", "--target", "sushitrack",
                        "--target", "tracker_bridge", "--target", "mot_engine",
                        "--target", "sushitrack_tests", "--target", "sushitrack_application_tests"]


def test_a_matching_stamp_skips_configure(tmp_path, runner):
    """Builds without reconfiguring when the stamp matches."""
    paths = _configured_tree(tmp_path, "|".join(["OFF", "Ninja", *_launcher_stamp(tmp_path)]),
                             sentinel="build.ninja")
    assert build_svc.build(_cfg(), paths, tests=False) == 0
    assert [c[1] for c in runner.calls] == ["--build"]


def test_a_visual_studio_tree_is_not_reconfigured_for_a_missing_makefile(tmp_path, runner):
    """Needs no sentinel file for a multi-config generator."""
    gen = "Visual Studio 17 2022"
    paths = _configured_tree(tmp_path, "|".join(["OFF", gen, *_launcher_stamp(tmp_path)]))
    assert build_svc.build(_cfg(generator=gen), paths, tests=False) == 0
    assert len(runner.calls) == 1


def test_an_empty_generator_needs_no_makefile(tmp_path, runner):
    """Reuses a tree built by CMake's default generator whatever file it left."""
    paths = _configured_tree(tmp_path, "|".join(["OFF", "", *_launcher_stamp(tmp_path)]),
                             sentinel="build.ninja")
    assert build_svc.build(_cfg(windows=False), paths, tests=False) == 0
    assert [c[1] for c in runner.calls] == ["--build"]


def test_a_stamp_mismatch_reconfigures(tmp_path, runner):
    """Reconfigures when the stamp names other settings."""
    paths = _configured_tree(tmp_path, "ON|Ninja", sentinel="build.ninja")
    assert build_svc.build(_cfg(), paths, tests=False) == 0
    assert [c[1] for c in runner.calls] == ["-S", "--build"]
    stamp = (paths.build / build_svc.STAMP_NAME).read_text(encoding="utf-8")
    assert stamp.startswith("OFF|Ninja|")


@pytest.mark.parametrize("generator, sentinel",
                         [("", "Makefile"), ("Visual Studio 17 2022", None)])
def test_a_stale_tree_reconfigures(tmp_path, runner, generator, sentinel):
    """Reconfigures a tree whose cache names another source directory."""
    paths = _configured_tree(tmp_path, f"OFF|{generator}", sentinel=sentinel,
                             home=tmp_path / "elsewhere")
    assert build_svc.build(_cfg(generator=generator, windows=False), paths,
                           tests=False) == 0
    assert [c[1] for c in runner.calls] == ["-S", "--build"]


def test_clean_removes_the_tree_before_configuring(tmp_path, runner):
    """Deletes the old tree and configures a fresh one."""
    paths = _configured_tree(tmp_path, "OFF|Ninja", sentinel="build.ninja")
    (paths.build / "leftover.o").write_text("", encoding="utf-8")
    assert build_svc.build(_cfg(), paths, clean=True, tests=False) == 0
    assert not (paths.build / "leftover.o").exists()
    assert [c[1] for c in runner.calls] == ["-S", "--build"]


@pytest.mark.parametrize("stamp, clean", [("OFF|Ninja", True), ("ON|Ninja", False)])
def test_build_stops_at_a_tree_it_could_not_remove(tmp_path, monkeypatch, runner,
                                                   stamp, clean):
    """Returns 1 and runs no cmake when the clean or the reconfigure removal fails."""
    monkeypatch.setattr(build_svc.shutil, "rmtree", lambda target, ignore_errors=False: None)
    paths = _configured_tree(tmp_path, stamp, sentinel="build.ninja")
    assert build_svc.build(_cfg(), paths, clean=clean, tests=False) == 1
    assert runner.calls == []


def test_clean_reports_a_tree_it_could_not_remove(tmp_path, monkeypatch, recorder):
    """Returns 1 and names the directory when the build folder survives the removal."""
    spy = recorder(build_svc)
    monkeypatch.setattr(build_svc.shutil, "rmtree", lambda target, ignore_errors=False: None)
    paths = _paths(tmp_path)
    paths.build_root.mkdir()
    assert build_svc.clean(paths) == 1
    assert spy.said(f"Could not remove {paths.build_root}")
    assert not spy.said("Removed:")


def test_clean_removes_every_tree_and_the_package(tmp_path, recorder):
    """Returns 0 and leaves neither the build folder nor the package directory."""
    recorder(build_svc)
    paths = _paths(tmp_path)
    (paths.build_root / "default-release").mkdir(parents=True)
    paths.build.mkdir()
    paths.package.mkdir()
    assert build_svc.clean(paths) == 0
    assert not paths.build_root.exists() and not paths.package.exists()


def test_build_type_values_match_build_types():
    assert {member.value for member in build_svc.BuildType} == \
        {name.lower() for name in build_svc.BUILD_TYPES}
