"""The SushiTrack developer CLI (`st` / `sushitrack`).

Thin command layer: each command parses arguments and delegates to
:mod:`sushitrack_cli.services`. Nothing here does real work, so the surface stays
readable as one file and the behaviour stays testable without argparse.
"""

import sys
from pathlib import Path

from . import __version__
from .app import App, Opt, main
from .config import context
from .services import build as build_svc
from .services import deploy as deploy_svc
from .services import diag as diag_svc
from .services import docker as docker_svc
from .services import infer as infer_svc
from .services import label as label_svc
from .services import runner as run_svc
from .services import tests as test_svc

app = App(
    prog="st",
    description="SushiTrack developer CLI — build, test, evaluate, run and Docker.",
    version=__version__,
    epilog="Built on sushicore, the console/config layer shared with sr/se/sa/sb/sd/hub.",
)

docker = app.group("docker", "Docker image and container management.")

app.section("Project", "build", "test", "run", "clean", "deploy")
app.section("Benchmark", "eval", "infer")
app.section("Diagnostics", "status", "doctor", "config", "paths", "env")


@app.command("build", options=[
    Opt("--type", "-t", dest="build_type", default=None, metavar="TYPE",
        help="Build type: release | debug | relwithdebinfo | minsizerel "
             "(default from config)."),
    Opt("--no-test", dest="no_test", action="store_true",
        help="Skip the test targets (library only). Tests are built by default."),
    Opt("--clean", action="store_true",
        help="Delete the build tree before configuring."),
    Opt("--jobs", "-j", type=int, default=None, metavar="N",
        help="Parallel build jobs (default: CPU count)."),
    Opt("--deploy", choices=list(deploy_svc.KINDS), default=None,
        help="After building, assemble package/ for the given consumer."),
    Opt("-D", dest="cmake_defines", action="append", default=[],
        metavar="VAR=VALUE",
        help="Extra -D option passed straight to cmake (repeatable)."),
])
def build(args):
    """Configure and build the project.

    cmake configure runs only when the requested configuration differs from what
    the build tree was configured with; otherwise the existing tree is reused.
    """
    cfg, paths = context()
    build_type = build_svc.normalize_build_type(args.build_type) \
        if args.build_type else None
    code = build_svc.build(
        cfg, paths, build_type=build_type, clean=args.clean, jobs=args.jobs,
        tests=not args.no_test,
        defines=[f"-D{define}" for define in args.cmake_defines])
    if code != 0 or not args.deploy:
        return code
    return deploy_svc.deploy(paths, args.deploy)


@app.command("test", options=[
    Opt("--suite", "-s", choices=list(test_svc.SUITES),
        default=test_svc.DEFAULT_SUITE,
        help="CTest label group to run ('all' runs every test). "
             f"Default: {test_svc.DEFAULT_SUITE}."),
    Opt("--filter", "-f", default=None, metavar="REGEX",
        help="ctest -R regex over 'Suite.Case' test names."),
    Opt("--repeat", "-r", type=int, default=0, metavar="N",
        help="Re-run each test up to N times, stopping on the first failure "
             "(ctest --repeat until-fail). Useful for hunting flaky tests."),
    Opt("--jobs", "-j", type=int, default=None, metavar="N",
        help="Run up to N tests concurrently (ctest --parallel)."),
])
def test(args):
    """Run the test suites via CTest labels.

    For GTest-level knobs (shuffle, break-on-failure) run the binary directly:
    st run unit_test -- --gtest_shuffle --gtest_break_on_failure

    Metric evaluation and video inference are not CTest suites — see `st eval`
    and `st infer`.
    """
    cfg, paths = context()
    return test_svc.test(cfg, paths, suite=args.suite, filter=args.filter,
                         repeat=args.repeat, jobs=args.jobs)


@app.command("eval", options=[
    Opt("--scenario", default=None, metavar="NAME",
        help="Sequence name or substring filter (default: every sequence)."),
    Opt("--tracker", default=None, metavar="NAME",
        help="Tracker to run: sushitrack | bytetrack | ocsort."),
    Opt("--compare", action="store_true",
        help="Run every tracker side by side."),
    Opt("--eval-only", action="store_true",
        help="Skip inference; re-score the existing pred/*.txt files."),
    Opt("--trackeval", action="store_true",
        help="Score with the official TrackEval script instead of the "
             "internal runner."),
    Opt("--export", default=None, metavar="DIR",
        help="Write CSV/JSON metric exports to DIR."),
    Opt("--private", action="store_true",
        help="Only MOT sequences with private (YOLOX) detections."),
    Opt("--public", action="store_true",
        help="Exclude MOT sequences with private detections."),
])
def evaluate(args):
    """Run the MOT label benchmark and print TrackEval metrics.

    Drives the regression binary over det.txt-backed sequences, then scores the
    predictions (HOTA / DetA / AssA / MOTA / IDF1 / IDSW / latency).
    """
    cfg, paths = context()
    tracker = label_svc.resolve_tracker(args.tracker or cfg.test_tracker)
    return label_svc.evaluate_sequences(
        cfg, paths, scenario=args.scenario, tracker=tracker,
        compare=args.compare, eval_only=args.eval_only,
        trackeval=args.trackeval, export=args.export,
        private=args.private, public=args.public)


@app.command("infer", forwards=True, options=[
    Opt("--source", default=None, metavar="PATH",
        help="Video path or camera index (required)."),
    Opt("--tracker", default=None, metavar="NAME",
        help="Tracker to drive: sushitrack | bytetrack | ocsort."),
    Opt("--name", "-n", default="yolox-s", help="YOLOX model name."),
    Opt("--ckpt", "-c", default=None, help="YOLOX checkpoint path."),
    Opt("--device", default="gpu", help="cpu or gpu."),
    Opt("--conf", type=float, default=0.1,
        help="Detection confidence threshold."),
    Opt("--nms", type=float, default=0.45, help="NMS threshold."),
    Opt("--tsize", type=int, nargs="+", default=None, metavar="N",
        help="Inference resolution [H W] or [S]."),
    Opt("--num-classes", type=int, default=80, help="YOLOX class count."),
    Opt("--legacy", action="store_true",
        help="Legacy YOLOX normalisation."),
    Opt("--stress", action="store_true",
        help="Headless maximum-throughput mode."),
    Opt("--no-save", action="store_true",
        help="Do not write an output video."),
    Opt("--save_name", default=None, help="Output video filename."),
    Opt("--mot-label", action="store_true",
        help="Write detections to output/det.txt."),
    Opt("--no-screen", action="store_true", help="Disable the UI display."),
    Opt("--reid", action="store_true",
        help="Enable OSNet ReID embedding extraction."),
    Opt("--reid-model", default="osnet_x1_0.onnx",
        help="Path to the ReID ONNX model."),
])
def infer(args):
    """Run the YOLOX video inference pipeline through tracker_bridge.

    Example: st infer --source clip.mp4 -c weights/mot17x.pth.tar --tsize 800 1440
    """
    cfg, paths = context()
    return infer_svc.run(cfg, paths, args)


@app.command("run", forwards=True, options=[
    Opt("target", nargs="?", default=None,
        help="Executable name (exact, then substring match). "
             "Omit to pick from a list."),
    Opt("--select", "-s", action="store_true",
        help="Always show the picker, even when a target is given."),
])
def run(args):
    """Run a built executable.

    Example: st run unit_test -- --gtest_filter='TrackletTest.*'
    """
    cfg, paths = context()
    return run_svc.run(cfg, paths, target=args.target, select=args.select,
                       forwarded=args.forwarded)


@app.command("clean")
def clean(args):
    """Remove the build tree and the assembled package directory."""
    _, paths = context()
    return build_svc.clean(paths)


@app.command("deploy", options=[
    Opt("kind", choices=list(deploy_svc.KINDS),
        help="Consumer flavour to assemble."),
])
def deploy(args):
    """Assemble package/ from the current build for C/C++ or Python consumers."""
    _, paths = context()
    return deploy_svc.deploy(paths, args.kind)


@app.command("status")
def status(args):
    """Show what is built, what is missing and which data sets are present."""
    return diag_svc.status(*context())


@app.command("doctor")
def doctor(args):
    """Check the toolchain and data this project needs, and report what is missing."""
    return diag_svc.doctor(*context())


@app.command("config")
def config(args):
    """Print the resolved configuration and where each value came from."""
    return diag_svc.config_show(*context())


@app.command("paths")
def paths(args):
    """Print the resolved project directories (replaces cli/paths.env)."""
    return diag_svc.paths_show(*context())


@app.command("env", options=[
    Opt("--all", "-a", dest="show_all", action="store_true",
        help="Show every variable, not just the build-relevant ones."),
])
def env(args):
    """Print the environment cmake, ctest and the test binaries execute under."""
    cfg, project_paths = context()
    return diag_svc.env_dump(cfg, project_paths, show_all=args.show_all)


@docker.command("build", forwards=True, options=[
    Opt("--no-cache", action="store_true",
        help="Rebuild every layer, ignoring Docker's cache."),
])
def docker_build(args):
    """Build the SushiTrack Docker image."""
    cfg, paths = context()
    return docker_svc.build(cfg, paths, no_cache=args.no_cache,
                            forwarded=args.forwarded)


@docker.command("run", forwards=True, options=[
    Opt("--no-gpu", action="store_true",
        help="Omit --gpus all (no GPU available, or a CPU-only run)."),
    Opt("--command", "-c", nargs="+", default=None, metavar="ARG",
        help="Command to run in the container instead of an interactive bash."),
])
def docker_run(args):
    """Start the developer container with this repo mounted."""
    cfg, paths = context()
    return docker_svc.run(cfg, paths, no_gpu=args.no_gpu, command=args.command,
                          forwarded=args.forwarded)


def entry(argv=None, prog=None):
    """Process entry point: returns an exit code, never raises for user error."""
    if prog:
        app.prog = prog
    elif sys.argv:
        stem = Path(sys.argv[0]).stem.lower()
        if stem in ("sushitrack", "st"):
            app.prog = stem
    return main(app, argv)
