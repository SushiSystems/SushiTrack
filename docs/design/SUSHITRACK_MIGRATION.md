# SushiTrack Migration and Rebranding

**Status:** Open — P1 has two items left, P2 to P4 are built

## Problem

The repository came from a project that carried another name, another licence and a layout of its
own. It has to read as a Sushi Systems repository: one identity, the PolyForm Noncommercial
License 1.0.0, and the standards of `sushiskills`.

## Decision

The project is SushiTrack, a core tracking library within Sushi Systems. All intellectual
property rights have been transferred. The code is source-available, free for non-commercial use
under the PolyForm Noncommercial License 1.0.0; commercial use needs a licence from Sushi
Systems, see `COMMERCIAL.md`. The owner chose a clean break: no legacy shims keep an old name
alive.

| Item | Value |
| --- | --- |
| Project name | SushiTrack |
| C++ namespace | `SushiTrack` |
| C API prefix | `sushitrack_` |
| Header tree | `<SushiTrack/*.hpp>`, `<SushiTrack/c_api/sushitrack_c.h>` |
| Licence | PolyForm Noncommercial License 1.0.0 (`LICENSE`), commercial terms in `COMMERCIAL.md` |
| Copyright holder | `Copyright (c) 2026 Sushi Systems` |
| Organisation website | `https://sushisystems.io` |
| Repository URL | `https://github.com/SushiSystems/SushiTrack` |
| CLI commands | `sushitrack` and `st` |
| C++ standard | C++17 (`cpp-code-style`) |
| Language | English for code, comments, documentation and commits (`sushiskills`) |

## Design

The repository follows `repository-layout`, `source-comments`, `project-tools` and `testing`
under one developer CLI.

### Repository layout

The Sushi Systems single-module structure: public headers in `include/SushiTrack/`, sources in
`source/`, vendored code in `third_party/`, checkers in `tools/`, tests in `tests/unit`,
`tests/integration` and `tests/regression`, the CLI package in `cli/` and the binding in
`bindings/python/`. The tree as it landed is drawn in
[Project structure](../architecture/PROJECT_STRUCTURE.md). The root also holds `.clang-format`
and `.editorconfig`.

### Licensing and source file headers

Every C, C++, Python, shell and CMake file opens with the license block of the `source-comments`
skill. `write_license_block.py` in SushiSkills writes it; `NOTICE.md` records the exact command,
which also adds the upstream lines of the ported files.

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

### C++ core and namespaces

1. The engine class is `SushiTrack::Tracker`. The core types (`Tracklet`, `Rect`, `Object`,
   `KalmanFilter`, `IMMFilter`) live in `namespace SushiTrack`.
2. The export macro is `SUSHITRACK_API`.
   - Windows: `__declspec(dllexport)` / `__declspec(dllimport)`.
   - Linux: `__attribute__((visibility("default")))`.
3. The configuration file is `sushitrack.json`.

### C API (`sushitrack_c.h`)

All C API declarations use the full prefix `sushitrack_`:

- `sushitrack_tracker_t` opaque pointer handle.
- `sushitrack_create_tracker(const char* config_path)`
- `sushitrack_destroy_tracker(sushitrack_tracker_t tracker)`
- `sushitrack_update(sushitrack_tracker_t tracker, ...)`
- `sushitrack_get_tracklets(sushitrack_tracker_t tracker, ...)`
- Header: `include/SushiTrack/c_api/sushitrack_c.h`
- Implementation: `source/c_api/sushitrack_c.cpp`

### Developer CLI (`cli/`)

- Package name: `sushitrack_cli`.
- Program names: `st` and `sushitrack`, the console scripts declared in `cli/pyproject.toml`.
- `cli/config.toml` points to `source/` for the source root, `include/SushiTrack` for the
  include directory, `third_party/` for dependencies and `build/` for the build tree.
- Core commands: `build`, `test`, `run`, `clean`, `deploy`, `status`, `doctor`.

### Python bindings (`bindings/python`)

- Package name: `sushitrack`.
- Exposes `sushitrack.Tracker` wrapping the C++ core.

## Phases

| Id | Phase | State |
| --- | --- | --- |
| P1 | Licensing and repository structure: root licence files, `source/`, `third_party/`, `tools/`, the `docs/` tree, stray test discovery files | building |
| P2 | Header tree, C++ core and C API: `include/SushiTrack/`, snake_case files, licence blocks, `sushitrack_` prefixes | built |
| P3 | Developer CLI and Python bindings: `sushitrack_cli`, the `st` and `sushitrack` entry points, `sushitrack` module | built |
| P4 | Test suite and full verification: tests on `<SushiTrack/*.hpp>`, `st build`, `st test`, the checkers | built |

## Open questions

P1 leaves two items, each also in [the backlog](REMAINING_WORK.md):

- `.clang-format` and `.editorconfig` do not exist at the root.
- The test discovery files are still written to the root: `gtest_discover_tests` runs with
  `WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}` in `tests/unit/CMakeLists.txt`,
  `tests/integration/CMakeLists.txt` and `tests/regression/CMakeLists.txt`.
