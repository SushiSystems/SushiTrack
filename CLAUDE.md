# CLAUDE.md

## Getting oriented

When starting work on this project:

1. Follow the Sushi Systems doctrine (`sushiskills`) and repository conventions.
2. Read `docs/CONTRIBUTING.md` first and follow its rules.
3. Read `docs/README.md` and `docs/architecture/OVERVIEW.md` before modifying subsystems.
4. `docs/design/` holds design specifications and roadmaps. Read the relevant design document before changing a subsystem.

## Active skills

The repository enforces these skills:

- `sushiskills`: Stack-wide doctrine, SOLID principles, maintainability priority, and CLI-only entry point.
- `repository-layout`: Standard directory layout, module isolation, and root hygiene.
- `cpp-code-style`: C++17 standard, LLVM base, 4-space indentation, 100-column limit, and Allman braces.
- `documentation`: Single-home documentation structure, agent work lifecycle, and changelog discipline.
- `project-tools`: Verification scripts and checkers under `tools/`.
- `source-comments`: Doxygen formatting, license blocks, and file headers.
- `logging`: Log categories, levels, and structured emission.
- `continuous-integration`: CI workflows, build verification, and release gates.
- `humanizer`: Plain, factual prose in documentation, comments, and reports without generated filler.
- `testing`: Unit, integration, regression coverage, and CLI test verification.
- `platform-portability`: Windows and Linux compatibility, export macros, and path handling.

## How to work

### Priority 1: Maintainability

Design each component as an isolated brick. Sibling components share uniform shape. Changes must remain local to their unit instead of propagating across boundaries.

### Priority 2: Performance

Profile and measure hot paths before optimizing. Do not sacrifice readability or encapsulation for speculative gains.

### Quality bar

Pass all verification checks and test suites before reporting completion. "Good enough for now" is rejected.

### Honesty

State what was verified and what was not. When blocked or uncertain about public API boundaries or data ownership, ask instead of guessing.

### Building and toolchain

Never invoke CMake or CTest directly. Run builds and tests through `python cli/st.py build` and `python cli/st.py test`. The CLI sets the necessary compiler flags and dynamic library paths.

### Task tracking

Track work with actionable task lists. Record progress in the corresponding `docs/agent/` work directory.

---

love you boss, kolay gelsin.
