# Contributing to SushiTrack

Contributions from outside Sushi Systems are not accepted yet.

This document defines the engineering standards for contributing to SushiTrack. It assumes familiarity with C++17, the CMake build system, and the unified developer CLI (`st` / `sushitrack`).

## Code Style

### Naming

| Entity | Convention | Example |
|---|---|---|
| Namespaces | PascalCase | `SushiTrack` |
| Classes / Structs | PascalCase | `Tracker`, `TrackerConfig` |
| Interfaces (pure virtual) | `I` prefix + PascalCase | `CostCalculator`, `KalmanFactory` |
| Methods | camelCase | `update()`, `get_tracks()` |
| Member variables | snake_case with trailing `_` | `frame_id_`, `config_` |
| Local variables | snake_case | `track_remaining`, `dt` |
| C API symbols | `sushitrack_` prefix | `sushitrack_update`, `sushitrack_params_t` |
| Constants / Enumerators | PascalCase / `kPrefix` | `TrackState::Tracked`, `K_MAX_KF_DT_STEPS` |

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

Source: not compiled

### Namespaces

Use nested namespaces (C++17 syntax). Close each namespace with an end-of-line comment.

```cpp
namespace SushiTrack::Detail
{
    // ...
} // namespace SushiTrack::Detail
```

Source: not compiled

All public library code lives in the `SushiTrack` namespace. The C ABI layer in `include/SushiTrack/c_api/sushitrack_c.h` is `extern "C"` with no namespace.

### Comments

Comments follow the `source-comments` skill: a comment says what a thing does, in Doxygen
form. The reason for a design goes in a document under `docs/design/`, which the comment may
cite by path. `tools/documentation/check_source_comments.py` checks the shape; the findings it still reports
are in [Remaining work](design/REMAINING_WORK.md).

### Includes

Order: own header first, then standard library, then third-party. Separate groups with a blank line. Use angle brackets for library headers.

```cpp
#include <SushiTrack/tracking/tracklet.hpp>

#include <cstdint>
#include <memory>
#include <vector>

#include <Eigen/Dense>
```

Source: not compiled

## Documentation

Where a document goes, how it is named and how an agent's work is filed follow the
`documentation` skill; [the style guide](DOCUMENTATION_STYLE_GUIDE.md) lists the homes this
repository uses. A change carries its documentation in the same commit:

- the manual page or module README the change made false, corrected;
- one line under `## Unreleased` in [the changelog](reference/CHANGELOG.md), in the form
  `- <date> — <scope>: <Past-tense verb> <what changed> (<where>).`, at most 240 characters and
  five cited places, never the reason;
- the status line of its design document and the entry in
  [Remaining work](design/REMAINING_WORK.md), when the change opens or finishes planned work.

Before a commit that touches `docs/`, both of these print nothing:

```bash
python tools/documentation/check_docs_layout.py .
python tools/documentation/check_changelog.py .
```

### Source files

All public declarations in `include/SushiTrack/` must carry Doxygen comments.

### File header

Every source and header file starts with the license block of the `source-comments` skill,
written by `write_license_block.py` and never by hand, followed by the Doxygen file tag:

```cpp
/**
 * @file file_name.hpp
 * @brief Explains file responsibility starting with a verb.
 * @author Mustafa Garip
 */
```

Source: not compiled

Note: `@date` is forbidden; git records dates.

## Configuration Parameters

Every tunable parameter has **three representations** that must always be kept in sync:

| Layer | Location |
|---|---|
| C++ struct | `include/SushiTrack/tracking/tracker_config.hpp` |
| C ABI struct | `include/SushiTrack/c_api/sushitrack_c.h` → `sushitrack_params_t` |
| JSON schema | `sushitrack.json` |

When adding or removing a parameter:

1. Add the field to `TrackerConfig` with a default value and a `///<` doc comment.
2. Add the corresponding field to `sushitrack_params_t` in `sushitrack_c.h` with a `///<` comment.
3. Propagate the value in `source/c_api/sushitrack_c.cpp` (`sushitrack_get_default_params` and `sushitrack_create` mapping).
4. Add the key to `sushitrack.json` under the correct section (`basic` or `advanced.*`).
5. Handle the key in `source/configuration/config_loader.cpp`.
6. Mirror the field in `bindings/python/sushitrack.py` → `SushiTrackParams._fields_` in exact declaration order.
7. Add a test in `tests/configuration/test_config_loader.cpp` verifying that the JSON key loads correctly.

## C ABI Stability

`include/SushiTrack/c_api/sushitrack_c.h` and `sushitrack_params_t` form the public binary interface consumed by the Python binding and external C/C++ integrations.

- Never reorder or remove existing fields from `sushitrack_params_t` or `sushitrack_track_t`. New fields may only be appended at the end.
- Never change the type of an existing field.
- Never rename a `sushitrack_*` function or struct.
- The Python binding in `bindings/python/sushitrack.py` must be updated atomically with any ABI change.

## Python Binding

`bindings/python/sushitrack.py` is a ctypes mirror of `sushitrack_c.h`. Its constructor
arguments and the order in which it looks for the shared library are in
[`bindings/python/README.md`](../bindings/python/README.md).

Verify after any ABI change by running the test:

```bash
python -c "import sys; sys.path.insert(0, 'bindings/python'); import sushitrack; t = sushitrack.Tracker(config='auto'); t.close()"
```

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
st build                       # library + test programs
st test                        # small and medium tests
st test --label small
st eval                        # the MOT benchmark
```

`st` is installed with pipx from `cli/` (see [Installing](getting_started/INSTALL.md)); it is not run as a bare script.

Do not invoke raw `cmake`, `ninja`, or `ctest` directly.

## Commit Messages

Commit messages follow the Conventional Commits style, with the changed area as the scope. The
changelog entry of the same change uses that scope.

```
feat(association): add SIoU distance strategy

GIoU diverged on near-zero-area boxes in dense pedestrian scenes.
SIoU penalizes direction misalignment, which reduces ID switches in
the low-confidence association stage.
```

Types: `feat`, `fix`, `refactor`, `test`, `docs`, `build`, `chore`. Scopes used so far include `cli`, `build`, `tracker`, `bindings` and `regression`.
