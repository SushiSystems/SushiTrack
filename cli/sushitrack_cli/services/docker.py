"""Docker image and container management.

Replaces cli/docker/build.{sh,bat} and cli/docker/run.{sh,bat}.
"""

from .. import console, proc


def build(cfg, paths, no_cache=False, forwarded=()):
    console.header("Docker build")
    command = ["docker", "build", "-t", cfg.docker_image]
    if no_cache:
        console.info("Rebuilding every layer (--no-cache).")
        command.append("--no-cache")
    command.extend(forwarded)
    command.append(".")

    code = proc.run(command, cwd=paths.root)
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
    return proc.run(argv, cwd=paths.root)
