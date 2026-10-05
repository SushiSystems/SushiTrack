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
| `cli/sushitrack_cli/services/` | build, tests (ctest), label (`st eval`), infer, evaluate (TrackEval), deploy, docker, diag, artifacts, checks, runner |
| `cli/config.toml` | Tracked CLI defaults (`cli`, `paths`, `build`, `docker`, `test`, `tool` sections) |
| `cli/tests/` | The pytest suite for the CLI; it runs without a build |
| `cli/sushistack.deps.toml` | SushiStack dependency fragment: GoogleTest, the only port SushiTrack does not vendor |
| `cli/misc/clean_preds.sh` / `cli/misc/clean_preds.bat` | Recursively removes `pred/` directories under the evaluator data root |
| `cli/misc/construct_trackeval_data.ps1` | Creates the official TrackEval directory tree by symlinking GT (MOT17-SDP as representative for each sequence) and copying per-tracker predictions for `sushitrack`/`bytetrack`/`ocsort` |

`Dockerfile`: CUDA 12.1 devel base, installs `build-essential`, `cmake`, `git`, `python3`, `python3-pip`, `libgtest-dev`, `libgl1-mesa-glx`, `libglib2.0-0`, then pip-installs the pinned wheel set, copies the project, and installs the CLI with `pip3 install`, which puts `st` on `PATH`.
