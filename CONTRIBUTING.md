# Contributing to SushiTrack

This document defines the engineering standards for contributing to SushiTrack. It is written for engineers at Sushi Systems and assumes familiarity with C++17, the CMake build system, and the project structure.

---

## Code Style

### Naming

| Entity | Convention | Example |
|---|---|---|
| Namespaces | PascalCase | `SushiTrack` |
| Classes / Structs | PascalCase | `SushiTrackTracker`, `TrackerConfig` |
| Interfaces (pure virtual) | `I` prefix + PascalCase | `ICostCalculator`, `IKalmanFactory` |
| Methods | camelCase | `update()`, `getTracks()` |
| Member variables | snake_case with trailing `_` | `frame_id_`, `config_` |
| Local variables | snake_case | `track_remaining`, `dt` |
| C API symbols | `SushiTrack_` prefix | `SushiTrack_Update`, `SushiTrack_Params` |
| Constants / Enumerators | PascalCase | `TrackState::Tracked` |

### Braces

Allman style everywhere: opening brace on its own line.

```cpp
void Foo::bar()
{
    if (condition)
    {
        doSomething();
    }
}
```

### Namespaces

Use nested namespaces (C++17 syntax). Close each namespace with an end-of-line comment.

```cpp
namespace SushiTrack::Detail
{
    // ...
} // namespace SushiTrack::Detail
```

All public library code lives in the `SushiTrack` namespace. The C ABI layer in `SushiTrackC.h` is `extern "C"` with no namespace.

### Includes

Order: own header first, then standard library, then third-party. Separate groups with a blank line. Use angle brackets only for system and third-party headers; use quotes for project headers.

```cpp
#include "SushiTrack/Tracklet.h"

#include <cstdint>
#include <memory>
#include <vector>

#include <Eigen/Dense>
```

---

## Documentation

All public declarations in `include/` must carry Doxygen comments. Private methods may be documented when the logic is non-obvious.

### File header

Every file starts with the standard block (copy from an existing header and update the fields):

```cpp
/**
 * @file      FileName.h
 * @brief     One sentence describing the file's responsibility.
 * @author    Author Name (email@sushisystems.io)
 * @date      DD.MM.YYYY
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */
```

### Class and struct

```cpp
/**
 * @brief One sentence describing the class's role.
 *
 * Optional second paragraph for non-obvious invariants or design decisions.
 * Do not describe parameters or internal layout here.
 */
class MyClass { ... };
```

### Method

```cpp
/**
 * @brief One sentence in plain English describing what the method does.
 *
 * @param name_of_param Description in plain English. State units, range, or ownership when relevant.
 * @return What the return value represents.
 * @throws std::invalid_argument If the parameter violates a documented precondition.
 */
```

Rules:
- The `@brief` line is a single sentence ending with a period.
- `@param`, `@return`, and `@throws` descriptions are plain English, technical and objective. No implementation details.
- Omit `@param` for self-evident parameters (`void`, single `bool` flags already named `enable_*`).
- Do not repeat the function signature in prose.

### Member variables

Use `///<` trailing doc comments on struct and class fields:

```cpp
float match_thresh = 0.7f; ///< Maximum cost limit allowed for the primary association step.
```

---

## Configuration Parameters

Every tunable parameter has **three representations** that must always be kept in sync:

| Layer | Location |
|---|---|
| C++ struct | `include/SushiTrack/TrackerConfig.h` |
| C ABI struct | `include/SushiTrackC.h` → `SushiTrack_Params` |
| JSON schema | `sushitrack.json` |

When adding or removing a parameter:

1. Add the field to `TrackerConfig` with a default value and a `///<` doc comment.
2. Add the corresponding field to `SushiTrack_Params` in `SushiTrackC.h` with a `/** @brief */` comment.
3. Propagate the value in `src/SushiTrackC.cpp` (the `SushiTrack_GetDefaultParams` function and the `SushiTrack_Create` → `TrackerConfig` mapping).
4. Add the key to `sushitrack.json` under the correct section (`basic`, `advanced.*`, or `extended`).
5. Handle the key in `src/ConfigLoader.cpp`.
6. Mirror the field in `bindings/python/sushitrack.py` → `SushiTrackParams._fields_` in exact declaration order (field order determines memory layout).
7. Add a unit test in `tests/unit/TestConfiguration.cpp` verifying that the JSON key loads correctly.

Field order in `SushiTrackParams._fields_` must mirror the C struct declaration exactly. A mismatch silently corrupts all fields that follow.

---

## C ABI Stability

`SushiTrackC.h` and `SushiTrack_Params` form the public binary interface consumed by the Python binding and any external C integrations.

- Never reorder or remove existing fields from `SushiTrack_Params` or `SushiTrack_Track`. New fields may only be appended at the end.
- Never change the type of an existing field.
- Never rename a `SushiTrack_*` function or struct.
- The Python binding in `bindings/python/sushitrack.py` must be updated atomically with any ABI change.

---

## Python Binding

`bindings/python/sushitrack.py` is a ctypes mirror of `SushiTrackC.h`. It contains no logic of its own; it only wraps the C ABI.

When the C ABI changes, update the binding in the same commit:

- `SushiTrackObject._fields_` → mirrors `SushiTrack_Object`
- `SushiTrackTrack._fields_` → mirrors `SushiTrack_Track`
- `SushiTrackParams._fields_` → mirrors `SushiTrack_Params`
- `Status` → mirrors `SushiTrack_Status`
- `TrackState` / `KinematicState` → mirror the corresponding enums in `SushiTrackTypes.h`
- `_bind()` → declares argtypes/restype for every entry point in `SushiTrackC.h`

Verify after any ABI change by running the Python smoke test:

```
python bindings/python/sushitrack.py
```

---

## Adding a New Feature

Features in SushiTrack are typically controlled by a flag in `TrackerConfig`. The full checklist for a flag-gated feature:

- [ ] `TrackerConfig` field with default and doc comment
- [ ] `SushiTrack_Params` field in the C header
- [ ] `SushiTrack_GetDefaultParams` and `SushiTrack_Create` mapping in `src/SushiTrackC.cpp`
- [ ] `sushitrack.json` key under the appropriate section
- [ ] `ConfigLoader.cpp` handler
- [ ] `SushiTrackParams._fields_` entry in `bindings/python/sushitrack.py`
- [ ] `TestConfiguration.cpp` test for the new JSON key
- [ ] Unit tests for the feature logic in `tests/unit/`
- [ ] Integration test scenario in `tests/integration/` if the feature affects multi-frame behavior
- [ ] `README.md` section describing the parameter (name, type, range, default, effect)

A pull request that adds a flag without updating all layers above will not be merged.

---

## Tests

### Structure

```
tests/
  unit/         # Per-component tests. Fast, deterministic, no file I/O.
  integration/  # Multi-frame, end-to-end behavioral tests using the C API.
  regression/   # MOT benchmark evaluation against ground truth.
```

### Build flags

```bash
# Unit tests only
cmake -DBUILD_UNIT_TEST=ON ..

# Integration tests only
cmake -DBUILD_UNIT_TEST=ON -DBUILD_INTEGRATION_TEST=ON ..

# Run via CTest
ctest -R unit --output-on-failure
ctest -R integration --output-on-failure
```

### Expectations

- Unit tests cover individual classes and methods in isolation. Mock interfaces where necessary.
- Integration tests operate through the C API (`SushiTrack_Create` / `SushiTrack_Update` / `SushiTrack_Destroy`). They must not access internal C++ classes directly.
- Every new parameter must have a configuration round-trip test in `TestConfiguration.cpp`.
- Every new behavioral path must have a corresponding integration test.
- Tests must be deterministic. Do not use real-time clocks or file system state.

---

## README

`README.md` is the reference for users of the built library. Update it when:

- A new parameter is added to `TrackerConfig` / `SushiTrack_Params` (add it to the parameter table).
- A new enum value is added to `TrackState`, `KinematicState`, or `SushiTrack_Status`.
- A new public C API function is introduced.
- Build prerequisites or build steps change.
- The JSON configuration schema changes.

---

## Interfaces

The library uses interface classes (pure virtual, `I` prefix) to decouple algorithm components from their implementations. New algorithm variants (cost metrics, assignment solvers, Kalman filters, timing policies) are introduced by implementing the relevant interface, not by adding branches to existing classes.

| Interface | Implemented by |
|---|---|
| `ICostCalculator` | Distance and similarity strategies in `DistanceStrategies.h` |
| `IAssignmentSolver` | LAPJV and legacy solvers in `AssignmentSolvers.h` |
| `IKalmanFactory` / `IStatePredictor` | `KalmanFilter`, `NSAKalmanFilter` |
| `ITimingPolicy` | Frame-based and wall-clock policies in `TimingPolicies.h` |

When adding a new interface implementation, register it in `KalmanFilterRegistry.h` or the factory function in `SushiTrackC.cpp` as appropriate.

---

## Commit Messages

Use the imperative form: `add`, `fix`, `remove`, `refactor`. One short subject line. No period at the end. A body paragraph is required only when the change is non-obvious or has a subtle invariant the reader must know.

```
feat: add SIoU distance strategy

GIoU diverged on near-zero-area boxes in dense pedestrian scenes.
SIoU penalizes direction misalignment, which reduces ID switches in
the low-confidence association stage.
```

Prefix conventions: `feat:`, `fix:`, `refactor:`, `test:`, `docs:`, `build:`.
