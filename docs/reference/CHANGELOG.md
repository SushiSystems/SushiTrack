# Changelog

## Unreleased

- 2026-09-24 — cli: Moved cmake, ctest and generator into a `[tool]` section loaded as `ToolSettings` through sushicore (`cli/sushitrack_cli/config.py`, `cli/config.toml`).
- 2026-09-23 — evaluator/inference: Replaced `loguru` with `sushicore`'s `Console`, matching every sibling Sushi* CLI (`tests/regression/evaluator/`, `tests/regression/inference/`, `environment.yml`, `Dockerfile`).

- 2026-09-22 — cli: Made `st doctor`'s compiler check and `st test`'s ctest `-C` flag OS-agnostic instead of Windows-only (`cli/sushitrack_cli/services/diag.py`, `cli/sushitrack_cli/services/tests.py`, `environment.yml`).

- 2026-09-22 — build: Hid symbols by default on every platform, exporting only `SUSHITRACK_API`; fixed a linker-dropped static initializer this uncovered in the IMM Kalman filter registration (`CMakeLists.txt`, `include/SushiTrack/sushitrack_c.h`, `tests/unit/CMakeLists.txt`, `tests/integration/CMakeLists.txt`, `tests/regression/CMakeLists.txt`, `tests/regression/TrackerRegistry.h`, `third_party/ocsort/CMakeLists.txt`).

- 2026-09-22 — build: Fixed `Dockerfile` and `README.md` referencing `cli/st.sh`/`cli/st.py`, which no longer exist since the CLI became a pip package (`Dockerfile`, `README.md`).

- 2026-09-22 — cli: Rebuilt the `st` command layer on Typer, matching every sibling Sushi* CLI (`cli/sushitrack_cli/cli.py`, removed `cli/sushitrack_cli/app.py`).

- 2026-09-22 — test: Added a Python suite for the `st` command, pinning what `proc` and `env` do (`cli/tests/`).

- 2026-09-22 — build: Took `sushicore` from PyPI instead of a sibling checkout (`cli/pyproject.toml`).

- 2026-09-21 — docs: Added the upstream licence and a `NOTICE` to `third_party/bytetrack` and `third_party/ocsort`, and listed every vendored dependency in `docs/README.md`.

- 2026-09-17 — cli: Rewired `st` onto `sushicore`'s console/config layer, adding `cli/pyproject.toml`, `cli/config.toml`, and `cli/sushistack.deps.toml`.

## v1.0.0 — 2026-09-17

- 2026-09-17 — core: Released initial Apache-2.0 open-source version of SushiTrack tracking engine.
- 2026-09-17 — capi: Established sushitrack_ C ABI layer with unified error codes.
- 2026-09-17 — cli: Implemented unified developer toolchain under st and sushitrack.
- 2026-09-17 — bindings: Delivered native ctypes Python binding with full feature parity.
- 2026-09-17 — layout: Reorganized repository layout into single-module Sushi Systems structure.
