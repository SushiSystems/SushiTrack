# Known issues

Defects the estate audit of 2026-10-05 found that are still open. The audit is
`docs/agent/2026_10_05_ESTATE_AUDIT/REPORT.md`. Work that is planned and not a defect is in
[Remaining work](../design/REMAINING_WORK.md).

## C ABI

| Issue | Where |
| --- | --- |
| `sushitrack_update` and `sushitrack_get_tracklets` return 0 on a caught exception and on a rejected argument, which a caller cannot tell from a frame with no tracks; the ABI has no status return on these calls and no last-error call | `source/sushitrack_c.cpp`, `include/SushiTrack/sushitrack_c.h` |
| A log callback that throws while a C entry point's handler logs at ERROR level throws again from inside the handler and crosses the C boundary | `source/logger.cpp`, `source/sushitrack_c.cpp` |
| `sushitrack_create_ex` can return `SUSHITRACK_RUNTIME_ERROR` with `*out_tracker` already set when its last log line throws, and `sushitrack_create` then drops that handle; a throwing `Tracker` constructor leaks the implementation object | `source/sushitrack_c.cpp` |

## Installing and versions

| Issue | Where |
| --- | --- |
| The conda environment pins `sushicore==0.4.0` and the CLI requires `sushicore>=0.7.0`, so an environment built from the file cannot run `st` | `environment.yml`, `cli/pyproject.toml` |
| The image build installs the CLI with `pip3 install` and names no source for `sushicore` 0.7.0 | `Dockerfile` |
| Four version numbers disagree: the library and the binding say 1.0.0, the CLI package 0.1.0 and its `__version__` 2.0.0 | `CMakeLists.txt`, `bindings/python/pyproject.toml`, `cli/pyproject.toml`, `cli/sushitrack_cli/__init__.py` |
| The changelog has a `v1.0.0` section and no `v1.0.0` tag exists | `docs/reference/CHANGELOG.md`, git |
| The `v1.0.0` section says an open-source version was released; the repository was private and nothing was published | `docs/reference/CHANGELOG.md` |
| The GoogleTest fragment says the test targets are off by default, while `st build` builds them unless `--no-test` is given, and it carries no check command, so `st doctor` never looks for GoogleTest | `cli/sushistack.deps.toml` |

## Repository hygiene

| Issue | Where |
| --- | --- |
| Test discovery writes `cmake_test_discovery_*.json` into the repository root | `tests/unit/CMakeLists.txt`, `tests/integration/CMakeLists.txt`, `tests/regression/CMakeLists.txt` |
| 234 MB of benchmark annotation data is tracked in git, with 22 files stored twice | `tests/regression/evaluator/data/` |
| The ignore file names `.sushitrack_env.json`, not the `.sushitrack_env.json` the CLI writes, and has no rule for `build-asan/`, `.worktrees/`, `worktrees/` or `.superpowers/` | `.gitignore` |
| The ignore file hides every `*.pth`, `*.tar` and `*.db`, which would also hide a fixture added on purpose | `.gitignore` |
| The Docker ignore file names `Dockerfile.cli`, which does not exist, and does not exclude `*.egg-info` | `.dockerignore` |
| The submodule sections are still named `3rdparty/...` while their paths are `third_party/...` | `.gitmodules` |
| An environment variable and a docstring still say `3rdparty` | `cli/sushitrack_cli/config.py`, `cli/sushitrack_cli/services/evaluate.py` |
| No CI workflow exists | `.github/` |

## Layout

| Issue | Where |
| --- | --- |
| The root holds entries the layout does not name (`bindings/`, `Dockerfile`, `.dockerignore`, `environment.yml`, `sushitrack.json`) and lacks `.clang-format`, `.editorconfig` and `cmake/` | repository root |
| The root build file holds compiler flags, the Eigen lookup and target definitions, where the layout allows `project()`, options and `add_subdirectory` | `CMakeLists.txt` |
| The headers and sources are flat, without the `<area>` level, and four headers hold more than one primary symbol | `include/SushiTrack/`, `source/` |
| Test files are `PascalCase` and their headers end in `.h` | `tests/unit/`, `tests/integration/`, `tests/regression/` |
| Two tracker configs sit loose in the test root instead of a fixtures folder | `tests/bytetrack.json`, `tests/ocsort.json` |
| Python applications live inside the test tree | `tests/regression/evaluator/`, `tests/regression/inference/` |
| A second script folder sits inside the CLI package | `cli/misc/` |

## Documentation

| Issue | Where |
| --- | --- |
| A fix message and a dependency description cite sections of the old one-file manual; the sections are now `docs/reference/DATA_LAYOUT.md` and `docs/getting_started/INSTALL.md` | `cli/sushitrack_cli/services/checks.py`, `cli/sushistack.deps.toml` |
| The date, commit and machine of the recorded benchmark runs are not written down | `docs/reference/BENCHMARKS.md` |
| The pipeline, IMM and lifecycle descriptions have not been checked against the sources; the cited paths have | `docs/architecture/OVERVIEW.md` |
| The flag tables, parameter defaults and scenario counts were carried over from the old manual and not compared with the code one by one | `docs/reference/CLI.md`, `docs/reference/CONFIGURATION.md` |
