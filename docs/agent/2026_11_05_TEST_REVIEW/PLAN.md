# Test review: plan

**Status:** Open — M1 to M12 applied (2026-11-10); M13 partial (11 of 25 files); the owner chooses inline or parallel

**Resume note (M13, tracking).** M8 closed with the owner's ruling (REPORT decision 14) and its missing iou add; M9 is applied (seams: `InMemoryGateValidator` in `include/SushiTrack/filtering/in_memory_gate_validator.hpp`; the registry seam T2 built already serves the factory and registry tests); M10 is applied with one seam, `LogThrottle::admit(suppressed, generation)`; M11 is applied; M12 is void, since both of its files were deleted by T6 and T7. M13 applied so far, one commit per file: `test_frame_timing_policy.cpp`, `test_wall_clock_timing_policy.cpp`, `test_timing_policy.cpp` (deleted for `compile_timing_policy.cpp`), `test_advanced_path.cpp`, `test_association_stages.cpp`, `test_boundary_conditions.cpp`, `test_buffers.cpp`, `test_byte_association.cpp`, `test_config_variations.cpp` and `test_duplicate_suppression.cpp`. Resume at `tests/tracking/test_tracker/test_end_to_end.cpp`; then `test_full_stack.cpp`, `test_kinematic_regimes.cpp`, `test_multi_object.cpp`, `test_observation_momentum.cpp`, `test_occlusion_recovery.cpp`, `test_profiling.cpp`, `test_tentative_lifecycle.cpp`, `test_time_aware.cpp`, `tests/tracking/test_tracker_config.cpp`, `tests/tracking/test_tracking_log_category.cpp` and `tests/tracking/test_tracklet/`. The Adds of tracking (tracklet kinematic state, last-seen state, maneuver probability, appearance eviction, OCM and ORU) are still to write. No compiler and no `build*/compile_commands.json` exist on this machine, so no C++ syntax check ran for any file changed since M7, and no test ran; every new expected value has its hand derivation in the test comment. The commit hook (comment rules, test mirror) ran and passed for each commit. The `Totals` table of `REPORT.md` was not recounted.

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
| M13 | the tests of `tracking` (25 files): 63 rewrite, 4 resize, 5 split, 36 merge, 17 delete | T1, T2, T3, T4, T5, T6, T7 | partial |

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
