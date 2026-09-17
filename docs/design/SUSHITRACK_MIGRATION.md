# SushiTrack Open-Source Migration and Rebranding

**Status:** Open — phase 1 of 4

This design document specifies the complete transition of the repository from
SushiTrack to SushiTrack, under the Apache-2.0 license, fully conforming to the
Sushi Systems engineering doctrine (`sushiskills`).

## 1. Context and Motivation

The project is entering the open-source ecosystem as a core tracking library within
Sushi Systems. All intellectual property rights have been transferred, and the codebase
is being released as an open-source project under Apache License Version 2.0.

The repository must reflect Sushi Systems standards:
- Clear separation of responsibilities (`SOLID`).
- Standard repository layout (`repository-layout`).
- Explicit Doxygen and Apache-2.0 headers (`source-comments`).
- Unified developer CLI (`st` / `sushitrack`).
- Standard checkers and test verification (`project-tools`, `testing`).

## 2. Global Constraints and Identity

- **Project Name:** SushiTrack
- **C++ Namespace:** `SushiTrack`
- **C API Prefix:** `sushitrack_`
- **Header Tree:** `<SushiTrack/*.hpp>`, `<SushiTrack/sushitrack_c.h>`
- **License:** Apache License Version 2.0 (`LICENSE`)
- **Copyright Holder:** `Copyright (c) 2026-present Mustafa Garip & Sushi Systems`
- **Organization Website:** `https://sushisystems.io`
- **Repository URL:** `https://github.com/SushiSystems/SushiTrack`
- **CLI Commands:** `sushitrack` and `st`
- **C++ Standard:** C++17 (`cpp-code-style`)
- **Language Policy:** English for all code, comments, documentation, and commits (`sushiskills`).

## 3. Architecture and Component Migration

### 3.1 Repository Layout Migration

The repository layout transforms to follow the Sushi Systems single-module structure:

```
SushiTrack/
├── .clang-format
├── .editorconfig
├── .gitattributes
├── .gitignore
├── AGENTS.md
├── CLAUDE.md
├── CMakeLists.txt
├── LICENSE
├── README.md
├── cli/
│   ├── config.json
│   ├── pyproject.toml
│   ├── sushitrack_cli/
│   ├── st.bat
│   ├── st.py
│   ├── st.sh
│   ├── sushitrack.bat
│   ├── sushitrack.py
│   └── sushitrack.sh
├── docs/
│   ├── CONTRIBUTING.md
│   ├── README.md
│   ├── architecture/
│   │   └── OVERVIEW.md
│   ├── design/
│   │   ├── README.md
│   │   ├── REMAINING_WORK.md
│   │   └── SUSHITRACK_MIGRATION.md
│   └── reference/
│       └── CHANGELOG.md
├── include/
│   └── SushiTrack/
│       ├── assignment_solvers.hpp
│       ├── config_loader.hpp
│       ├── distance_strategies.hpp
│       ├── imm_filter.hpp
│       ├── kalman_filter.hpp
│       ├── lapjv.hpp
│       ├── logger.hpp
│       ├── object.hpp
│       ├── rect.hpp
│       ├── sushitrack_c.h
│       ├── tracker.hpp
│       └── tracklet.hpp
├── source/
│   ├── assignment_solvers.cpp
│   ├── config_loader.cpp
│   ├── distance_strategies.cpp
│   ├── imm_filter.cpp
│   ├── kalman_filter.cpp
│   ├── lapjv.cpp
│   ├── logger.cpp
│   ├── object.cpp
│   ├── rect.cpp
│   ├── sushitrack_c.cpp
│   ├── tracker.cpp
│   └── tracklet.cpp
├── tests/
│   ├── benchmark/
│   ├── integration/
│   └── unit/
├── third_party/
│   ├── eigen/
│   ├── json/
│   └── lapjv/
└── tools/
    ├── documentation/
    │   ├── check_changelog.py
    │   ├── check_docs_layout.py
    │   └── check_source_comments.py
    └── layering/
        └── check_layering.py
```

### 3.2 Licensing and Source File Headers

Every C, C++, Python, and shell file receives a standardized Apache-2.0 header:

```cpp
/**************************************************************************/
/* tracker.hpp                                                            */
/**************************************************************************/
/*                          This file is part of:                         */
/*                               SushiTrack                               */
/*              https://github.com/SushiSystems/SushiTrack                */
/*                          https://sushisystems.io                       */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                          */
/**************************************************************************/

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
- Header: `include/SushiTrack/sushitrack_c.h`
- Implementation: `source/sushitrack_c.cpp`

### 3.5 Developer CLI (`cli/`)

- Package name: `sushitrack_cli`.
- Program names supported: `st` and `sushitrack`.
- Entry scripts:
  - `st.py`, `st.bat`, `st.sh`
  - `sushitrack.py`, `sushitrack.bat`, `sushitrack.sh`
- Configuration `cli/config.json` points to:
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
   - Place root `LICENSE` (Apache 2.0) and root `README.md`.
   - Move `src/` to `source/`, `3rdparty/` to `third_party/`, `.tools/` to `tools/`.
   - Organize `docs/` tree (`docs/reference/CHANGELOG.md`, `docs/architecture/OVERVIEW.md`, `docs/design/`).
   - Clean up stray test discovery files.
2. **Phase 2: Header Tree, C++ Core, and C API Modernization**
   - Move headers from `include/SushiTrack/` to `include/SushiTrack/`.
   - Rename headers and source files to snake_case (`tracker.hpp`, `sushitrack_c.h`, etc.).
   - Apply Apache-2.0 and Doxygen comment headers across all source files.
   - Update `CMakeLists.txt` for `project(sushitrack VERSION 1.0.0)` and `SUSHITRACK_API`.
   - Update `namespace SushiTrack` to `namespace SushiTrack` and classes to `SushiTrack::Tracker`.
   - Modernize C API to `sushitrack_` function prefixes.
3. **Phase 3: Developer CLI and Python Bindings**
   - Rename `sushitrack_cli` to `sushitrack_cli`.
   - Add entry points `st` and `sushitrack`.
   - Update `cli/config.json` paths and defaults.
   - Update `bindings/python` to module `sushitrack`.
4. **Phase 4: Test Suite Modernization and Full Verification**
   - Update test files in `tests/` to use `<SushiTrack/*.hpp>` and `namespace SushiTrack`.
   - Build complete project through `python cli/st.py build`.
   - Execute all unit, integration, and regression tests via `python cli/st.py test`.
   - Run standard checkers (`tools/documentation/check_source_comments.py`, etc.).
