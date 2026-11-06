# test_tests.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""What `st test` hands the shared test command."""

from __future__ import annotations

from types import SimpleNamespace

import pytest

from sushitrack_cli.services import tests as test_svc

pytestmark = pytest.mark.medium


def _tree(tmp_path, cache, name="default-release"):
    """Create a configured tree called *name* whose cache holds *cache*."""
    build = tmp_path / "build" / name
    build.mkdir(parents=True)
    (build / "CMakeCache.txt").write_text(
        f"CMAKE_HOME_DIRECTORY:INTERNAL={tmp_path}\n{cache}", encoding="utf-8")
    return build


_TOOL = SimpleNamespace(ctest_exe="", expand=lambda v: v)


def _module(tmp_path, session, build_type="Release", tool=_TOOL):
    """Return what `st test` hands the shared command over a project at *tmp_path*."""
    cfg = SimpleNamespace(build_type=build_type, tool=tool, jobs=lambda: 2)
    paths = SimpleNamespace(root=tmp_path, config=tmp_path / "sushitrack.json", cli=tmp_path,
                            build=tmp_path / "build" / "default-release",
                            build_root=tmp_path / "build")
    return test_svc.module_tests(lambda: (cfg, paths), workspace=session.workspace)


@pytest.mark.parametrize("cache, config_flag", [
    ("CMAKE_BUILD_TYPE:STRING=Release\n", []),
    ("", ["-C", "Release"]),
])
def test_ctest_gets_a_config_only_for_a_multi_config_tree(tmp_path, session, cache, config_flag):
    build = _tree(tmp_path, cache)
    junit = tmp_path / "junit.xml"
    assert _module(tmp_path, session).run_ctest(build, label_regex="^(small)$", output_junit=junit) == 0
    call = session.commands[0]
    assert (call[call.index("-C"):][:2] if "-C" in call else []) == config_flag
    assert call[call.index("--output-junit") + 1] == str(junit)


def test_the_default_tree_is_the_one_of_the_configured_build_type(tmp_path, session):
    assert _module(tmp_path, session, "Debug").build_tree() == tmp_path / "build" / "default-debug"


def test_a_sanitizer_names_its_own_tree_and_the_two_builds_it_offers(tmp_path, session):
    module = _module(tmp_path, session, "Debug")
    assert module.sanitized.names == ("address", "thread")
    assert module.sanitized.tree("address") == tmp_path / "build" / "asan-debug"


def test_a_sanitizer_build_is_the_st_build_of_that_sanitizer(tmp_path, session):
    tool = SimpleNamespace(
        cmake_exe="", ctest_exe="", generator="Ninja", ccache_exe="", is_windows=False,
        platform="linux", vs_vcvars="", expand=lambda v: v, resolved_compiler=lambda root: "",
        sushi_definitions=lambda root, packaged=None: [],
        sushi_packages=lambda *args, **kwargs: SimpleNamespace(
            names=lambda: [], definitions=lambda: [], run_dirs=lambda platform: []))

    assert _module(tmp_path, session, "Debug", tool).sanitized.build("thread") == 0

    configure = session.commands[0]
    assert "-DSUSHITRACK_SANITIZE=thread" in configure
    assert configure[configure.index("-B") + 1] == str(tmp_path / "build" / "tsan-debug")


def test_a_stale_tree_is_refused_with_the_clean_hint(tmp_path, session):
    build = tmp_path / "build" / "default-release"
    build.mkdir(parents=True)
    (build / "CMakeCache.txt").write_text("CMAKE_HOME_DIRECTORY:INTERNAL=/workspace\n", encoding="utf-8")
    assert "st build --clean" in _module(tmp_path, session).check_tree(build)
