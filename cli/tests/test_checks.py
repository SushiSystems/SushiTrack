# test_checks.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Tests for SushiTrack's own doctor checks."""

from __future__ import annotations

from types import SimpleNamespace

import pytest
from sushicore.provision import checks as sushicore_checks
from sushicore.provision.doctor import Doctor, State

from sushitrack_cli.services import checks
from sushitrack_cli.services.checks import st_checks


@pytest.fixture
def st_paths(tmp_path):
    dense = tmp_path / "third_party" / "eigen" / "Eigen" / "Dense"
    dense.parent.mkdir(parents=True)
    dense.write_text("", encoding="utf-8")
    benchmarks = tmp_path / "benchmarks"
    return SimpleNamespace(root=tmp_path, third_party=tmp_path / "third_party",
                           benchmarks=benchmarks, benchmark_names=lambda: ["mot17"],
                           data_root=lambda name: benchmarks / name / "data")


def _by_name(checks):
    return {c.name: c for c in checks}


def test_groups_cover_build_infer_and_eval(st_paths):
    assert {c.group for c in st_checks(st_paths)} == {"build", "infer", "eval"}


def test_infer_group_names_the_spec_modules(st_paths):
    names = {c.name for c in st_checks(st_paths) if c.group == "infer"}
    assert names == {"yolox", "torch", "torchvision", "cv2", "pycocotools"}


def test_eigen_is_required_and_found(st_paths):
    eigen = _by_name(st_checks(st_paths))["eigen"]
    assert eigen.required and eigen.run().state is State.OK


def test_missing_python_module_fix_names_the_pip_package(st_paths, monkeypatch):
    monkeypatch.setattr("importlib.util.find_spec", lambda name: None)
    result = _by_name(st_checks(st_paths))["cv2"].run()
    assert result.state is State.FAIL
    assert result.fix == "pip install opencv-python"


def test_missing_datasets_fail_with_a_pointer_to_the_readme(st_paths):
    result = _by_name(st_checks(st_paths))["datasets"].run()
    assert result.state is State.FAIL
    assert "docs/README.md" in result.fix


def test_datasets_found_when_a_sequence_has_ground_truth(st_paths, tmp_path):
    gt = tmp_path / "benchmarks" / "mot17" / "data" / "MOT17-02" / "gt" / "gt.txt"
    gt.parent.mkdir(parents=True)
    gt.write_text("", encoding="utf-8")
    assert _by_name(st_checks(st_paths))["datasets"].run().state is State.OK


@pytest.mark.parametrize("system, fix", [
    ("Windows", "winget install Docker.DockerDesktop"),
    ("Linux", "https://docs.docker.com/engine/install/"),
])
def test_missing_docker_fix_follows_the_platform(st_paths, monkeypatch, system, fix):
    monkeypatch.setattr(checks.platform, "system", lambda: system)
    monkeypatch.setattr(sushicore_checks.probe, "binary_works", lambda exe: False)
    assert _by_name(st_checks(st_paths))["docker"].run().fix == fix


def test_doctor_for_infer_runs_only_infer(st_paths, monkeypatch):
    monkeypatch.setattr("importlib.util.find_spec", lambda name: None)
    report = Doctor(st_checks(st_paths)).run({"infer"})
    assert {check.group for check, _result in report.rows} == {"infer"}
