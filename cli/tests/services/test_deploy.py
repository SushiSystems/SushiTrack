# test_deploy.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""The package `st deploy` assembles from a stand-in build tree: its examples and its notices."""

from __future__ import annotations

import ast
from types import SimpleNamespace

import pytest

from sushitrack_cli.services import artifacts, deploy

pytestmark = pytest.mark.medium

K_KINDS = ["cpp", "python"]
K_NOTICES = ["LICENSE", "NOTICE.md",
             *(f"licenses/{target}" for _, target in deploy.K_LICENCE_TEXTS)]


def _write(path, text="text"):
    """Create *path* with *text*, making its folder."""
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


@pytest.fixture
def paths(tmp_path):
    """Return the paths of a repository with its notices, the Python binding and built libraries."""
    root = tmp_path / "repo"
    for name in ["LICENSE", "NOTICE.md", *(source for source, _ in deploy.K_LICENCE_TEXTS)]:
        _write(root / name)
    _write(root / "bindings" / "python" / "sushitrack.py")
    build = root / "build"
    for name in (artifacts.shared_lib_name(), artifacts.sushilog_lib_name(),
                 artifacts.import_lib_name()):
        _write(build / "bin" / name)
    return SimpleNamespace(root=root, build=build, package=tmp_path / "package")


def test_the_c_example_includes_each_header_on_its_own_line(paths, session):
    """Writes one `#include` directive per line in example.c."""
    assert deploy.deploy(paths, "cpp", workspace=session.workspace) == 0
    lines = (paths.package / "example" / "example.c").read_text(encoding="utf-8").splitlines()
    assert "#include <stdio.h>" in lines
    assert "#include <SushiTrack/c_api/sushitrack_c.h>" in lines
    assert not [line for line in lines if line.count("include") > 1]


def test_the_python_example_imports_the_tracker_from_the_binding(paths, session):
    """Writes an example.py that parses and imports Tracker from sushitrack."""
    assert deploy.deploy(paths, "python", workspace=session.workspace) == 0
    tree = ast.parse((paths.package / "example.py").read_text(encoding="utf-8"))
    imported = {(node.module, alias.name) for node in ast.walk(tree)
                if isinstance(node, ast.ImportFrom) for alias in node.names}
    assert ("sushitrack", "Tracker") in imported


@pytest.mark.parametrize("kind", K_KINDS)
def test_the_readmes_name_every_runtime_library(paths, session, kind):
    """Names the platform's SushiTrack and SushiLog libraries and NOTICE.md in each README."""
    assert deploy.deploy(paths, kind, workspace=session.workspace) == 0
    text = (paths.package / "README.md").read_text(encoding="utf-8")
    assert artifacts.shared_lib_name() in text
    assert artifacts.sushilog_lib_name() in text
    assert "NOTICE.md" in text


@pytest.mark.parametrize("kind", K_KINDS)
@pytest.mark.parametrize("notice", K_NOTICES)
def test_notices_reach_the_package(paths, session, kind, notice):
    """Copies LICENSE, NOTICE.md and each licence text into the package."""
    assert deploy.deploy(paths, kind, workspace=session.workspace) == 0
    assert (paths.package / notice).is_file()


@pytest.mark.parametrize("kind", K_KINDS)
def test_a_missing_notice_stops_the_package(paths, session, kind):
    """Refuses to finish a package that would ship without LICENSE."""
    (paths.root / "LICENSE").unlink()
    assert deploy.deploy(paths, kind, workspace=session.workspace) == 1
    assert session.said("LICENSE")
