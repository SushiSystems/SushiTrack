# Changelog

## Unreleased

- 2026-10-05 — core: Added catch-all handlers to the C entry points that log a non-std exception and return the call's failure value (`source/sushitrack_c.cpp`, `source/config_loader.cpp`).
- 2026-10-05 — core: Replaced the per-pair gating log lines with one line per cost-matrix call (`source/distance_strategies.cpp`).
- 2026-10-05 — core: Passed exception text and the unregistered Kalman type to the logger as arguments of a literal format (`source/sushitrack_c.cpp`, `include/SushiTrack/kalman_filter_registry.hpp`).
- 2026-10-05 — docs: Shortened eleven comment paragraphs and docstrings and moved their reasoning into the manual (`docs/architecture/CLI_PACKAGE.md`, `docs/guides/TESTING.md`, `docs/guides/DEPLOY_PACKAGES.md`, `docs/guides/INFERENCE.md`).
- 2026-10-05 — docs: Split the manual into pages under an index, added the required documents and archived the finished Typer and migration records (`docs/README.md`, `docs/reference/`, `docs/design/`, `docs/archive/`, `tools/`).
- 2026-10-05 — cli: Renamed st docker to st container; the old word still runs, hidden, until 2027-03-25 (`cli/sushitrack_cli/cli.py`, `cli/sushitrack_cli/aliases.py`).
- 2026-10-05 — cli: Added --version and --describe and took config, env and the entry point from sushicore (`cli/sushitrack_cli/cli.py`, `cli/sushitrack_cli/services/diag.py`, `cli/sushitrack_cli/__main__.py`).
- 2026-10-05 — cli: Changed st eval with no matching sequence and st clean with a surviving directory to exit 1 (`cli/sushitrack_cli/services/label.py`, `cli/sushitrack_cli/services/build.py`).
- 2026-10-05 — cli: Reported an unknown build type or tracker and a malformed config.toml as one error line (`cli/sushitrack_cli/errors.py`, `cli/sushitrack_cli/services/build.py`, `cli/sushitrack_cli/services/label.py`).
- 2026-10-05 — cli: Repaired the C and Python examples st deploy writes and eight cut comments (`cli/sushitrack_cli/services/deploy.py`).
- 2026-10-05 — cli: Listed setup, doctor, link and unlink under the Diagnostics help panel (`cli/sushitrack_cli/cli.py`).
- 2026-10-05 — docs: Corrected the sushicore install step and documented st run, deploy, the diagnostics, the root options and the exit codes (`docs/README.md`).
- 2026-10-05 — licence: Replaced Apache-2.0 with PolyForm Noncommercial 1.0.0 and named the ported files and the benchmark data (`LICENSE`, `COMMERCIAL.md`, `NOTICE.md`, `third_party/licenses/`, `README.md`).
- 2026-10-04 — cli: Required sushicore 0.7.0, whose setup takes --toolchain and --no-gpu (`cli/pyproject.toml`).
- 2026-09-24 — cli: Removed the unused private copies (`cli/sushitrack_cli/proc.py`, `cli/sushitrack_cli/services/discovery.py`, `cli/tests/test_proc.py`).
- 2026-09-24 — cli: Replaced st doctor with sushicore's shared setup, doctor, link and unlink commands plus SushiTrack's own checks (`cli/sushitrack_cli/cli.py`, `cli/sushitrack_cli/services/checks.py`).
- 2026-09-24 — cli: Moved st run, label and deploy onto the shared runner and artifact locator (`cli/sushitrack_cli/services/runner.py`, `cli/sushitrack_cli/services/label.py`, `cli/sushitrack_cli/services/deploy.py`).
- 2026-09-24 — cli: Moved st status and st env onto the shared runner, artifact locator and cmake cache reader (`cli/sushitrack_cli/services/diag.py`).
- 2026-09-24 — cli: Moved st docker, infer and eval onto the shared sushicore runner (`cli/sushitrack_cli/services/docker.py`, `cli/sushitrack_cli/services/infer.py`, `cli/sushitrack_cli/services/evaluate.py`).
- 2026-09-24 — cli: Moved st test's ctest invocation onto sushicore's CMakeDriver.ctest_run (`cli/sushitrack_cli/services/tests.py`).
- 2026-09-24 — cli: Moved st build's cmake configure and compile onto sushicore's CMakeDriver (`cli/sushitrack_cli/services/build.py`).
- 2026-09-24 — cli: Rebuilt the vcvars build environment on sushicore.build_env with a keyed cache (`cli/sushitrack_cli/env.py`).
- 2026-09-24 — cli: Added a build-tree artifact locator over sushicore's ExecutableIndex (`cli/sushitrack_cli/services/artifacts.py`).
- 2026-09-24 — cli: Added a shared sushicore process runner for st services (`cli/sushitrack_cli/process.py`).
- 2026-09-24 — cli: Moved cmake, ctest and generator into `[tool]`, loaded as `ToolSettings` through sushicore and layered over the linked workspace's tool paths (`cli/sushitrack_cli/config.py`, `cli/config.toml`).
- 2026-09-23 — evaluator: Replaced loguru with sushicore's Console, matching every sibling Sushi CLI (`tests/regression/evaluator/`, `environment.yml`, `Dockerfile`).
- 2026-09-23 — inference: Replaced loguru with sushicore's Console, matching every sibling Sushi CLI (`tests/regression/inference/`).
- 2026-09-22 — cli: Made st doctor's compiler check and st test's ctest -C flag OS-agnostic instead of Windows-only (`cli/sushitrack_cli/services/diag.py`, `cli/sushitrack_cli/services/tests.py`, `environment.yml`).
- 2026-09-22 — build: Hid symbols by default on every platform, exporting only `SUSHITRACK_API` (`CMakeLists.txt`, `include/SushiTrack/sushitrack_c.h`, `tests/regression/TrackerRegistry.h`, `third_party/ocsort/CMakeLists.txt`).
- 2026-09-22 — build: Fixed a linker-dropped static initializer in the IMM Kalman filter registration (`CMakeLists.txt`, `tests/unit/CMakeLists.txt`, `tests/integration/CMakeLists.txt`, `tests/regression/CMakeLists.txt`).
- 2026-09-22 — build: Fixed the references to `cli/st.sh` and `cli/st.py`, which no longer exist since the CLI became a pip package (`Dockerfile`, `README.md`).
- 2026-09-22 — cli: Rebuilt the st command layer on Typer, matching every sibling Sushi CLI, and removed `cli/sushitrack_cli/app.py` (`cli/sushitrack_cli/cli.py`).
- 2026-09-22 — test: Added a Python suite for the st command, pinning what proc and env do (`cli/tests/`).
- 2026-09-22 — build: Took sushicore from PyPI instead of a sibling checkout (`cli/pyproject.toml`).
- 2026-09-21 — docs: Added the upstream licence and a NOTICE to the vendored trackers and listed every vendored dependency (`third_party/bytetrack`, `third_party/ocsort`, `docs/README.md`).
- 2026-09-17 — cli: Rewired st onto sushicore's console and config layer and added its package files (`cli/pyproject.toml`, `cli/config.toml`, `cli/sushistack.deps.toml`).

## v1.0.0 — 2026-09-17

- 2026-09-17 — core: Released initial Apache-2.0 open-source version of SushiTrack tracking engine (`LICENSE`).
- 2026-09-17 — capi: Established sushitrack_ C ABI layer with unified error codes (`include/SushiTrack/sushitrack_c.h`, `source/sushitrack_c.cpp`).
- 2026-09-17 — cli: Implemented unified developer toolchain under st and sushitrack (`cli/`).
- 2026-09-17 — bindings: Delivered native ctypes Python binding with full feature parity (`bindings/python/sushitrack.py`).
- 2026-09-17 — layout: Reorganized repository layout into single-module Sushi Systems structure (`include/SushiTrack/`, `source/`, `third_party/`, `tools/`).
