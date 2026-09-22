# `st` CLI: Typer Migration

**Status:** Open — phase 0 of 1

## 1. Context and Motivation

`sushitrack_cli`'s configuration and console layers already use `sushicore.module_config.ModuleConfig`/`ModuleProfile` and `sushicore.cli_console.LazyConsole`, matching every other Sushi* developer CLI (`sa`, `sb`, `sd`, `hub`). The one place `st` diverges is the command layer: `cli/sushitrack_cli/app.py` is a hand-rolled, declarative wrapper over `argparse` (`App`/`Opt`/`Group`), while every sibling CLI declares its commands with Typer. `app.py`'s own docstring names the one behavioural cost of that choice — argparse's raw `--help` section headings go uncoloured, because sushicore's `Console` exposes only semantic print calls, not a "paint this string" primitive argparse's own `write()` could use.

Per the SushiSkills doctrine, "siblings that do the same kind of thing are shaped the same." This document specifies bringing `st`'s command layer onto Typer, closing the one gap a review against `sushiai`'s and `sushistack`'s CLIs found, and specifies the two follow-on gaps that review surfaced: a test suite thinner than its siblings', and one code comment worth adding while the command layer is being rewritten anyway.

## 2. Scope

In scope:

- `cli/sushitrack_cli/app.py` and `cli/sushitrack_cli/cli.py` — the command declaration layer.
- `cli/pyproject.toml` — the `typer` dependency and the `sushicore` version floor.
- `cli/tests/` — new contract tests mirroring the sibling CLIs' `test_help_screen.py`, `test_config.py`, `test_console_lazy.py`.
- `cli/sushitrack_cli/services/diag.py` — one added comment (no behaviour change; see §3.3).

Out of scope (reviewed, no gap found):

- `cli/sushitrack_cli/config.py` — already builds on `ModuleConfig`/`ModuleProfile`; its sectioned schema is a documented, deliberate deviation from `sushicore.config_base.ToolConfig`'s flat table (SushiTrack provisions nothing from the shared SushiStack dependency tree, unlike SushiAI/SushiEngine/SushiBLAS).
- `cli/sushitrack_cli/console.py` — already the same `LazyConsole` wrapper every sibling CLI uses, unchanged.
- `cli/sushitrack_cli/services/{build,deploy,discovery,docker,evaluate,infer,label,runner,tests}.py` — already shaped as thin `(cfg, paths, ...) -> exit code` functions, the same shape siblings use; the extra files (`evaluate`, `infer`, `label`) are domain-specific to SushiTrack (MOT benchmarking, YOLOX inference) and have no sibling counterpart to match against.

## 3. Design

### 3.1 Command layer

Replace `app.py`'s `App`/`Opt`/`Group`/`main` with a `typer.Typer` app, following `sushiai.cli`'s pattern:

- `app = typer.Typer(name="st", cls=help_group(console.current), help=..., no_args_is_help=True, add_completion=False, rich_markup_mode="rich")`, where `help_group` comes from `sushicore.typer_help`.
- The three existing `app.section(...)` groups (`Project`, `Benchmark`, `Diagnostics`) become `rich_help_panel` values on each `@app.command(...)`.
- Every command carries an `epilog` with one or more example invocations, the convention every sibling CLI's commands already follow.
- Fixed-choice flags become `Enum`s instead of `choices=[...]` strings, passed as `typer.Option(..., case_sensitive=False)`:
  - `--type` (`build`) → a `BuildType` enum (`release`/`debug`/`relwithdebinfo`/`minsizerel`).
  - `--suite` (`test`) → a `Suite` enum (`unit`/`integration`/`regression`/`functional`/`all`).
  - `deploy`'s positional `kind` → a `DeployKind` enum (`cpp`/`python`).
  - `--tracker` (`eval`, `infer`) stays a plain string: its value set is a runtime registry (`sushitrack`/`bytetrack`/`ocsort` today), not a closed CLI-level choice, and label.py's `resolve_tracker` already validates it.
  These enums live beside the service function that already validates/normalizes the value today (`services/build.py`'s `normalize_build_type`, `services/tests.py`'s `SUITES`, `services/deploy.py`'s `KINDS`), so the CLI layer imports them rather than redeclaring the value set.
- `docker build`/`docker run` become a `typer.Typer` sub-app added with `app.add_typer(docker_app, name="docker", rich_help_panel="...")`, mirroring `sushiai.cli`'s `demo_app` pattern.
- Commands that forward trailing arguments (`run`, `infer`, `docker build`, `docker run`) take `ctx: typer.Context` and read `ctx.args`, with `context_settings={"allow_extra_args": True, "ignore_unknown_options": True}`; the current `_split_forwarded` helper and the `Opt(forwards=True)` bookkeeping are removed, since Typer's context already carries this.
- `entry()`'s program-name detection (`sys.argv[0]` stem → `st`/`sushitrack`) is kept: Typer apps are still invoked through a single callable, so the `st = "sushitrack_cli.cli:entry"` / `sushitrack = "sushitrack_cli.cli:entry"` script entries in `pyproject.toml` keep working unchanged, `entry()` now calling `app()` (via `typer.main.get_command(app)` if the prog-name override needs to reach Click) instead of `main(app, argv)`.
- `services/` function signatures do not change. Every command body still reads `cfg, paths = context()` and calls into the existing service function; only the argument-parsing declaration above it changes.

### 3.2 Dependency

`cli/pyproject.toml`: raise the floor from `sushicore>=0.1.0` to `>=0.4.0` (the version that ships `sushicore.typer_help`) and add `typer>=0.12`, matching `sushiai-cli`'s and `sushidsp-cli`'s floors. Verified available: PyPI carries `sushicore` 0.4.0, and the local environment now has it installed with `sushicore.typer_help.help_group` importable.

`cli/sushistack.deps.toml` is unaffected — it lists native/system dependencies (GoogleTest), not Python packages.

### 3.3 `services/diag.py` note

No behavioural change. `status`, `doctor`, and `paths_show` do not call `sushicore.diag.Diagnostics` because that class's `ConfigModule` protocol assumes a flat, single-table config (`_ENV_OVERRIDES` keyed directly by field name); SushiTrack's `Config` is sectioned (`paths_*`, `build_*`, `docker_*`, `test_*`), for the reason `config.py`'s own module docstring already gives. Add one line to `diag.py`'s module docstring stating this explicitly, so a future reader comparing it against a sibling's `diag.py` (which does delegate to `Diagnostics`) does not read the difference as an oversight.

### 3.4 Tests

Add, mirroring the sibling CLIs' `cli/tests/`:

- `test_help_screen.py` — root `--help` lists `Project`/`Benchmark`/`Diagnostics` in order, each command sits under its own heading, a bare invocation prints the same page (`no_args_is_help=True`), every command's `--help` carries its epilog example, `docker --help` draws its own sub-app page, and forwarding commands (`run`, `infer`, `docker build`, `docker run`) keep their own `--help` for the program they invoke rather than Typer's.
- `test_config.py` — the sectioned layering (`config.toml` → `config.local.toml` → `ST_*` env) and the platform-table overlay resolve as documented.
- `test_console_lazy.py` — importing `sushitrack_cli.console` touches neither the filesystem nor `sys.argv`, and the console builds only on first attribute access.

`test_env.py` and `test_proc.py` are unchanged.

## 4. Non-goals

- No change to any command's name, flag name, default value, or exit-code behaviour. This is a framework swap, not a UX redesign.
- No change to `config.py`'s sectioned schema or to `console.py`.
- No change to the Docker image, build system, or any C++ code.

## 5. Verification

- `st --help`, `st <command> --help` for every command, and `st docker --help` render correctly (colored section headings — the exact gap the argparse docstring named).
- `python -m pytest cli/tests` passes, including the new `test_help_screen.py`, `test_config.py`, `test_console_lazy.py`.
- Every existing example invocation in `docs/README.md`'s CLI Reference section still runs unchanged.
