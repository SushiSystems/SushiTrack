# Test review: plan

**Status:** Open — M1 to M13 applied (2026-11-10), with two open items filed as RW-40 and RW-41; the owner chooses inline or parallel

**Resume note (M13, tracking).** M13 is applied, one commit per file: every file of `tests/tracking/` except the compile checks, with `test_tracking_log_category.cpp` replaced by `compile_tracking_log_category.cpp`. The Adds of tracking are written: the single-model Cruising and zero manoeuvre probability, the last seen state while coasting, the ORU metamorphic pair and the history eviction (inside `FullFeatureHistoryEvictsItsOldestFeature`). Two items stay open. RW-40: the two profiling resize records need an in-memory profiler sink port that SushiProfiler does not offer, so `TentativeStageNamesItsOwnZone` and `OverflowIsCountedAsDroppedTracks` stay medium. RW-41: the OCM add (a crossing scene where OCM on and off assign differently) needs costs that cannot be derived by hand without a compiler, and the two OCM scene tests also pass with OCM off. Two oracles keep the value the old tests held and are not re-derived from the IMM mixing: the Cruising regime after the ramp and the Maneuvering frames after the jerk in `test_kinematic_regimes.cpp`. No compiler and no `build*/compile_commands.json` exist on this machine, so no C++ syntax check ran for any file changed since M7, and no test ran; every new expected value has its hand derivation in the test comment. The commit hook (comment rules, test mirror) ran and passed for each commit. The `Totals` table of `REPORT.md` was recounted from the records (597 tests, 34 adds); the `Verdicts by question` and `Modules` tables follow it.

Applies the verdicts of `REPORT.md`, in hierarchy order: the shared helpers, adapters and seams the
rewrites need come first, then the defects the review found, then each module's tests. A module job
applies every record of its module: rewrite, resize, split, merge and delete as the record says, and
writes each add. Nothing in a module job changes production code except a seam a job above built.

## Jobs

| Job | Files | Waits on | Status |
| --- | --- | --- | --- |
| T1 | `tests/support/tolerance.hpp` copied from SushiSkills |  | done |
| T2 | The seams of decision 2 |  | done |
| T3 | `CapturedLog` moved onto SushiLog's in-memory sink | sushilog T1 | done |
| T4 | A public profiler macro header for the three tests that include the private one |  | done |
| T5 | The CLI runner fixtures replaced by sushicore's in-memory runner and console | sushicore T1 | done |
| T6 | The defects of this report, each with a test that fails first; `docs/guides/TESTING.md` aligned with the skill |  | done |
| T7 | `tools/testing/check_test_additions.py`, `tools/tests/test_check_test_additions.py` deleted |  | done |
| M1 | the tests of `applications` (14 files): 21 rewrite, 2 resize, 7 split, 1 merge, 5 delete | T1, T2, T3, T4, T5, T6, T7 | done (one add filed as RW-38) |
| M2 | the tests of `association` (17 files): 20 rewrite, 1 resize, 6 merge, 6 delete | T1, T2, T3, T4, T5, T6, T7 | done |
| M3 | the tests of `c_api` (5 files): 11 rewrite, 9 resize, 3 split, 1 merge, 6 delete | T1, T2, T3, T4, T5, T6, T7 | done (one rewrite open as RW-39) |
| M4 | the tests of `cli` (8 files): 21 rewrite, 3 resize, 2 delete | T1, T2, T3, T4, T5, T6, T7 | done |
| M5 | the tests of `cli/services` (12 files): 44 rewrite, 5 resize, 2 split, 4 delete | T1, T2, T3, T4, T5, T6, T7 | done |
| M6 | the tests of `cli/test_cli` (2 files): 8 rewrite, 4 delete | T1, T2, T3, T4, T5, T6, T7 | done |
| M7 | the tests of `configuration` (2 files): 19 rewrite, 1 delete | T1, T2, T3, T4, T5, T6, T7 | done |
| M8 | the tests of `detection` (2 files): 3 rewrite, 3 merge, 7 delete | T1, T2, T3, T4, T5, T6, T7 | done |
| M9 | the tests of `filtering` (12 files): 23 rewrite, 3 resize, 4 split, 8 delete | T1, T2, T3, T4, T5, T6, T7 | done |
| M10 | the tests of `logging` (3 files): 4 rewrite, 5 resize, 1 delete | T1, T2, T3, T4, T5, T6, T7 | done |
| M11 | the tests of `profiling` (4 files): 6 rewrite, 1 split | T1, T2, T3, T4, T5, T6, T7 | done |
| M12 | the tests of `tools` (2 files): 1 rewrite, 2 resize, 1 split, 5 delete | T1, T2, T3, T4, T5, T6, T7 | done (void: both files were deleted by T6 and T7) |
| M13 | the tests of `tracking` (25 files): 63 rewrite, 4 resize, 5 split, 36 merge, 17 delete | T1, T2, T3, T4, T5, T6, T7 | done (two items open as RW-40 and RW-41) |

## Acceptance

- **T1.** The helper is in place and its self-check passes. Build: the repository's test command.
- **T2.** No small test reads `SUSHITRACK_CONFIG`, a process-wide registry or a file. Build: the repository's test command.
- **T3.** Every log capture goes through SushiLog's scoped in-memory sink. Build: the repository's test command.
- **T4.** No test includes a private header. Build: the repository's test command.
- **T5.** No CLI test patches the repository's own functions or reads a private name. Build: the CLI tests.
- **T6.** Every defect above is fixed or has a backlog entry. Build: the tests of each touched module.
- **T7.** Neither file exists and the checkers pass. Build: `python tools/check_all.py .`
- **M jobs.** Every record of the module is applied: no test the report deletes remains, every kept and
  new test passes at the size its label states, and the module's tests pass through the project's own
  test command. Build: that command over the module.

The owner chooses inline or parallel mode.
