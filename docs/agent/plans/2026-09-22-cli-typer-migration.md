# `st` CLI Typer Migration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace `cli/sushitrack_cli/app.py`'s hand-rolled argparse command layer with Typer, matching every sibling Sushi* CLI, and close the test-coverage and documentation gaps a review against them found.

**Architecture:** `cli/sushitrack_cli/cli.py` becomes the single file that builds a `typer.Typer` app (as `sushiai.cli` does) and declares every command directly on it, plus one `docker` sub-app for `st docker build`/`st docker run`. `app.py` is deleted — Typer needs no separate declarative wrapper. Three selection enums (`BuildType`, `Suite`, `DeployKind`) are added beside the service constants they mirror, so the command layer imports typed choices instead of raw string tuples. No `services/` function signature changes except where a command's own argument-passing shape changes (the `infer` command builds a `types.SimpleNamespace` carrying the same attribute names `services/infer.py` already reads).

**Tech Stack:** Python 3.10+, Typer >=0.12 (installed: 0.20.0), `sushicore` >=0.4.0 (installed: 0.4.0, confirmed via `pip index versions sushicore`), pytest, `typer.testing.CliRunner`.

**Spec:** `docs/design/CLI_TYPER_MIGRATION.md`

## Global Constraints

- No command name, flag name, default value, or exit-code behaviour changes, **except** the two named exceptions below (confirmed with the project owner during planning, not decided unilaterally):
  - `st infer --tsize` becomes a fixed two-value option (`--tsize H W`; a square size is now spelled `--tsize 640 640` instead of `--tsize 640`).
  - `st docker run --command`/`-c` is removed; the container command is now given after `--` (`st docker run -- bash -lc "echo hi"`), the same mechanism that previously carried extra pre-image `docker run` flags (that use is dropped — nothing in `docs/README.md` or the tests exercised it).
- `cli/sushitrack_cli/config.py` is not touched. `cli/sushitrack_cli/console.py` gets exactly one addition, `current()` (Task 3, Step 0) — the one function its sibling CLIs' `console.py` already carries and the new `cli.py` needs; its `LazyConsole`/`__getattr__` wiring is otherwise untouched.
- `cli/sushitrack_cli/services/*.py` function signatures are not touched, except the two additive enum declarations in Task 2.
- Every command keeps its `rich_help_panel` grouping in this order, matching the current `app.section(...)` calls: `Project` (`build`, `test`, `run`, `clean`, `deploy`, `docker`), `Benchmark` (`eval`, `infer`), `Diagnostics` (`status`, `doctor`, `config`, `paths`, `env`). `sushicore.help.from_click`'s `_command_sections` groups panels "in order of first appearance", so commands must be declared on `app` in that order.
- Chat and commit messages: English in code/docs, per `sushiskills`. Every commit follows the `commits` skill's subject grammar.

---

### Task 1: Bump the CLI's Typer/sushicore dependency floor

**Files:**
- Modify: `cli/pyproject.toml`

**Interfaces:**
- Produces: `typer` importable at `>=0.12`, `sushicore.typer_help.help_group` importable (ships in `sushicore>=0.4.0`). Both are consumed starting in Task 3.

- [ ] **Step 1: Edit the dependency list**

In `cli/pyproject.toml`, replace the `dependencies` block's comment and `sushicore` floor, and add `typer`:

```toml
dependencies = [
    # The command layer (sushitrack_cli/cli.py) is Typer, the same framework
    # every sibling Sushi* CLI (sa/sb/sd/hub) uses. Rich IS declared directly,
    # even though sushicore.console and typer both bring it in transitively: a
    # package this CLI itself relies on stays a direct dependency so its
    # version floor is stated here rather than left to whatever sushicore or
    # typer happens to pull in.
    "typer>=0.12",
    "rich>=13.0",
    "sushicore>=0.4.0",
    "tomli>=2.0; python_version < '3.11'",
]
```

- [ ] **Step 2: Reinstall the package and verify the new imports resolve**

Run: `pip install -e cli`

Then run: `python -c "import typer; from sushicore.typer_help import help_group; print('ok')"`

Expected: `ok`, no `ImportError`.

- [ ] **Step 3: Commit**

```bash
git add cli/pyproject.toml
git commit -m "build(cli): add typer and raise the sushicore floor to 0.4.0"
```

---

### Task 2: Selection enums beside their service constants

**Files:**
- Modify: `cli/sushitrack_cli/services/build.py`
- Modify: `cli/sushitrack_cli/services/tests.py`
- Modify: `cli/sushitrack_cli/services/deploy.py`
- Test: `cli/tests/test_service_enums.py` (create)

**Interfaces:**
- Produces: `services.build.BuildType` (str Enum; members `release`, `debug`, `relwithdebinfo`, `minsizerel`), `services.tests.Suite` (str Enum; members `unit`, `integration`, `regression`, `functional`, `all`), `services.deploy.DeployKind` (str Enum; members `cpp`, `python`). Consumed by `cli.py` in Task 3.
- Consumes: `services.build.BUILD_TYPES`, `services.tests.SUITES`, `services.deploy.KINDS` (all already defined; unchanged).

- [ ] **Step 1: Write the failing test**

Create `cli/tests/test_service_enums.py`:

```python
"""The Typer command layer's selection enums mirror their service's own
string constants exactly — one canonical value set, not two."""

from __future__ import annotations

from sushitrack_cli.services import build as build_svc
from sushitrack_cli.services import deploy as deploy_svc
from sushitrack_cli.services import tests as test_svc


def test_build_type_values_match_build_types():
    assert {member.value for member in build_svc.BuildType} == \
        {name.lower() for name in build_svc.BUILD_TYPES}


def test_suite_values_match_suites():
    assert {member.value for member in test_svc.Suite} == set(test_svc.SUITES)


def test_deploy_kind_values_match_kinds():
    assert {member.value for member in deploy_svc.DeployKind} == set(deploy_svc.KINDS)
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `python -m pytest cli/tests/test_service_enums.py -v`
Expected: FAIL — `AttributeError: module 'sushitrack_cli.services.build' has no attribute 'BuildType'` (and similarly for the other two once that one is fixed).

- [ ] **Step 3: Add the enums**

In `cli/sushitrack_cli/services/build.py`, add after the `BUILD_TYPES` tuple (line 11):

```python
import enum


class BuildType(str, enum.Enum):
    """The `--type` choices for `st build`, lower-cased for the CLI surface."""

    release = "release"
    debug = "debug"
    relwithdebinfo = "relwithdebinfo"
    minsizerel = "minsizerel"
```

(Move the `import enum` to the top of the file with the other imports, not inline.)

In `cli/sushitrack_cli/services/tests.py`, add after `DEFAULT_SUITE` (line 16):

```python
import enum


class Suite(str, enum.Enum):
    """The `--suite` choices for `st test`."""

    unit = "unit"
    integration = "integration"
    regression = "regression"
    functional = "functional"
    all = "all"
```

(Move `import enum` to the top of the file.)

In `cli/sushitrack_cli/services/deploy.py`, add after `KINDS = ("cpp", "python")` (line 15):

```python
import enum


class DeployKind(str, enum.Enum):
    """The `deploy` command's consumer-flavour argument."""

    cpp = "cpp"
    python = "python"
```

(Move `import enum` to the top of the file.)

- [ ] **Step 4: Run the test to verify it passes**

Run: `python -m pytest cli/tests/test_service_enums.py -v`
Expected: 3 passed.

- [ ] **Step 5: Commit**

```bash
git add cli/sushitrack_cli/services/build.py cli/sushitrack_cli/services/tests.py cli/sushitrack_cli/services/deploy.py cli/tests/test_service_enums.py
git commit -m "feat(cli): add BuildType, Suite and DeployKind enums to their services"
```

---

### Task 3: Rewrite the command layer on Typer

**Files:**
- Delete: `cli/sushitrack_cli/app.py`
- Modify: `cli/sushitrack_cli/cli.py` (full rewrite)
- Modify: `cli/sushitrack_cli/console.py` (add `current()`, one function)
- Test: `cli/tests/test_help_screen.py` (create)

**Interfaces:**
- Consumes: `services.build.BuildType`, `services.tests.Suite`, `services.deploy.DeployKind` (Task 2); `sushicore.typer_help.help_group` (Task 1); every existing `services.*` function, unchanged, at the signatures read in `cli/sushitrack_cli/services/*.py` today.
- Produces: `sushitrack_cli.cli.app` (a `typer.Typer` instance, replacing the old `sushitrack_cli.app.App` instance of the same name) and `sushitrack_cli.cli.entry(argv=None, prog=None)` (same signature as today, still the `st`/`sushitrack` `pyproject.toml` script target).

- [ ] **Step 0: Add `console.current()`**

The new `cli.py` (Step 4 below) calls `console.current`, mirroring `sushiai.console.current`. `console.py` itself is otherwise unchanged (see spec §2) — this is the one function it is missing relative to its sibling. Add to `cli/sushitrack_cli/console.py`, after `_lazy = LazyConsole(config_dir)`:

```python
def current():
    """Return the sushicore Console this run prints through, building it on first use."""
    return _lazy.get()
```

- [ ] **Step 1: Write the failing test**

Create `cli/tests/test_help_screen.py`:

```python
"""st's help screen groups its commands under three headings, in the same
order the removed app.py's app.section(...) calls declared them."""

from __future__ import annotations

from typer.testing import CliRunner

from sushitrack_cli.cli import app

K_GROUPS = {
    "Project": ["build", "test", "run", "clean", "deploy", "docker"],
    "Benchmark": ["eval", "infer"],
    "Diagnostics": ["status", "doctor", "config", "paths", "env"],
}

K_EXAMPLES = {
    "build": "st build --type debug",
    "test": "st test --suite unit",
    "run": "st run unit_test",
    "clean": "st clean",
    "deploy": "st deploy cpp",
    "eval": "st eval --compare",
    "config": "st config",
    "paths": "st paths",
    "env": "st env --all",
}

# Commands whose --help is forwarded to the program they run, so Typer draws no page.
K_FORWARDING = {"infer"}


def _run(*args: str) -> str:
    result = CliRunner().invoke(app, list(args))
    assert result.exit_code == 0, result.output
    return result.output


def _commands_by_heading(page: str) -> dict[str, list[str]]:
    """The first word of each command row, keyed by the heading printed above it."""
    grouped: dict[str, list[str]] = {}
    heading = ""
    for line in page.splitlines():
        stripped = line.strip()
        if line and not line.startswith(" "):
            heading = stripped
            grouped[heading] = []
        elif line.startswith("  ") and stripped and heading:
            grouped[heading].append(stripped.split()[0])
    return grouped


def test_root_help_lists_the_three_groups_in_order():
    lines = [line.rstrip() for line in _run("--help").splitlines()]
    positions = [lines.index(heading) for heading in K_GROUPS if heading in lines]
    assert positions == sorted(positions)
    assert len(positions) == len(K_GROUPS)


def test_every_command_sits_under_its_own_group():
    grouped = _commands_by_heading(_run("--help"))
    for heading, names in K_GROUPS.items():
        assert set(grouped.get(heading, [])) >= set(names)


def test_a_bare_invocation_prints_the_same_page():
    result = CliRunner().invoke(app, [])
    assert result.output.strip() == _run("--help").strip()


def test_docker_sub_app_draws_its_own_page():
    out = _run("docker", "--help")
    assert "build" in out and "run" in out
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `python -m pytest cli/tests/test_help_screen.py -v`
Expected: FAIL — `ModuleNotFoundError` or `ImportError: cannot import name 'app'` (the old `app.py`'s `App` instance is not a Typer app the test's `CliRunner` can drive), or a collection error since `cli.py` still imports `app.py`.

- [ ] **Step 3: Delete `app.py`**

```bash
git rm cli/sushitrack_cli/app.py
```

- [ ] **Step 4: Rewrite `cli.py`**

Replace the full contents of `cli/sushitrack_cli/cli.py` with:

```python
"""The SushiTrack developer CLI (`st` / `sushitrack`).

Thin Typer layer: each command parses arguments and delegates to
:mod:`sushitrack_cli.services`. Nothing here does real work, so the surface
stays readable as one file and the behaviour stays testable without Typer.
"""

from __future__ import annotations

import sys
import types
from pathlib import Path
from typing import Optional

import typer
from sushicore.typer_help import help_group

from . import console
from .config import context
from .services import build as build_svc
from .services import deploy as deploy_svc
from .services import diag as diag_svc
from .services import docker as docker_svc
from .services import infer as infer_svc
from .services import label as label_svc
from .services import runner as run_svc
from .services import tests as test_svc
from .services.build import BuildType
from .services.deploy import DeployKind
from .services.tests import Suite

K_PROJECT = "Project"
K_BENCHMARK = "Benchmark"
K_DIAGNOSTICS = "Diagnostics"

_help_group = help_group(console.current)

app = typer.Typer(
    name="st",
    cls=_help_group,
    help="SushiTrack developer CLI — build, test, evaluate, run and Docker.",
    no_args_is_help=True,
    add_completion=False,
    rich_markup_mode="rich",
)


# --------------------------------------------------------------------------- #
# build / test / run / clean / deploy (Project)
# --------------------------------------------------------------------------- #
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
    epilog="st test  # functional: unit + integration + regression\n"
           "st test --suite unit\n"
           "st test --suite unit -f 'RectTest'\n"
           "st test --suite integration --repeat 3\n"
           "st test --suite all -j 8",
)
def test(
    suite: Suite = typer.Option(
        Suite(test_svc.DEFAULT_SUITE), "--suite", "-s", case_sensitive=False,
        help="CTest label group to run ('all' runs every test). "
             f"Default: {test_svc.DEFAULT_SUITE}."),
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
    """Run the test suites via CTest labels.

    For GTest-level knobs (shuffle, break-on-failure) run the binary directly:
    st run unit_test -- --gtest_shuffle --gtest_break_on_failure

    Metric evaluation and video inference are not CTest suites — see `st eval`
    and `st infer`.
    """
    cfg, paths = context()
    raise typer.Exit(test_svc.test(cfg, paths, suite=suite.value, filter=filter,
                                   repeat=repeat, jobs=jobs))


@app.command(
    "run",
    rich_help_panel=K_PROJECT,
    context_settings={"allow_extra_args": True, "ignore_unknown_options": True},
    epilog="st run  # Pick from a list\n"
           "st run unit_test\n"
           "st run unit_test -- --gtest_filter='TrackletTest.*'",
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


# --------------------------------------------------------------------------- #
# docker (Project)
# --------------------------------------------------------------------------- #
docker_app = typer.Typer(
    cls=_help_group,
    help="Docker image and container management.",
    no_args_is_help=True,
)
app.add_typer(docker_app, name="docker", rich_help_panel=K_PROJECT)


@docker_app.command(
    "build",
    context_settings={"allow_extra_args": True, "ignore_unknown_options": True},
    epilog="st docker build\nst docker build --no-cache",
)
def docker_build(
    ctx: typer.Context,
    no_cache: bool = typer.Option(
        False, "--no-cache", help="Rebuild every layer, ignoring Docker's cache."),
):
    """Build the SushiTrack Docker image."""
    cfg, paths = context()
    raise typer.Exit(docker_svc.build(cfg, paths, no_cache=no_cache,
                                      forwarded=list(ctx.args)))


@docker_app.command(
    "run",
    context_settings={"allow_extra_args": True, "ignore_unknown_options": True},
    epilog="st docker run\nst docker run -- bash -lc 'echo hi'",
)
def docker_run(
    ctx: typer.Context,
    no_gpu: bool = typer.Option(
        False, "--no-gpu",
        help="Omit --gpus all (no GPU available, or a CPU-only run)."),
):
    """Start the developer container with this repo mounted.

    Args after `--` are the command to run inside the container, in place of
    an interactive bash: st docker run -- bash -lc 'echo hi'
    """
    cfg, paths = context()
    raise typer.Exit(docker_svc.run(cfg, paths, no_gpu=no_gpu,
                                    command=list(ctx.args) or None))


# --------------------------------------------------------------------------- #
# eval / infer (Benchmark)
# --------------------------------------------------------------------------- #
@app.command(
    "eval",
    rich_help_panel=K_BENCHMARK,
    epilog="st eval\n"
           "st eval --compare\n"
           "st eval --scenario MOT17-04-DPM --compare\n"
           "st eval --eval-only --export ./results\n"
           "st eval --trackeval --private",
)
def evaluate(
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
    """Run the MOT label benchmark and print TrackEval metrics.

    Drives the regression binary over det.txt-backed sequences, then scores
    the predictions (HOTA / DetA / AssA / MOTA / IDF1 / IDSW / latency).
    """
    cfg, paths = context()
    key = label_svc.resolve_tracker(tracker or cfg.test_tracker)
    raise typer.Exit(label_svc.evaluate_sequences(
        cfg, paths, scenario=scenario, tracker=key, compare=compare,
        eval_only=eval_only, trackeval=trackeval, export=export,
        private=private, public=public))


@app.command(
    "infer",
    rich_help_panel=K_BENCHMARK,
    context_settings={"allow_extra_args": True, "ignore_unknown_options": True},
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


# --------------------------------------------------------------------------- #
# status / doctor / config / paths / env (Diagnostics)
# --------------------------------------------------------------------------- #
@app.command("status", rich_help_panel=K_DIAGNOSTICS, epilog="st status")
def status():
    """Show what is built, what is missing and which data sets are present."""
    raise typer.Exit(diag_svc.status(*context()))


@app.command("doctor", rich_help_panel=K_DIAGNOSTICS, epilog="st doctor")
def doctor():
    """Check the toolchain and data this project needs, and report what is missing."""
    raise typer.Exit(diag_svc.doctor(*context()))


@app.command("config", rich_help_panel=K_DIAGNOSTICS, epilog="st config")
def config():
    """Print the resolved configuration and where each value came from."""
    raise typer.Exit(diag_svc.config_show(*context()))


@app.command("paths", rich_help_panel=K_DIAGNOSTICS, epilog="st paths")
def paths():
    """Print the resolved project directories (replaces cli/paths.env)."""
    raise typer.Exit(diag_svc.paths_show(*context()))


@app.command(
    "env", rich_help_panel=K_DIAGNOSTICS,
    epilog="st env\nst env --all  # Show every variable, not just build-relevant",
)
def env(
    show_all: bool = typer.Option(
        False, "--all", "-a",
        help="Show every variable, not just the build-relevant ones."),
):
    """Print the environment cmake, ctest and the test binaries execute under."""
    cfg, project_paths = context()
    raise typer.Exit(diag_svc.env_dump(cfg, project_paths, show_all=show_all))


def entry(argv=None, prog=None):
    """Process entry point: returns an exit code, never raises for user error."""
    if prog:
        app.info.name = prog
    elif sys.argv:
        stem = Path(sys.argv[0]).stem.lower()
        if stem in ("sushitrack", "st"):
            app.info.name = stem
    try:
        return app(args=argv, standalone_mode=False)
    except SystemExit as exc:
        return exc.code or 0


if __name__ == "__main__":
    app()
```

- [ ] **Step 5: Run the new test to verify it passes**

Run: `python -m pytest cli/tests/test_help_screen.py -v`
Expected: 4 passed.

- [ ] **Step 6: Run the full CLI test suite**

Run: `python -m pytest cli/tests -v`
Expected: all tests pass, including `test_env.py`, `test_proc.py` (Task 1's unrelated pass-through) and `test_service_enums.py` (Task 2).

- [ ] **Step 7: Manually verify every command's `--help` renders**

Run each of these and confirm no traceback and that the description/options are legible:

```bash
st --help
st build --help
st test --help
st run --help
st clean --help
st deploy --help
st docker --help
st docker build --help
st docker run --help
st eval --help
st infer --help
st status --help
st doctor --help
st config --help
st paths --help
st env --help
```

- [ ] **Step 8: Commit**

```bash
git add cli/sushitrack_cli/cli.py cli/sushitrack_cli/console.py cli/tests/test_help_screen.py
git rm cli/sushitrack_cli/app.py
git commit -m "refactor(cli)!: rebuild the st command layer on Typer

Replaces app.py's argparse wrapper with typer.Typer, matching sa/sb/sd/hub.
st infer --tsize is now a fixed two-value option, and st docker run's
container command moves from --command/-c to the trailing -- args, both
per docs/design/CLI_TYPER_MIGRATION.md §3.5.

Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>"
```

---

### Task 4: Config layering contract test

**Files:**
- Test: `cli/tests/test_config.py` (create)

**Interfaces:**
- Consumes: `sushitrack_cli.config.Config`, `sushitrack_cli.config.load_config`, `sushitrack_cli.config._merge_platform`, `sushitrack_cli.config.ENV_OVERRIDES` (all exist today, unchanged).

- [ ] **Step 1: Write the test**

Create `cli/tests/test_config.py`:

```python
"""The sectioned config layering: defaults -> config.toml -> config.local.toml
-> ST_* env, with a per-platform overlay. Mirrors the sibling CLIs' own
test_config.py, adapted to this CLI's sectioned schema (see config.py's
module docstring for why it is sectioned rather than flat)."""

from __future__ import annotations

import platform

import pytest

from sushitrack_cli import config as config_module


def test_defaults_are_used_when_no_file_or_env_overrides(tmp_path, monkeypatch):
    monkeypatch.setattr(config_module, "config_dir", lambda root=None: tmp_path)
    for var in config_module.ENV_OVERRIDES.values():
        monkeypatch.delenv(var, raising=False)

    cfg = config_module.load_config(tmp_path)

    assert cfg.build_type == "Release"
    assert cfg.sources["build_type"] == "default"


def test_config_toml_overrides_a_default(tmp_path, monkeypatch):
    monkeypatch.setattr(config_module, "config_dir", lambda root=None: tmp_path)
    (tmp_path / "config.toml").write_text(
        "[build]\ntype = \"Debug\"\n", encoding="utf-8")

    cfg = config_module.load_config(tmp_path)

    assert cfg.build_type == "Debug"
    assert cfg.sources["build_type"] == "config.toml"


def test_config_local_toml_wins_over_config_toml(tmp_path, monkeypatch):
    monkeypatch.setattr(config_module, "config_dir", lambda root=None: tmp_path)
    (tmp_path / "config.toml").write_text(
        "[build]\ntype = \"Debug\"\n", encoding="utf-8")
    (tmp_path / "config.local.toml").write_text(
        "[build]\ntype = \"RelWithDebInfo\"\n", encoding="utf-8")

    cfg = config_module.load_config(tmp_path)

    assert cfg.build_type == "RelWithDebInfo"
    assert cfg.sources["build_type"] == "config.local.toml"


def test_an_env_override_wins_over_every_file(tmp_path, monkeypatch):
    monkeypatch.setattr(config_module, "config_dir", lambda root=None: tmp_path)
    (tmp_path / "config.toml").write_text(
        "[build]\ntype = \"Debug\"\n", encoding="utf-8")
    monkeypatch.setenv("ST_BUILD_TYPE", "MinSizeRel")

    cfg = config_module.load_config(tmp_path)

    assert cfg.build_type == "MinSizeRel"
    assert cfg.sources["build_type"] == "env:ST_BUILD_TYPE"


def test_a_platform_table_overlays_the_current_platform_only(tmp_path, monkeypatch):
    monkeypatch.setattr(config_module, "config_dir", lambda root=None: tmp_path)
    other = "linux" if platform.system().lower() == "windows" else "windows"
    (tmp_path / "config.toml").write_text(
        f"[build]\ncmake = \"cmake-default\"\n"
        f"[platform.{platform.system().lower()}]\nbuild.cmake = \"cmake-here\"\n"
        f"[platform.{other}]\nbuild.cmake = \"cmake-elsewhere\"\n",
        encoding="utf-8")

    cfg = config_module.load_config(tmp_path)

    assert cfg.build_cmake == "cmake-here"


def test_a_bool_field_coerces_common_truthy_env_spellings(tmp_path, monkeypatch):
    monkeypatch.setattr(config_module, "config_dir", lambda root=None: tmp_path)
    monkeypatch.setenv("ST_DOCKER_GPUS", "no")

    cfg = config_module.load_config(tmp_path)

    assert cfg.docker_gpus is False
```

- [ ] **Step 2: Run it to verify it fails first, unpatched**

Run: `python -m pytest cli/tests/test_config.py -v` (before checking whether `config_dir` is monkeypatchable — it is a module-level function assigned from `_MODULE.find_project_root`/`config_dir`, so `monkeypatch.setattr(config_module, "config_dir", ...)` replaces the module attribute `load_config`'s inner calls read through `config_dir(root)`).
Expected at this point: PASS immediately, since `config.py` already implements this behaviour — this test suite documents and pins existing, correct behaviour rather than driving new code (there is no new implementation step in this task).

- [ ] **Step 3: Run the full suite once more to confirm no regression**

Run: `python -m pytest cli/tests -v`
Expected: all tests pass.

- [ ] **Step 4: Commit**

```bash
git add cli/tests/test_config.py
git commit -m "test(cli): pin the sectioned config layering and env override"
```

---

### Task 5: Lazy console contract test

**Files:**
- Test: `cli/tests/test_console_lazy.py` (create)

**Interfaces:**
- Consumes: `sushitrack_cli.console.current()` (added in Task 3, Step 0) and the module's existing `__getattr__`/PEP 562 laziness (unchanged).

- [ ] **Step 1: Write the test**

Create `cli/tests/test_console_lazy.py`:

```python
"""Importing sushitrack_cli.console must not touch the filesystem: it has to
stay importable from `st --help`, which runs with no project checkout at
all. Mirrors the sibling CLIs' own test_console_lazy.py."""

from __future__ import annotations

import importlib
import sys


def test_importing_console_does_not_resolve_the_project_root(monkeypatch):
    monkeypatch.delitem(sys.modules, "sushitrack_cli.console", raising=False)

    def _fail(*args, **kwargs):
        raise AssertionError("config_dir was called at import time")

    import sushitrack_cli.config as config_module
    monkeypatch.setattr(config_module, "config_dir", _fail)

    importlib.import_module("sushitrack_cli.console")


def test_current_returns_a_console_object():
    from sushitrack_cli import console

    assert console.current() is console.console
```

- [ ] **Step 2: Run it to verify it passes**

Run: `python -m pytest cli/tests/test_console_lazy.py -v`
Expected: 2 passed (the laziness already held before Task 3; `current()`, added in Task 3, is a one-line accessor over the existing `_lazy.get()`).

- [ ] **Step 3: Run the full suite**

Run: `python -m pytest cli/tests -v`
Expected: all tests pass.

- [ ] **Step 4: Commit**

```bash
git add cli/tests/test_console_lazy.py
git commit -m "test(cli): pin the console module's import-time laziness"
```

---

### Task 6: Document the `diag.py`/`sushicore.diag.Diagnostics` divergence

**Files:**
- Modify: `cli/sushitrack_cli/services/diag.py`

**Interfaces:** None (docstring only; no behaviour change).

- [ ] **Step 1: Extend the module docstring**

Replace line 1 of `cli/sushitrack_cli/services/diag.py`:

```python
"""Read-only diagnostics: status, doctor, config, env, paths."""
```

with:

```python
"""Read-only diagnostics: status, doctor, config, env, paths.

Unlike sibling CLIs' diag.py, this does not delegate config_show/env_dump to
sushicore.diag.Diagnostics: that class's ConfigModule protocol assumes a
flat, single-table config (``_ENV_OVERRIDES`` keyed directly by field name),
and SushiTrack's Config is sectioned (paths_*, build_*, docker_*, test_*) for
the reason config.py's own module docstring gives. status, doctor and
paths_show also have no Diagnostics equivalent to begin with.
"""
```

- [ ] **Step 2: Run the full suite to confirm no regression**

Run: `python -m pytest cli/tests -v`
Expected: all tests pass (docstring-only change).

- [ ] **Step 3: Commit**

```bash
git add cli/sushitrack_cli/services/diag.py
git commit -m "docs(cli): explain why diag.py does not use sushicore.diag.Diagnostics"
```

---

### Task 7: Update `docs/README.md` and the changelog

**Files:**
- Modify: `docs/README.md`
- Modify: `docs/reference/CHANGELOG.md`

**Interfaces:** None (documentation only).

- [ ] **Step 1: Remove the `app.py` line from the Project Structure tree**

In `docs/README.md`, delete this line (currently line 279):

```
    │   ├── app.py                # Declarative argparse layer
```

- [ ] **Step 2: Update the Internal Tools table**

In `docs/README.md`, replace this row (currently line 1221):

```
| `cli/sushitrack_cli/app.py` | Declarative argparse layer (command registry, grouped help, `--` passthrough) |
```

by deleting it, and edit the `cli.py` row just above it (currently line 1220) from:

```
| `cli/sushitrack_cli/cli.py` | Flat command declarations delegating to the service layer |
```

to:

```
| `cli/sushitrack_cli/cli.py` | Typer command declarations delegating to the service layer, plus the `docker` sub-app |
```

- [ ] **Step 3: Update the CLI installation description**

In `docs/README.md`'s Installation section, the sentence "The CLI is a small pipx-installed package, built on `sushicore` — the console, theme and config-layering foundation shared with every other Sushi\* developer CLI" already covers the change accurately (it never named argparse), so no edit is needed there — only the two references above named the old command layer explicitly.

- [ ] **Step 4: Add the changelog entry**

In `docs/reference/CHANGELOG.md`, add to the top of the `## Unreleased` section (matching the file's existing entry format — one line, past tense, paths in backticks):

```
- 2026-09-22 — cli: Rebuilt the `st` command layer on Typer, matching every sibling Sushi* CLI (`cli/sushitrack_cli/cli.py`, removed `cli/sushitrack_cli/app.py`).
```

- [ ] **Step 5: Verify every doc'd example still runs**

Run each of `docs/README.md`'s CLI Reference examples against the built CLI (a syntax/exit-code check, not a full build):

```bash
st build --help
st test --help
st eval --help
st infer --help
st clean --help
st docker build --help
st docker run --help
```

Expected: every invocation exits 0 and prints help text with no traceback.

- [ ] **Step 6: Commit**

```bash
git add docs/README.md docs/reference/CHANGELOG.md
git commit -m "docs: update the CLI reference for the Typer command layer"
```

---

## Final Verification

- [ ] `python -m pytest cli/tests -v` — every test passes (existing `test_env.py`, `test_proc.py`, plus `test_service_enums.py`, `test_help_screen.py`, `test_config.py`, `test_console_lazy.py`).
- [ ] `st --help` and every subcommand's `--help` render without a traceback, with the three `rich_help_panel` groups in order.
- [ ] `st docker run -- bash -lc "echo hi"` and `st infer --tsize 800 1440` behave as documented in Task 3 Step 8's commit message and §3.5 of the spec.
- [ ] `git log --oneline` on this branch shows one commit per task, each following the `commits` skill's subject grammar.
