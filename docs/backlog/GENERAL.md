# General

## Remaining work

| ID | Pri | State | Title | Done when | Blocked by | Source | Opened |
| --- | --- | --- | --- | --- | --- | --- | --- |
| RW-3 | P2 | decision | Decide where the Python binding lives | SushiSkills' `repository-layout` names a home for a language binding and `bindings/python/` sits there |  |  | 2026-10-19 |
| RW-5 | P2 | decision | Decide which of AGENTS.md and CLAUDE.md is the source of the agent instructions | one file holds the directives and the other points at it |  |  | 2026-10-19 |
| RW-6 | P2 | decision | Decide whether contributions from outside are accepted | the owner has named the contributor agreement, or ruled contributions out |  |  | 2026-10-19 |
| RW-7 | P2 | decision | Decide whether the SushiTrack migration is closed | `docs/design/SUSHITRACK_MIGRATION/README.md` carries a `Shipped` status, or a phase with work left |  | `docs/design/SUSHITRACK_MIGRATION/README.md` | 2026-10-19 |
| RW-36 | P2 | open | Set the CTest size timeouts to the testing skill's 1, 10 and 120 seconds | cmake/SushiTrackTests.cmake gives small 1 s, medium 10 s and large 120 s per case and docs/guides/TESTING.md says so |  | `docs/guides/TESTING.md` | 2026-11-09 |
| RW-44 | P1 | open | Use the shared clean command and drop build --clean | st clean says what it would remove and removes trees only with --yes, and st build has no --clean |  | `cli/sushitrack_cli/cli.py` | 2026-11-21 |

## Known issues

| ID | Origin | Symptom | Workaround | Fix | Source | Opened |
| --- | --- | --- | --- | --- | --- | --- |
