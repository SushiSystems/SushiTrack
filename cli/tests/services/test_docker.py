# test_docker.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""`st container` spawns docker through the workspace's runner."""

from __future__ import annotations

from pathlib import Path
from types import SimpleNamespace

import pytest

from sushitrack_cli.services import docker

pytestmark = pytest.mark.small

_CFG = SimpleNamespace(docker_image="st-dev", docker_workdir="/work", docker_gpus=True)
_PATHS = SimpleNamespace(root=Path("/repo"))


def test_build_names_the_dockerfile_the_image_and_the_forwarded_options(session):
    code = docker.build(_CFG, _PATHS, no_cache=True, forwarded=["--pull"],
                        workspace=session.workspace)

    assert code == 0
    assert session.commands == [["docker", "build", "-f", str(Path("/repo") / docker.DOCKERFILE),
                                 "-t", "st-dev", "--no-cache", "--pull", "."]]


def test_run_mounts_the_checkout_and_asks_for_the_gpus(session):
    docker.run(_CFG, _PATHS, workspace=session.workspace)

    assert session.commands == [["docker", "run", "-it", "--rm", "--gpus", "all",
                                 "-v", f"{Path('/repo')}:/work",
                                 "-w", "/work", "st-dev", "bash"]]
