# test_deploy.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""The package `st deploy` assembles: its templates and its notices."""

from __future__ import annotations

import pytest

import ast
from types import SimpleNamespace

from sushitrack_cli.services import deploy

pytestmark = pytest.mark.medium


def test_the_c_example_includes_each_header_on_its_own_line():
    """Writes one `#include` directive per line in example.c."""
    lines = deploy._template("example.c.in").splitlines()
    assert "#include <stdio.h>" in lines
    assert "#include <SushiTrack/c_api/sushitrack_c.h>" in lines
    assert not [line for line in lines if line.count("include") > 1]


def test_the_python_example_parses():
    """Writes an example.py that is valid Python."""
    ast.parse(deploy._template("example.py.in"))


def test_the_readmes_name_every_runtime_library():
    """Fills both READMEs with the SushiTrack and SushiLog library names."""
    for name in ("cpp_readme.md.in", "python_readme.md.in"):
        text = deploy._template(name).format(import_lib="i", runtime_lib="r",
                                             sushilog_lib="s", platform="p")
        assert "r" in text and "s" in text and "NOTICE.md" in text


def test_notices_reach_the_package(tmp_path):
    """Copies LICENSE, NOTICE.md and every licence text into the package."""
    root = tmp_path / "repo"
    for path in ["LICENSE", "NOTICE.md", *(source for source, _ in deploy.K_LICENCE_TEXTS)]:
        (root / path).parent.mkdir(parents=True, exist_ok=True)
        (root / path).write_text("text", encoding="utf-8")
    package = tmp_path / "package"
    assert deploy._copy_notices(SimpleNamespace(root=root), package)
    assert (package / "LICENSE").is_file() and (package / "NOTICE.md").is_file()
    for _, target in deploy.K_LICENCE_TEXTS:
        assert (package / "licenses" / target).is_file()


def test_a_missing_notice_stops_the_package(tmp_path, recorder):
    """Refuses to finish a package that would ship without LICENSE."""
    spy = recorder(deploy)
    assert not deploy._copy_notices(SimpleNamespace(root=tmp_path), tmp_path / "package")
    assert spy.said("LICENSE")


def test_deploy_kind_values_match_kinds():
    assert {member.value for member in deploy.DeployKind} == set(deploy.KINDS)


def test_the_runtime_library_names_follow_the_platform():
    assert deploy.shared_lib_name() in ("sushitrack.dll", "libsushitrack.so")
