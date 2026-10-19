# docker.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""Builds the Docker image and runs the developer container."""

from .. import console
from ..process import RUNNER

#: The Dockerfile, from the repository root; its ignore file sits beside it.
DOCKERFILE = ".config/docker/Dockerfile"


def build(cfg, paths, no_cache=False, forwarded=()):
    console.header("Docker build")
    command = ["docker", "build", "-f", str(paths.root / DOCKERFILE), "-t", cfg.docker_image]
    if no_cache:
        console.info("Rebuilding every layer (--no-cache).")
        command.append("--no-cache")
    command.extend(forwarded)
    command.append(".")

    code = RUNNER.run(command, paths.root)
    if code != 0:
        console.error("Docker build failed.")
        return code
    console.success(f"Image '{cfg.docker_image}' built.")
    return 0


def run(cfg, paths, no_gpu=False, command=None, forwarded=()):
    """Start the developer container with the repo mounted at docker_workdir."""
    console.header("Docker run")
    argv = ["docker", "run", "-it", "--rm"]
    if no_gpu or not cfg.docker_gpus:
        console.info("Running without GPU passthrough.")
    else:
        argv += ["--gpus", "all"]
    argv += ["-v", f"{paths.root}:{cfg.docker_workdir}",
             "-w", cfg.docker_workdir]
    argv.extend(forwarded)
    argv.append(cfg.docker_image)
    argv.extend(command or ["bash"])
    return RUNNER.run(argv, paths.root)
