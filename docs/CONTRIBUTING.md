# Contributing to SushiTrack

This document defines the engineering standards for contributing to SushiTrack. It assumes familiarity with C++17, the CMake build system, and the unified developer CLI (`st` / `sushitrack`).

---

## Code Style

### Naming

| Entity | Convention | Example |
|---|---|---|
| Namespaces | PascalCase | `SushiTrack` |
| Classes / Structs | PascalCase | `Tracker`, `TrackerConfig` |
| Interfaces (pure virtual) | `I` prefix + PascalCase | `ICostCalculator`, `IKalmanFactory` |
| Methods | camelCase | `update()`, `getTracks()` |
| Member variables | snake_case with trailing `_` | `frame_id_`, `config_` |
| Local variables | snake_case | `track_remaining`, `dt` |
| C API symbols | `sushitrack_` prefix | `sushitrack_update`, `sushitrack_params_t` |
| Constants / Enumerators | PascalCase / `kPrefix` | `TrackState::Tracked`, `kMaxKfDtSteps` |

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

All public library code lives in the `SushiTrack` namespace. The C ABI layer in `include/SushiTrack/sushitrack_c.h` is `extern "C"` with no namespace.

### Comments

Write comments only when the **why** is non-obvious: a hidden constraint, a subtle invariant, or a workaround for a specific bug. Do not describe what the code does.

Forbidden:
- Decorative separator lines: `//---`, `// ===`, `// ****`, `// <--------->` and any variation.
- Multi-line comment blocks for sections inside a function or struct body.

A single `///<` trailing doc comment or a one-line `//` comment is the preferred form. If an explanation requires more than one line, keep comment blocks under 8 lines as enforced by repository checkers.

### Includes

Order: own header first, then standard library, then third-party. Separate groups with a blank line. Use angle brackets for library headers.

```cpp
#include <SushiTrack/tracklet.hpp>

#include <cstdint>
#include <memory>
#include <vector>

#include <Eigen/Dense>
```

---

## Documentation

All public declarations in `include/SushiTrack/` must carry Doxygen comments.

### File header

Every source and header file starts with the standard Apache-2.0 header and Doxygen file tag:

```cpp
/**
 * @file file_name.hpp
 * @brief Explains file responsibility starting with a verb.
 * @author Mustafa Garip
 */
```

Note: `@date` is forbidden; git records dates.

---

## Configuration Parameters

Every tunable parameter has **three representations** that must always be kept in sync:

| Layer | Location |
|---|---|
| C++ struct | `include/SushiTrack/tracker_config.hpp` |
| C ABI struct | `include/SushiTrack/sushitrack_c.h` → `sushitrack_params_t` |
| JSON schema | `sushitrack.json` |

When adding or removing a parameter:

1. Add the field to `TrackerConfig` with a default value and a `///<` doc comment.
2. Add the corresponding field to `sushitrack_params_t` in `sushitrack_c.h` with a `///<` comment.
3. Propagate the value in `source/sushitrack_c.cpp` (`sushitrack_get_default_params` and `sushitrack_create` mapping).
4. Add the key to `sushitrack.json` under the correct section (`basic` or `advanced.*`).
5. Handle the key in `source/config_loader.cpp`.
6. Mirror the field in `bindings/python/sushitrack.py` → `SushiTrackParams._fields_` in exact declaration order.
7. Add a unit test in `tests/unit/TestConfiguration.cpp` verifying that the JSON key loads correctly.

---

## C ABI Stability

`include/SushiTrack/sushitrack_c.h` and `sushitrack_params_t` form the public binary interface consumed by the Python binding and external C/C++ integrations.

- Never reorder or remove existing fields from `sushitrack_params_t` or `sushitrack_track_t`. New fields may only be appended at the end.
- Never change the type of an existing field.
- Never rename a `sushitrack_*` function or struct.
- The Python binding in `bindings/python/sushitrack.py` must be updated atomically with any ABI change.

---

## Python Binding

`bindings/python/sushitrack.py` is a ctypes mirror of `sushitrack_c.h`. It wraps the C ABI for easy consumption in Python scripts and evaluators.

Verify after any ABI change by running the test:

```bash
python -c "import sys; sys.path.insert(0, 'bindings/python'); import sushitrack; t = sushitrack.Tracker(config='auto'); t.close()"
```

---

## Tests

### Structure

```
tests/
  unit/         # Per-component tests. Fast, deterministic, no file I/O.
  integration/  # Multi-frame, end-to-end behavioral tests using the C API.
  regression/   # MOT benchmark evaluation against ground truth.
cli/tests/      # The `st` command itself, in Python.
```

The C++ suites test the library. `cli/tests/` tests the CLI that drives it, and runs
without building anything:

```bash
pip install -e "cli[test]"
python -m pytest cli/tests -q
```

No test there runs a real tool, prints through the shared console, or reads the
developer's own project: a test that does says something different on someone else's
machine.

### Building and running tests

Execute all operations exclusively through the repository CLI:

```bash
st build                       # library + test targets
st test                        # runs all test suites
st test --suite unit
st test --suite integration
st test --suite regression
```

`st` is installed with pipx from `cli/` (see `docs/README.md`'s Installation section); it is not run as a bare script.

Do not invoke raw `cmake`, `ninja`, or `ctest` directly.

---

## Commit Messages

Commit messages follow the Conventional Commits style:

```
feat: add SIoU distance strategy

GIoU diverged on near-zero-area boxes in dense pedestrian scenes.
SIoU penalizes direction misalignment, which reduces ID switches in
the low-confidence association stage.
```

Prefix conventions: `feat:`, `fix:`, `refactor:`, `test:`, `docs:`, `build:`.
