# Test review: plan

**Status:** Open — not started; the owner chooses inline or parallel

Applies the verdicts of `REPORT.md`, in hierarchy order: the shared helpers, adapters and seams the
rewrites need come first, then the defects the review found, then each module's tests. A module job
applies every record of its module: rewrite, resize, split, merge and delete as the record says, and
writes each add. Nothing in a module job changes production code except a seam a job above built.

## Jobs

| Job | Files | Waits on | Status |
| --- | --- | --- | --- |
| T1 | `tests/support/tolerance.hpp` copied from SushiSkills |  | done |
| T2 | The seams of decision 2 |  | todo |
| T3 | `CapturedLog` moved onto SushiLog's in-memory sink | sushilog T1 | todo |
| T4 | A public profiler macro header for the three tests that include the private one |  | todo |
| T5 | The CLI runner fixtures replaced by sushicore's in-memory runner and console | sushicore T1 | done |
| T6 | The defects of this report, each with a test that fails first; `docs/guides/TESTING.md` aligned with the skill |  | todo |
| T7 | `tools/testing/check_test_additions.py`, `tools/tests/test_check_test_additions.py` deleted |  | done |
| M1 | the tests of `applications` (14 files): 21 rewrite, 2 resize, 7 split, 1 merge, 5 delete | T1, T2, T3, T4, T5, T6, T7 | todo |
| M2 | the tests of `association` (17 files): 20 rewrite, 1 resize, 6 merge, 6 delete | T1, T2, T3, T4, T5, T6, T7 | todo |
| M3 | the tests of `c_api` (5 files): 11 rewrite, 9 resize, 3 split, 1 merge, 6 delete | T1, T2, T3, T4, T5, T6, T7 | todo |
| M4 | the tests of `cli` (8 files): 21 rewrite, 3 resize, 2 delete | T1, T2, T3, T4, T5, T6, T7 | todo |
| M5 | the tests of `cli/services` (12 files): 44 rewrite, 5 resize, 2 split, 4 delete | T1, T2, T3, T4, T5, T6, T7 | todo |
| M6 | the tests of `cli/test_cli` (2 files): 8 rewrite, 4 delete | T1, T2, T3, T4, T5, T6, T7 | todo |
| M7 | the tests of `configuration` (2 files): 19 rewrite, 1 delete | T1, T2, T3, T4, T5, T6, T7 | todo |
| M8 | the tests of `detection` (2 files): 3 rewrite, 3 merge, 7 delete | T1, T2, T3, T4, T5, T6, T7 | todo |
| M9 | the tests of `filtering` (12 files): 23 rewrite, 3 resize, 4 split, 8 delete | T1, T2, T3, T4, T5, T6, T7 | todo |
| M10 | the tests of `logging` (3 files): 4 rewrite, 5 resize, 1 delete | T1, T2, T3, T4, T5, T6, T7 | todo |
| M11 | the tests of `profiling` (4 files): 6 rewrite, 1 split | T1, T2, T3, T4, T5, T6, T7 | todo |
| M12 | the tests of `tools` (2 files): 1 rewrite, 2 resize, 1 split, 5 delete | T1, T2, T3, T4, T5, T6, T7 | todo |
| M13 | the tests of `tracking` (25 files): 63 rewrite, 4 resize, 5 split, 36 merge, 17 delete | T1, T2, T3, T4, T5, T6, T7 | todo |

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
