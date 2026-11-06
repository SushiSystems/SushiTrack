# test_tests.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""What `st test` hands the shared test command."""

from __future__ import annotations

from types import SimpleNamespace

import pytest
from sushicore.cmake_driver import CMakeDriver

from conftest import RecordingConsole
from sushitrack_cli.services import build as build_svc
from sushitrack_cli.services import tests as test_svc

pytestmark = pytest.mark.medium


class DrainRecorder:
    """Record drained argv instead of running it."""

    def __init__(self):
        """Start with no recorded calls."""
        self.calls = []

    def run_drained(self, cmd, cwd, env=None):
        """Record *cmd* and succeed."""
        self.calls.append(list(cmd))
        return 0


def _tree(tmp_path, cache, name="default-release"):
    """Create a configured tree called *name* whose cache holds *cache*."""
    build = tmp_path / "build" / name
    build.mkdir(parents=True)
    (build / "CMakeCache.txt").write_text(
        f"CMAKE_HOME_DIRECTORY:INTERNAL={tmp_path}\n{cache}", encoding="utf-8")
    return build


_TOOL = SimpleNamespace(ctest_exe="", expand=lambda v: v)


def _module(tmp_path, build_type="Release"):
    """Return what `st test` hands the shared command over a project at *tmp_path*."""
    cfg = SimpleNamespace(build_type=build_type, tool=_TOOL)
    paths = SimpleNamespace(root=tmp_path, config=tmp_path / "sushitrack.json", cli=tmp_path)
    return test_svc.module_tests(lambda: (cfg, paths))


@pytest.fixture
def rec(monkeypatch):
    """Swap in a recording runner and an empty build environment."""
    runner = DrainRecorder()
    monkeypatch.setattr(build_svc, "DRIVER", CMakeDriver(RecordingConsole(), runner))
    monkeypatch.setattr(test_svc, "load_build_env", lambda tool, d, config: {})
    return runner


@pytest.mark.parametrize("cache, config_flag", [
    ("CMAKE_BUILD_TYPE:STRING=Release\n", []),
    ("", ["-C", "Release"]),
])
def test_ctest_gets_a_config_only_for_a_multi_config_tree(tmp_path, rec, cache, config_flag):
    build = _tree(tmp_path, cache)
    junit = tmp_path / "junit.xml"
    assert _module(tmp_path).run_ctest(build, label_regex="^(small)$", output_junit=junit) == 0
    call = rec.calls[0]
    assert (call[call.index("-C"):][:2] if "-C" in call else []) == config_flag
    assert call[call.index("--output-junit") + 1] == str(junit)


def test_the_default_tree_is_the_one_of_the_configured_build_type(tmp_path):
    assert _module(tmp_path, "Debug").build_tree() == tmp_path / "build" / "default-debug"


def test_a_sanitizer_names_its_own_tree_and_the_two_builds_it_offers(tmp_path):
    module = _module(tmp_path, "Debug")
    assert module.sanitized.names == ("address", "thread")
    assert module.sanitized.tree("address") == tmp_path / "build" / "asan-debug"


def test_a_sanitizer_build_is_the_st_build_of_that_sanitizer(tmp_path, monkeypatch):
    seen = []
    monkeypatch.setattr(build_svc, "build", lambda cfg, paths, **options: seen.append(options) or 0)
    assert _module(tmp_path).sanitized.build("thread") == 0
    assert seen == [{"sanitizer": build_svc.Sanitizer.thread}]


def test_a_stale_tree_is_refused_with_the_clean_hint(tmp_path):
    build = tmp_path / "build" / "default-release"
    build.mkdir(parents=True)
    (build / "CMakeCache.txt").write_text("CMAKE_HOME_DIRECTORY:INTERNAL=/workspace\n", encoding="utf-8")
    assert "st build --clean" in _module(tmp_path).check_tree(build)
