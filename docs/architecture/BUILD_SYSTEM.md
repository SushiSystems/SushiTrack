# Build system

How the CMake tree is laid out and why its targets are shaped the way they are. The options and
the commands are in [Building](../getting_started/BUILD.md).

## Layout

| File | Holds |
| --- | --- |
| `CMakeLists.txt` | `project()`, the options and the `add_subdirectory` calls |
| `cmake/SushiTrackBuildPolicy.cmake` | The language standard, output folders, symbol visibility and compiler flags |
| `cmake/SushiTrackEigen.cmake` | The `Eigen3::Eigen` target over the `third_party/eigen` submodule |
| `source/CMakeLists.txt` | The library `sushitrack` and its object twin `sushitrack_static` |
| `tests/CMakeLists.txt` | The test programs |

Every executable and the library land in `build/bin`, so a test program finds `sushitrack.dll`
beside it on Windows without a path change.

## Symbol visibility

Symbols are hidden by default on every platform. Only declarations that carry `SUSHITRACK_API`
(`include/SushiTrack/sushitrack_export.hpp`) leave the shared library. Windows does not use
`CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS`: every function of the C API carries the macro, so
`__declspec(dllexport)` does what `visibility("default")` does on GCC and Clang, and a symbol
that lacks the macro stays hidden on both.

`Tracker` also carries `SUSHITRACK_API`, for the in-tree targets that link its C++ symbols. That
makes MSVC raise C4251 for its standard library members. The build silences C4251: every target
of the tree shares one compiler and one runtime, which is the condition the warning exists to
question, and the C API never exposes `Tracker` to an outside consumer.

## Compiler flags

Release builds use `/O2 /Ot /fp:fast /arch:AVX2` on MSVC and `-O3 -ffast-math -funroll-loops
-mavx2` on GCC and Clang. `-mavx2` is added only on an x86 host, where it matches the MSVC flag;
on ARM it would fail to compile.

## The object twin

The tests and the applications reach classes the shared library keeps hidden, such as `Rect`,
`LAPJVSolver`, the cost calculators and `Logger`. Exporting them would widen the public surface,
so `sushitrack_static` compiles the same sources a second time with `SUSHITRACK_STATIC`, which
turns `SUSHITRACK_API` into nothing. The define is public, so a consumer's own translation units
do not see `__declspec(dllimport)` for a library that has no DLL.

The twin is an `OBJECT` library, not a `STATIC` one. `IMMFilter` and `NSAKalmanFilter` register
themselves in `KalmanFilterRegistry` from a static initialiser, and nothing else in a program
calls into their translation units. A linker pulls archive members only on demand, so MSVC
dropped `imm_filter.obj` from a `STATIC` twin, the registration never ran, and a tracker with
`kalman_type` 2 crashed on a null predictor. An `OBJECT` library hands every object file to the
link line, as the shared library does.
