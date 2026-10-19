# cli.py
# SushiTrack - https://github.com/SushiSystems/SushiTrack
# Copyright (c) 2026-present Mustafa Garip & Sushi Systems
# Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.
# Commercial use requires a licence from Sushi Systems.
"""The SushiTrack developer CLI (`st` / `sushitrack`).

Thin Typer layer: each command parses arguments and delegates to
:mod:`sushitrack_cli.services`. Nothing here does real work, so the surface
stays readable as one file and the behaviour stays testable without Typer.
"""

from __future__ import annotations

import sys
import types
from pathlib import Path
from typing import List, Optional, Sequence

import typer
from sushicore import entry as core_entry
from sushicore.diag_commands import register_diagnostic_commands
from sushicore.docs_bundle import register_docs_commands
from sushicore.provision.commands import ModuleProvision, register_provision_commands
from sushicore.root_options import register_root_options
from sushicore.typer_help import help_group

from . import console
from .aliases import ALIASES
from .config import PROFILE, context, find_project_root, load_tool
from .services import build as build_svc
from .services import deploy as deploy_svc
from .services import diag as diag_svc
from .services import docker as docker_svc
from .services import infer as infer_svc
from .services import label as label_svc
from .services import runner as run_svc
from .services import tests as test_svc
from .services.checks import st_checks
from .services.build import BuildType
from .services.deploy import DeployKind
from .services.test_label import TestLabel

K_PROJECT = "Project"
K_BENCHMARK = "Benchmark"
K_DIAGNOSTICS = "Diagnostics"
K_DISTRIBUTION = "sushitrack-cli"
K_PROGRAMS = ("st", "sushitrack")
K_FORWARDING = {"allow_extra_args": True, "ignore_unknown_options": True}

_help_group = help_group(console.current)

app = typer.Typer(
    name="st",
    cls=_help_group,
    help="SushiTrack developer CLI: build, test, run, eval, infer, container and docs.",
    no_args_is_help=True,
    add_completion=False,
    rich_markup_mode="rich",
)
register_root_options(app, distribution=K_DISTRIBUTION)


@app.command(
    "build",
    rich_help_panel=K_PROJECT,
    epilog="st build\n"
           "st build --type debug\n"
           "st build --no-test  # Library only\n"
           "st build --deploy cpp  # Build, then assemble package/",
)
def build(
    type: Optional[BuildType] = typer.Option(
        None, "--type", "-t", case_sensitive=False,
        help="Build type: release | debug | relwithdebinfo | minsizerel "
             "(default from config)."),
    no_test: bool = typer.Option(
        False, "--no-test",
        help="Skip the test targets (library only). Tests are built by default."),
    clean: bool = typer.Option(
        False, "--clean", help="Delete the build tree before configuring."),
    jobs: Optional[int] = typer.Option(
        None, "--jobs", "-j", help="Parallel build jobs (default: CPU count)."),
    deploy: Optional[DeployKind] = typer.Option(
        None, "--deploy", case_sensitive=False,
        help="After building, assemble package/ for the given consumer."),
    define: Optional[list[str]] = typer.Option(
        None, "-D", metavar="VAR=VALUE",
        help="Extra -D option passed straight to cmake (repeatable)."),
):
    """Configure and build the project.

    cmake configure runs only when the requested configuration differs from
    what the build tree was configured with; otherwise the existing tree is
    reused.
    """
    cfg, paths = context()
    build_type = build_svc.normalize_build_type(type.value) if type else None
    code = build_svc.build(
        cfg, paths, build_type=build_type, clean=clean, jobs=jobs,
        tests=not no_test,
        defines=[f"-D{item}" for item in (define or [])])
    if code != 0 or not deploy:
        raise typer.Exit(code)
    raise typer.Exit(deploy_svc.deploy(paths, deploy.value))


@app.command(
    "test",
    rich_help_panel=K_PROJECT,
    epilog="st test  # small and medium\n"
           "st test --label small\n"
           "st test --label small -f 'RectTest'\n"
           "st test --label medium --repeat 3\n"
           "st test --label large  # Needs real hardware\n"
           "st test --label quarantined",
)
def test(
    label: Optional[List[TestLabel]] = typer.Option(
        None, "--label", "-l", case_sensitive=False,
        help="Size label to run, repeatable: small, medium, large or quarantined. "
             "Default: small and medium; quarantined tests run only when named."),
    filter: Optional[str] = typer.Option(
        None, "--filter", "-f", metavar="REGEX",
        help="ctest -R regex over 'Suite.Case' test names."),
    repeat: int = typer.Option(
        0, "--repeat", "-r", min=0, metavar="N",
        help="Re-run each test up to N times, stopping on the first failure "
             "(ctest --repeat until-fail). Useful for hunting flaky tests."),
    jobs: Optional[int] = typer.Option(
        None, "--jobs", "-j", metavar="N",
        help="Run up to N tests concurrently (ctest --parallel)."),
):
    """Run the tests through CTest, selected by size label.

    For GTest-level knobs (shuffle, break-on-failure) run the binary directly:
    st run sushitrack_tests -- --gtest_shuffle --gtest_break_on_failure

    Metric evaluation and video inference are not CTest suites — see `st eval`
    and `st infer`.
    """
    cfg, paths = context()
    raise typer.Exit(test_svc.test(cfg, paths, labels=label, filter=filter,
                                   repeat=repeat, jobs=jobs))


@app.command(
    "run",
    rich_help_panel=K_PROJECT,
    context_settings=K_FORWARDING,
    epilog="st run  # Pick from a list\n"
           "st run sushitrack_tests\n"
           "st run sushitrack_tests -- --gtest_filter='Small_TrackletTest.*'",
)
def run(
    ctx: typer.Context,
    target: Optional[str] = typer.Argument(
        None, help="Executable name (exact, then substring match). "
                    "Omit to pick from a list."),
    select: bool = typer.Option(
        False, "--select", "-s",
        help="Always show the picker, even when a target is given."),
):
    """Run a built executable."""
    cfg, paths = context()
    raise typer.Exit(run_svc.run(cfg, paths, target=target, select=select,
                                 forwarded=list(ctx.args)))


@app.command("clean", rich_help_panel=K_PROJECT, epilog="st clean")
def clean():
    """Remove the build tree and the assembled package directory."""
    _, paths = context()
    raise typer.Exit(build_svc.clean(paths))


@app.command(
    "deploy", rich_help_panel=K_PROJECT,
    epilog="st deploy cpp\nst deploy python",
)
def deploy_cmd(
    kind: DeployKind = typer.Argument(..., help="Consumer flavour to assemble."),
):
    """Assemble package/ from the current build for C/C++ or Python consumers."""
    _, paths = context()
    raise typer.Exit(deploy_svc.deploy(paths, kind.value))


container_app = typer.Typer(
    cls=_help_group,
    help="Build the developer image and start a container from it.",
    no_args_is_help=True,
)
app.add_typer(container_app, name="container", rich_help_panel=K_PROJECT)

docker_alias_app = typer.Typer(
    cls=_help_group,
    help="The old spelling of `st container`.",
    no_args_is_help=True,
)
app.add_typer(docker_alias_app, name="docker", hidden=True)


@container_app.command(
    "build",
    context_settings=K_FORWARDING,
    epilog="st container build\nst container build --no-cache",
)
def container_build(
    ctx: typer.Context,
    no_cache: bool = typer.Option(
        False, "--no-cache", help="Rebuild every layer, ignoring Docker's cache."),
):
    """Build the SushiTrack Docker image."""
    cfg, paths = context()
    raise typer.Exit(docker_svc.build(cfg, paths, no_cache=no_cache,
                                      forwarded=list(ctx.args)))


@container_app.command(
    "run",
    context_settings=K_FORWARDING,
    epilog="st container run\nst container run -- bash -lc 'echo hi'",
)
def container_run(
    ctx: typer.Context,
    no_gpu: bool = typer.Option(
        False, "--no-gpu",
        help="Omit --gpus all (no GPU available, or a CPU-only run)."),
):
    """Start the developer container with this repo mounted.

    Args after `--` are the command to run inside the container, in place of
    an interactive bash: st container run -- bash -lc 'echo hi'
    """
    cfg, paths = context()
    raise typer.Exit(docker_svc.run(cfg, paths, no_gpu=no_gpu,
                                    command=list(ctx.args) or None))


ALIASES.command(docker_alias_app, "build", container_build, "st docker build",
                hidden=False, context_settings=K_FORWARDING)
ALIASES.command(docker_alias_app, "run", container_run, "st docker run",
                hidden=False, context_settings=K_FORWARDING)

register_docs_commands(
    app,
    program=PROFILE.program,
    panel=K_PROJECT,
    project_root=find_project_root,
    report=lambda line: console.success(line),
    group_cls=_help_group,
)


@app.command(
    "eval",
    rich_help_panel=K_BENCHMARK,
    epilog="st eval  # every benchmark\n"
           "st eval --benchmark mot17 --compare\n"
           "st eval --scenario MOT17-04-DPM --compare\n"
           "st eval --eval-only --export ./results\n"
           "st eval --trackeval --private",
)
def evaluate(
    benchmark: Optional[str] = typer.Option(
        None, "--benchmark", "-b", metavar="NAME",
        help="Benchmark under applications/benchmarks/ to run: mot17, mot20 or dancetrack "
             "(default: every benchmark)."),
    scenario: Optional[str] = typer.Option(
        None, "--scenario", metavar="NAME",
        help="Sequence name or substring filter (default: every sequence)."),
    tracker: Optional[str] = typer.Option(
        None, "--tracker", metavar="NAME",
        help="Tracker to run: sushitrack | bytetrack | ocsort."),
    compare: bool = typer.Option(
        False, "--compare", help="Run every tracker side by side."),
    eval_only: bool = typer.Option(
        False, "--eval-only",
        help="Skip inference; re-score the existing pred/*.txt files."),
    trackeval: bool = typer.Option(
        False, "--trackeval",
        help="Score with the official TrackEval script instead of the "
             "internal runner."),
    export: Optional[str] = typer.Option(
        None, "--export", metavar="DIR",
        help="Write CSV/JSON metric exports to DIR."),
    private: bool = typer.Option(
        False, "--private",
        help="Only MOT sequences with private (YOLOX) detections."),
    public: bool = typer.Option(
        False, "--public",
        help="Exclude MOT sequences with private detections."),
):
    """Run the MOT benchmarks and print TrackEval metrics.

    Drives the mot_engine program over each benchmark's det.txt-backed sequences, then
    scores the predictions (HOTA / DetA / AssA / MOTA / IDF1 / IDSW / latency).
    """
    cfg, paths = context()
    key = label_svc.resolve_tracker(tracker or cfg.test_tracker)
    raise typer.Exit(label_svc.evaluate_sequences(
        cfg, paths, benchmark=benchmark, scenario=scenario, tracker=key, compare=compare,
        eval_only=eval_only, trackeval=trackeval, export=export,
        private=private, public=public))


@app.command(
    "infer",
    rich_help_panel=K_BENCHMARK,
    context_settings=K_FORWARDING,
    epilog="st infer -n yolox-x -c third_party/weight/mot17x.pth.tar "
           "--source third_party/video/MOT17-04.mp4 --num-classes 1 --legacy\n"
           "st infer --source 0 --tracker sushitrack --no-save --no-screen --stress",
)
def infer(
    ctx: typer.Context,
    source: Optional[str] = typer.Option(
        None, "--source", metavar="PATH", help="Video path or camera index (required)."),
    tracker: Optional[str] = typer.Option(
        None, "--tracker", metavar="NAME",
        help="Tracker to drive: sushitrack | bytetrack | ocsort."),
    name: str = typer.Option("yolox-s", "--name", "-n", help="YOLOX model name."),
    ckpt: Optional[str] = typer.Option(None, "--ckpt", "-c", help="YOLOX checkpoint path."),
    device: str = typer.Option("gpu", "--device", help="cpu or gpu."),
    conf: float = typer.Option(0.1, "--conf", help="Detection confidence threshold."),
    nms: float = typer.Option(0.45, "--nms", help="NMS threshold."),
    tsize: Optional[tuple[int, int]] = typer.Option(
        None, "--tsize", metavar="H W",
        help="Inference resolution H W (square: pass the same value twice)."),
    num_classes: int = typer.Option(80, "--num-classes", help="YOLOX class count."),
    legacy: bool = typer.Option(False, "--legacy", help="Legacy YOLOX normalisation."),
    stress: bool = typer.Option(False, "--stress", help="Headless maximum-throughput mode."),
    no_save: bool = typer.Option(False, "--no-save", help="Do not write an output video."),
    save_name: Optional[str] = typer.Option(
        None, "--save_name", help="Output video filename."),
    mot_label: bool = typer.Option(
        False, "--mot-label", help="Write detections to output/det.txt."),
    no_screen: bool = typer.Option(False, "--no-screen", help="Disable the UI display."),
    reid: bool = typer.Option(False, "--reid", help="Enable OSNet ReID embedding extraction."),
    reid_model: str = typer.Option(
        "osnet_x1_0.onnx", "--reid-model", help="Path to the ReID ONNX model."),
):
    """Run the YOLOX video inference pipeline through tracker_bridge.

    Example: st infer --source clip.mp4 -c weights/mot17x.pth.tar --tsize 800 1440
    """
    cfg, paths = context()
    args = types.SimpleNamespace(
        source=source, tracker=tracker, name=name, ckpt=ckpt, device=device,
        conf=conf, nms=nms, tsize=tsize, num_classes=num_classes, legacy=legacy,
        stress=stress, no_save=no_save, save_name=save_name, mot_label=mot_label,
        no_screen=no_screen, reid=reid, reid_model=reid_model,
        forwarded=list(ctx.args))
    raise typer.Exit(infer_svc.run(cfg, paths, args))


@app.command("status", rich_help_panel=K_DIAGNOSTICS, epilog="st status")
def status():
    """Show what is built, what is missing and which data sets are present."""
    raise typer.Exit(diag_svc.status(*context()))


@app.command("paths", rich_help_panel=K_DIAGNOSTICS, epilog="st paths")
def paths():
    """Print the resolved project directories."""
    raise typer.Exit(diag_svc.paths_show(*context()))


register_diagnostic_commands(
    app, diag_svc.Diagnostics(context), program=PROFILE.program, panel=K_DIAGNOSTICS)

register_provision_commands(app, ModuleProvision(
    profile=PROFILE,
    project_root=find_project_root,
    load_config=load_tool,
    console=console.current,
    extra_checks=lambda: st_checks(context()[1]),
    panel=K_DIAGNOSTICS,
))


def _program_name() -> str:
    """Returns the name the process was started under when it is one of st's own, else `st`."""
    stem = Path(sys.argv[0]).stem.lower() if sys.argv else ""
    return stem if stem in K_PROGRAMS else PROFILE.program


def main(argv: Sequence[str] | None = None) -> None:
    """Runs st and exits; a failure the user can act on is one line and exit code 1.

    Args:
        argv: The arguments to parse; the process's own when None. An empty list prints the help.
    """
    app.info.name = _program_name()
    arguments = list(sys.argv[1:] if argv is None else argv) or ["--help"]
    core_entry.run(app, lambda message: console.error(message), argv=arguments)


def entry() -> None:
    """Runs :func:`main`; the `st` and `sushitrack` console scripts name this function."""
    main()


if __name__ == "__main__":
    main()
