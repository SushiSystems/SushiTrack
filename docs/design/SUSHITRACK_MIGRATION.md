# SushiTrack Migration and Rebranding

**Status:** Open — phases 2 to 4 landed; phase 1 leaves the two items of section 5

This design document specifies the complete transition of the repository from
SushiTrack to SushiTrack, under the PolyForm Noncommercial License 1.0.0, fully conforming to the
Sushi Systems engineering doctrine (`sushiskills`).

## 1. Context and Motivation

The project becomes a core tracking library within Sushi Systems. All intellectual property
rights have been transferred. The codebase is source-available, free for non-commercial use
under the PolyForm Noncommercial License 1.0.0; commercial use needs a licence from Sushi
Systems, see `COMMERCIAL.md`.

The repository must reflect Sushi Systems standards:
- Clear separation of responsibilities (`SOLID`).
- Standard repository layout (`repository-layout`).
- Doxygen comments and the license block of `source-comments`.
- Unified developer CLI (`st` / `sushitrack`).
- Standard checkers and test verification (`project-tools`, `testing`).

## 2. Global Constraints and Identity

- **Project Name:** SushiTrack
- **C++ Namespace:** `SushiTrack`
- **C API Prefix:** `sushitrack_`
- **Header Tree:** `<SushiTrack/*.hpp>`, `<SushiTrack/c_api/sushitrack_c.h>`
- **License:** PolyForm Noncommercial License 1.0.0 (`LICENSE`), commercial terms in `COMMERCIAL.md`
- **Copyright Holder:** `Copyright (c) 2026 Sushi Systems`
- **Organization Website:** `https://sushisystems.io`
- **Repository URL:** `https://github.com/SushiSystems/SushiTrack`
- **CLI Commands:** `sushitrack` and `st`
- **C++ Standard:** C++17 (`cpp-code-style`)
- **Language Policy:** English for all code, comments, documentation, and commits (`sushiskills`).

## 3. Architecture and Component Migration

### 3.1 Repository Layout Migration

The repository follows the Sushi Systems single-module structure: public headers in
`include/SushiTrack/`, sources in `source/`, vendored code in `third_party/`, checkers in
`tools/`, tests in `tests/unit`, `tests/integration` and `tests/regression`, the CLI package in
`cli/` and the binding in `bindings/python/`. The tree as it landed is drawn in
[Project structure](../architecture/PROJECT_STRUCTURE.md).

The target root also holds `.clang-format` and `.editorconfig`.

### 3.2 Licensing and Source File Headers

Every C, C++, Python, shell and CMake file opens with the license block of the
`source-comments` skill. `write_license_block.py` in SushiSkills writes it; `NOTICE.md` records
the exact command, which also adds the upstream lines of the ported files.

```cpp
/****************************************************************/
/* tracker.hpp                                                  */
/* SushiTrack - https://github.com/SushiSystems/SushiTrack      */
/* Copyright (c) 2026 Sushi Systems                             */
/* Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.    */
/* Commercial use requires a licence from Sushi Systems.        */
/****************************************************************/

/**
 * @file tracker.hpp
 * @brief Multi-object tracking engine managing tracklets and state estimation.
 * @author Mustafa Garip
 */
```

### 3.3 C++ Core and Namespaces

1. **Namespace:** Replaced `namespace SushiTrack` with `namespace SushiTrack`.
2. **Class Names:**
   - `SushiTrackTracker` is renamed to `SushiTrack::Tracker` (or aliased if needed, but per owner decision, clean break with no legacy shims).
   - Core types (`Tracklet`, `Rect`, `Object`, `KalmanFilter`, `IMMFilter`) live in `namespace SushiTrack`.
3. **Export Macro:** `SUSHITRACK_API` replaced with `SUSHITRACK_API`.
   - Windows: `__declspec(dllexport)` / `__declspec(dllimport)`.
   - Linux: `__attribute__((visibility("default")))`.
4. **Configuration File:** `sushitrack.json` renamed to `sushitrack.json`.

### 3.4 C API (`sushitrack_c.h`)

All C API declarations use the full prefix `sushitrack_`:
- `sushitrack_tracker_t` opaque pointer handle.
- `sushitrack_create_tracker(const char* config_path)`
- `sushitrack_destroy_tracker(sushitrack_tracker_t tracker)`
- `sushitrack_update(sushitrack_tracker_t tracker, ...)`
- `sushitrack_get_tracklets(sushitrack_tracker_t tracker, ...)`
- Header: `include/SushiTrack/c_api/sushitrack_c.h`
- Implementation: `source/c_api/sushitrack_c.cpp`

### 3.5 Developer CLI (`cli/`)

- Package name: `sushitrack_cli`.
- Program names supported: `st` and `sushitrack`.
- Entry points: the console scripts `st` and `sushitrack`, declared in `cli/pyproject.toml`.
- Configuration `cli/config.toml` points to:
  - `source/` for source root.
  - `include/SushiTrack` for include directory.
  - `third_party/` for dependencies.
  - `build/` for build tree.
- Core commands: `build`, `test`, `run`, `clean`, `deploy`, `status`, `doctor`.

### 3.6 Python Bindings (`bindings/python`)

- Package name: `sushitrack`.
- Exposes `sushitrack.Tracker` wrapping the C++ core.
- `pyproject.toml` / `setup.py` updated to package name `sushitrack`.

## 4. Execution Phases

1. **Phase 1: Licensing and Repository Structure Alignment**
   - Place root `LICENSE`, `COMMERCIAL.md`, `NOTICE.md` and root `README.md`.
   - Move `src/` to `source/`, `3rdparty/` to `third_party/`, `.tools/` to `tools/`.
   - Organize `docs/` tree (`docs/reference/CHANGELOG.md`, `docs/architecture/OVERVIEW.md`, `docs/design/`).
   - Clean up stray test discovery files.
2. **Phase 2: Header Tree, C++ Core, and C API Modernization**
   - Move headers from `include/SushiTrack/` to `include/SushiTrack/`.
   - Rename headers and source files to snake_case (`tracker.hpp`, `sushitrack_c.h`, etc.).
   - Apply the license block and Doxygen comment headers across all source files.
   - Update `CMakeLists.txt` for `project(sushitrack VERSION 1.0.0)` and `SUSHITRACK_API`.
   - Update `namespace SushiTrack` to `namespace SushiTrack` and classes to `SushiTrack::Tracker`.
   - Modernize C API to `sushitrack_` function prefixes.
3. **Phase 3: Developer CLI and Python Bindings**
   - Rename `sushitrack_cli` to `sushitrack_cli`.
   - Add entry points `st` and `sushitrack`.
   - Update `cli/config.toml` paths and defaults.
   - Update `bindings/python` to module `sushitrack`.
4. **Phase 4: Test Suite Modernization and Full Verification**
   - Update test files in `tests/` to use `<SushiTrack/*.hpp>` and `namespace SushiTrack`.
   - Build complete project through `st build`.
   - Execute all unit, integration, and regression tests via `st test`.
   - Run standard checkers (`tools/documentation/check_source_comments.py`, etc.).

## 5. What is left

Phases 2, 3 and 4 landed. Phase 1 leaves two items, each also in
[the backlog](REMAINING_WORK.md):

- `.clang-format` and `.editorconfig` do not exist at the root.
- The test discovery files are still written to the root: `gtest_discover_tests` runs with
  `WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}` in `tests/unit/CMakeLists.txt`,
  `tests/integration/CMakeLists.txt` and `tests/regression/CMakeLists.txt`.
