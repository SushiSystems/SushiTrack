# The `st` package

The CLI lives in `cli/` as the package `sushitrack-cli`. Its commands are in the
[command line reference](../reference/CLI.md).

| File | Purpose |
|---|---|
| `cli/pyproject.toml` | Package metadata; installed with pipx, provides the `st` and `sushitrack` entry points |
| `cli/sushitrack_cli/cli.py` | Typer command declarations delegating to the service layer, plus the `container` sub-app and the shared `sushicore` surfaces |
| `cli/sushitrack_cli/aliases.py` | The old command spellings `st` still answers to, each with its last day |
| `cli/sushitrack_cli/errors.py` | The failures `st` prints as one line with exit code 1 |
| `cli/sushitrack_cli/config.py` | Layered TOML config, `ST_*` overrides, project-root and path resolution via `sushicore.module_config` |
| `cli/sushitrack_cli/console.py` | `sushicore`'s console, theme and icon set, wired in lazily |
| `cli/sushitrack_cli/env.py` | vcvars discovery and build-environment snapshotting on Windows |
| `cli/sushitrack_cli/process.py` | The `sushicore` process runner every service shares |
| `cli/sushitrack_cli/__main__.py` | Entry for `python -m sushitrack_cli` |
| `cli/sushitrack_cli/services/` | build, tests (ctest and the pytest suite), test_label (the `--label` selection), label (`st eval`, `st clean --predictions`), infer, evaluate (TrackEval), deploy, docker, diag, artifacts, checks, runner |
| `cli/sushitrack_cli/templates/` | The example sources and READMEs `st deploy` writes into a package |
| `cli/config.toml` | Tracked CLI defaults (`cli`, `paths`, `build`, `docker`, `test`, `tool` sections) |
| `cli/tests/` | The pytest suite for the CLI, mirroring `cli/sushitrack_cli/`; `st test --cli` runs it, with no build |
| `cli/sushistack.deps.toml` | SushiStack dependency fragment: the SushiLog pin and GoogleTest, the only port SushiTrack does not vendor |

`.config/docker/Dockerfile`: CUDA 12.1 devel base, installs `build-essential`, `cmake`, `git`, `python3`, `python3-pip`, `libgtest-dev`, `libgl1-mesa-glx`, `libglib2.0-0`, then pip-installs the pinned wheel set, copies the project, and installs the CLI with `pip3 install`, which puts `st` on `PATH`.

## Decisions the sources cite

### The root marker

`PROFILE` in `cli/sushitrack_cli/config.py` describes SushiTrack to the shared `sushicore`
machinery. Its root marker is the C API header, `include/SushiTrack/c_api/sushitrack_c.h`. A bare
`CMakeLists.txt` exists in any enclosing checkout, but this path only ever sits at the SushiTrack
root.

### Environment overrides

`ENV_OVERRIDES` in `cli/sushitrack_cli/config.py` maps a config field to the environment variable
that overrides it. The table is sectioned, not flat.
`sushicore.profile.ModuleProfile.env_overrides()` assumes one `ToolConfig` field per override, so
the table stays local to `config.py` and is not derived from the shared profile.

### The lazy console

`cli/sushitrack_cli/console.py` is a thin wrapper around `sushicore`. The theme, icon and renderer
logic and the `[cli]` config schema live there, shared with every other Sushi CLI; sushicore's
README says how to change the colours.

`sushicore.cli_console.LazyConsole` builds the console on first use. Constructing it needs this
repository's `cli/` directory, and locating that fails outside a checkout: `config_dir()` calls
`find_project_root()`, which raises `SystemExit`. `cli.py` imports `console` transitively, so a
console built at import time would stop every `st` invocation outside a checkout, `st --help`
included. The module assigns `__getattr__` (PEP 562) instead: attributes resolve on first access,
so importing it never touches the filesystem or the project layout.

The lifecycle lives in `LazyConsole`. `cli/tests/test_console.py` pins that this CLI is wired to it.
