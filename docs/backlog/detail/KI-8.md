# KI-8 st doctor never looks for GoogleTest

The row of this entry is in `docs/backlog/INSTALLING_VERSIONS.md`. This file holds what did not fit the row.

- Origin: own
- Where: `cli/sushistack.deps.toml`
- Cause: the GoogleTest fragment carries no check command
- Recognise: `st doctor` passes and `st build` fails at `find_package(GTest)`
- Workaround: `st build --no-test`
- Fix: RW-18
