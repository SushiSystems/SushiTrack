# SushiTrack Migration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Transform the repository from SushiTrack to SushiTrack under Apache License Version 2.0 with full Sushi Systems engineering doctrine compliance.

**Architecture:** Reorganize directory layout to standard Sushi single-module structure (`source/`, `include/SushiTrack/`, `third_party/`, `tools/`), rename C++ namespace to `SushiTrack` with `sushitrack_` C API, rebrand developer CLI to `st`/`sushitrack`, and establish standard checkers and test verification.

**Tech Stack:** C++17, CMake 3.14+, Python 3.11+, Eigen 3.4, nlohmann_json, LAPJV.

**Spec:** `docs/design/SUSHITRACK_MIGRATION.md`

## Global Constraints

- License: Apache License Version 2.0 (`LICENSE`).
- Copyright: `Copyright (c) 2026-present Mustafa Garip & Sushi Systems`.
- Organization Website: `https://sushisystems.io`.
- Repository URL: `https://github.com/SushiSystems/SushiTrack`.
- C++ Namespace: `SushiTrack` (no legacy `SushiTrack` shims).
- C API Function/Type Prefix: `sushitrack_` (e.g. `sushitrack_create_tracker`, `sushitrack_tracker_t`).
- Export Macro: `SUSHITRACK_API`.
- CLI Names: Both `st` and `sushitrack`.
- C++ Standard: C++17 only (`cpp-code-style`).
- Language: English for all code, comments, documentation, and commits (`sushiskills`).

---

### Task 1: Apache-2.0 License, Root Metadata, and Documentation Reorganization

**Files:**
- Create: `LICENSE`
- Create: `README.md`
- Create: `AGENTS.md`
- Create: `CLAUDE.md`
- Move: `docs/ARCHITECTURE.md` -> `docs/architecture/OVERVIEW.md`
- Move: `docs/CHANGELOG.md` -> `docs/reference/CHANGELOG.md`
- Delete: `LICENCE`
- Delete: `cmake_test_discovery_*.json`

**Interfaces:**
- Consumes: Spec requirements from `docs/design/SUSHITRACK_MIGRATION.md`.
- Produces: Root metadata and organized documentation tree.

- [ ] **Step 1: Create the root `LICENSE` file**
Add official Apache License Version 2.0 full text to `LICENSE`.

- [ ] **Step 2: Remove the empty `LICENCE` file and temporary test discovery JSON files**
Delete `LICENCE`, `cmake_test_discovery_54e12a2f29.json`, `cmake_test_discovery_a8348403ea.json`, `cmake_test_discovery_d595c1816c.json`.

- [ ] **Step 3: Create root `README.md`**
Write a concise root `README.md` in Sushi Systems standard format:
```markdown
# SushiTrack

Real-time multi-object tracking engine in C++17.

```bash
python cli/st.py build
python cli/st.py test
```

See [docs/README.md](docs/README.md) for documentation.
```

- [ ] **Step 4: Create `AGENTS.md` and `CLAUDE.md`**
List active skills and adherence to `sushiskills`.

- [ ] **Step 5: Organize `docs/` hierarchy**
Ensure `docs/reference/CHANGELOG.md`, `docs/architecture/OVERVIEW.md`, and `docs/CONTRIBUTING.md` exist at proper paths.

- [ ] **Step 6: Verify file layout**
Verify all root and docs files exist at expected locations.

---

### Task 2: Repository Layout Reorganization

**Files:**
- Move: `src/` -> `source/`
- Move: `3rdparty/` -> `third_party/`
- Move: `.tools/` -> `tools/`
- Move: `include/SushiTrack/` -> `include/SushiTrack/`
- Move: `include/SushiTrackC.h` -> `include/SushiTrack/sushitrack_c.h`
- Move: `source/SushiTrackC.cpp` -> `source/sushitrack_c.cpp`
- Rename: `sushitrack.json` -> `sushitrack.json`

**Interfaces:**
- Consumes: Existing C++ source and header files.
- Produces: Standard directory tree conforming to `repository-layout`.

- [ ] **Step 1: Move `src/` to `source/`**
Rename directory `src/` to `source/`.

- [ ] **Step 2: Move `3rdparty/` to `third_party/`**
Rename directory `3rdparty/` to `third_party/`.

- [ ] **Step 3: Move `.tools/` to `tools/`**
Rename directory `.tools/` to `tools/`.

- [ ] **Step 4: Move `include/SushiTrack/` to `include/SushiTrack/`**
Create `include/SushiTrack/` and move headers from `include/SushiTrack/`.

- [ ] **Step 5: Move and rename C API files**
Move `include/SushiTrackC.h` to `include/SushiTrack/sushitrack_c.h`.
Move `source/SushiTrackC.cpp` to `source/sushitrack_c.cpp`.

- [ ] **Step 6: Rename `sushitrack.json` to `sushitrack.json`**
Rename `sushitrack.json` at root to `sushitrack.json`.

---

### Task 3: C++ Namespace, Class Names, and Header Renaming

**Files:**
- Rename in `include/SushiTrack/`:
  - `SushiTrackTracker.hpp` -> `tracker.hpp`
  - `AssignmentSolvers.hpp` -> `assignment_solvers.hpp`
  - `ConfigLoader.hpp` -> `config_loader.hpp`
  - `DistanceStrategies.hpp` -> `distance_strategies.hpp`
  - `IMMFilter.hpp` -> `imm_filter.hpp`
  - `KalmanFilter.hpp` -> `kalman_filter.hpp`
  - `LAPJV.hpp` -> `lapjv.hpp`
  - `Logger.hpp` -> `logger.hpp`
  - `Object.hpp` -> `object.hpp`
  - `Rect.hpp` -> `rect.hpp`
  - `Tracklet.hpp` -> `tracklet.hpp`
- Rename in `source/`:
  - `SushiTrackTracker.cpp` -> `tracker.cpp`
  - Matching snake_case names for all source files.
- Modify: All files above to use `namespace SushiTrack`, class `SushiTrack::Tracker`, macro `SUSHITRACK_API`, and Apache-2.0 + Doxygen headers.

**Interfaces:**
- Consumes: Moved source files from Task 2.
- Produces: Clean C++17 library interface in `namespace SushiTrack`.

- [ ] **Step 1: Rename files in `include/SushiTrack/` and `source/` to snake_case**
Execute renaming for all headers and translation units.

- [ ] **Step 2: Update include guards and `#include` statements**
Change all `#include <SushiTrack/...>` to `#include <SushiTrack/...>`.

- [ ] **Step 3: Replace `namespace SushiTrack` with `namespace SushiTrack`**
Update namespace declarations across all headers and source files.

- [ ] **Step 4: Replace `SushiTrackTracker` with `Tracker` inside `SushiTrack`**
Rename tracker class to `SushiTrack::Tracker`.

- [ ] **Step 5: Replace export macro `SUSHITRACK_API` with `SUSHITRACK_API`**
Define `SUSHITRACK_API` in `sushitrack_export.hpp` or core header with proper platform visibility.

- [ ] **Step 6: Add standard Apache-2.0 and Doxygen comment headers**
Prepend 6-line license box with `Copyright (c) 2026-present Mustafa Garip & Sushi Systems` and `@file`, `@brief`, `@author Mustafa Garip` to all C++ files.

---

### Task 4: C API Implementation (`sushitrack_c.h` & `sushitrack_c.cpp`)

**Files:**
- Modify: `include/SushiTrack/sushitrack_c.h`
- Modify: `source/sushitrack_c.cpp`

**Interfaces:**
- Consumes: `SushiTrack::Tracker` from Task 3.
- Produces: C ABI exports with `sushitrack_` prefix: `sushitrack_create_tracker`, `sushitrack_destroy_tracker`, `sushitrack_update`, `sushitrack_get_tracklets`.

- [ ] **Step 1: Update `sushitrack_c.h` declarations**
Rename all `sushitrack_*` functions and types to `sushitrack_*`:
```c
typedef void* sushitrack_tracker_t;

SUSHITRACK_API sushitrack_tracker_t sushitrack_create_tracker(const char* config_path);
SUSHITRACK_API void sushitrack_destroy_tracker(sushitrack_tracker_t tracker);
SUSHITRACK_API int sushitrack_update(sushitrack_tracker_t tracker, ...);
```

- [ ] **Step 2: Update `sushitrack_c.cpp` implementation**
Update all function definitions to delegate to `SushiTrack::Tracker`.

- [ ] **Step 3: Verify C API header compiles standalone**
Ensure `sushitrack_c.h` has no C++ syntax outside `#ifdef __cplusplus`.

---

### Task 5: Root CMakeLists.txt and Build System Modernization

**Files:**
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: Renamed directories and files from Tasks 1-4.
- Produces: Working CMake build target `sushitrack`.

- [ ] **Step 1: Update project declaration**
Set `project(sushitrack VERSION 1.0.0 LANGUAGES CXX C)`.

- [ ] **Step 2: Update source and include directory paths**
Point sources to `source/*.cpp` and includes to `include/`.
Point third-party dependencies to `third_party/`.

- [ ] **Step 3: Update library target name**
Set `add_library(sushitrack SHARED ...)` or `STATIC`.
Set define `SUSHITRACK_EXPORTS` for symbol export.

- [ ] **Step 4: Update test and benchmark target links**
Link test targets to `sushitrack`.

---

### Task 6: Developer CLI (`sushitrack_cli`) and Entry Points

**Files:**
- Rename: `cli/sushitrack_cli/` -> `cli/sushitrack_cli/`
- Modify: `cli/sushitrack_cli/cli.py`
- Modify: `cli/config.json`
- Create: `cli/st.py`, `cli/st.bat`, `cli/st.sh`
- Create: `cli/sushitrack.py`, `cli/sushitrack.bat`, `cli/sushitrack.sh`
- Delete: `cli/ds.py`, `cli/ds.bat`, `cli/ds.sh`

**Interfaces:**
- Consumes: Reorganized CMake and source tree.
- Produces: Working developer CLI supporting `st` and `sushitrack`.

- [ ] **Step 1: Rename package directory to `cli/sushitrack_cli/`**
Rename `cli/sushitrack_cli/` to `cli/sushitrack_cli/`.

- [ ] **Step 2: Update `cli.py` for `st` and `sushitrack`**
Update program name, description, and module imports to `sushitrack_cli`.

- [ ] **Step 3: Update `cli/config.json`**
Update paths to `source`, `include/SushiTrack`, `third_party`, and default config `sushitrack.json`.

- [ ] **Step 4: Create launcher scripts**
Create `cli/st.py`, `cli/st.bat`, `cli/st.sh`, `cli/sushitrack.py`, `cli/sushitrack.bat`, `cli/sushitrack.sh`.

- [ ] **Step 5: Remove old `ds.*` launchers**
Delete `cli/ds.py`, `cli/ds.bat`, `cli/ds.sh`.

---

### Task 7: Python Bindings and Standard Tools Checkers

**Files:**
- Modify: `bindings/python/` (rebrand to `sushitrack`)
- Add: `tools/documentation/check_source_comments.py`
- Add: `tools/documentation/check_docs_layout.py`
- Add: `tools/documentation/check_changelog.py`
- Add: `tools/layering/check_layering.py`

**Interfaces:**
- Consumes: Rebranded C++ library.
- Produces: Rebranded python bindings and 4 standard repository checkers.

- [ ] **Step 1: Update Python bindings module and setup**
Rename binding extension and wrapper package to `sushitrack`.

- [ ] **Step 2: Install standard Sushi Systems checkers under `tools/`**
Ensure `check_source_comments.py`, `check_docs_layout.py`, `check_changelog.py`, and `check_layering.py` are present and executable.

---

### Task 8: Test Suite Migration and Full Verification

**Files:**
- Modify: `tests/unit/*.cpp`
- Modify: `tests/integration/*.cpp`
- Modify: `tests/benchmark/*.cpp`

**Interfaces:**
- Consumes: All previous tasks.
- Produces: Fully verified, compiling, green test suite.

- [ ] **Step 1: Update all test files to `<SushiTrack/*.hpp>` and `SushiTrack::Tracker`**
Update namespace and includes across all test files.

- [ ] **Step 2: Build project using the CLI**
Run `python cli/st.py build` (or `python cli/sushitrack.py build`).
Confirm exit code 0.

- [ ] **Step 3: Run full test suite using the CLI**
Run `python cli/st.py test`.
Verify all unit, integration, and regression tests pass.

- [ ] **Step 4: Run repository checkers**
Run `python tools/documentation/check_docs_layout.py` and other checkers.
Verify no violations.
