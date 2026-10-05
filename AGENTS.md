# Agent Directives

These directives apply to every automated agent operating in this repository.

## 1. Maintainability first, performance second

Maintainability is the primary constraint. Every subsystem must be modular, decoupled, and explicit in its data ownership. Optimize hot paths without breaking module boundaries or hiding control flow.

## 2. SOLID discipline without exception

Every unit has a single responsibility. Classes and functions must be detachable and testable in isolation. Sibling components must share uniform structural shapes. Never apply temporary workarounds that only pass tests by coincidence.

## 3. The CLI is the only door (`st` / `sushitrack`)

Execute all build, test, and runtime operations through the repository's dedicated CLI:

```bash
st build
st test
st run <target>
```

`st` is the console script of the package in `cli/`, installed with pipx; `sushitrack` is the same command under its full name. `docs/getting_started/INSTALL.md` has the install steps and `docs/reference/CLI.md` every command.

Never invoke `cmake`, `ninja`, `ctest`, or compiler binaries directly. Only `st` configures the required runtime environment, compiler flags, and dynamic library search paths.

## 4. C++17 ABI agreement

The repository compiles against the C++17 standard (`-std=c++17`). Do not introduce C++20 features. Consumers and sibling libraries rely on C++17 ABI compatibility across shared library boundaries.

## 5. Standard repository layout

Follow the single-module layout defined in `repository-layout`:
- `include/SushiTrack/`: Public C++ headers and C API header (`sushitrack_c.h`).
- `source/`: C++ translation units and C API implementation (`sushitrack_c.cpp`).
- `third_party/`: External dependencies.
- `tools/`: Static checkers and repository audits.
- `tests/`: Unit, integration, and regression test suites.
- `cli/`: The `st` CLI package.
- `bindings/python/`: The ctypes binding.
- `docs/`: Repository documentation; `docs/README.md` is the index.

## 6. Strict quality bar and honesty

Work is complete only when all invariants, tests, and formatting checks pass. "Good enough for now" is rejected. When a task cannot be completed within scope, report exact status. State what was completed and what remains unfinished.
