# docker.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under the Apache License 2.0. See LICENSE.
"""Builds the Docker image and runs the developer container."""

from ..workspace import current_workspace

#: The Dockerfile, from the repository root; its ignore file sits beside it.
DOCKERFILE = ".config/docker/Dockerfile"


def build(cfg, paths, no_cache=False, forwarded=(), workspace=None):
    space = workspace or current_workspace()
    console = space.console
    console.header("Docker build")
    command = ["docker", "build", "-f", str(paths.root / DOCKERFILE), "-t", cfg.docker_image]
    if no_cache:
        console.info("Rebuilding every layer (--no-cache).")
        command.append("--no-cache")
    command.extend(forwarded)
    command.append(".")

    code = space.runner.run(command, paths.root)
    if code != 0:
        console.error("Docker build failed.")
        return code
    console.success(f"Image '{cfg.docker_image}' built.")
    return 0


def run(cfg, paths, no_gpu=False, command=None, forwarded=(), workspace=None):
    """Start the developer container with the repo mounted at docker_workdir."""
    space = workspace or current_workspace()
    console = space.console
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
    return space.runner.run(argv, paths.root)
