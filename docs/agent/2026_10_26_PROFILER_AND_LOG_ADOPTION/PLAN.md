# Plan: profiler and log adoption

**Status:** Open — written 2026-10-26 from `SPEC.md`; no task starts before the owner answers the decisions each task names

Tasks run in hierarchy order: a brick before its caller, a pin before its first use. Every task
has one acceptance criterion and one build; a task that would need two builds is two tasks.
"Orchestrator" marks a step that edits a build file (CMake, the deps toml) or a CI file; neither
a worker nor the implementer touches those. The build of a C++ task is `st test` scoped to the
suites it names, which builds first; a Python task builds nothing and runs `st test --cli` or the
test file it names. Nothing here was built or run.

Each task also lists the manual pages its change makes false. A worker reports them; the
orchestrator corrects them in the commit that closes the task (`documentation` skill,
`AGENT_WORK.md`).

Tests follow the `testing` skill: a `fix` carries a regression test seen failing first, a `feat`
a test per behaviour of its public surface, the suite name opens with `Small_`, `Medium_` or
`Large_`, the test file mirrors its header, and a log record is asserted through
`tests/captured_log.hpp` (`CapturedLog`).

## Part A: logging in the library

### A1 Configuration loader records

- **Waits on:** decision 12 (warn or refuse), decision 15 (`load_config_from_json`).
- **Covers:** C1 to C5, C6.
- **Files:** `source/configuration/config_loader.cpp`, `include/SushiTrack/configuration/config_loader.hpp`
  (only if decision 15 deletes), `tests/configuration/test_config_loader.cpp`.
- **Acceptance:** an explicit path that does not exist, `SUSHITRACK_CONFIG` naming a missing
  file, a wrong-typed value, an unknown key and a failed `sushitrack_create_from_config` each
  produce the one record the SPEC row names, at its level, and the return status is the one
  decision 12 chose.
- **Build:** `st test` on `Medium_ConfigLoader*`.
- **Tests carried:** one `fix`-style test per row, each seen failing on the current code; the test
  at `test_config_loader.cpp:506` changes only if decision 12 changes the status.
- **Pages made false:** `docs/reference/CONFIGURATION.md`, `docs/reference/C_API.md` (the status
  of a missing file), `docs/architecture/OVERVIEW.md` "Logging".

### A2 Create-time validation and refusal records

- **Waits on:** decision 12 (refuse or warn), decision 13 (categories).
- **Covers:** C7, C8, C9, C10.
- **Files:** `source/c_api/sushitrack_c.cpp`, `tests/c_api/test_sushitrack_c/test_error_handling.cpp`.
- **Acceptance:** `sushitrack_create_ex` with an unregistered `kalman_type` returns
  `SUSHITRACK_INVALID_ARGUMENT` and logs an ERROR instead of crashing at the first track; each of
  the three entry points logs an ERROR naming the refused argument; an unknown `iou_type` and a
  clamped `frame_rate` or `track_buffer` log a WARN.
- **Build:** `st test` on `Small_SushiTrackC*`.
- **Tests carried:** a regression test for the null `kf_` crash, written first and seen failing
  (it fails by crashing the test process, so it runs as a death test or under a guard); one test
  per record.
- **Pages made false:** `docs/reference/C_API.md`.

### A3 Lifecycle and solver records that need no rate limit

- **Waits on:** nothing.
- **Covers:** C15, C17, C18, C20 (the Kalman registry half).
- **Files:** `source/tracking/tracker.cpp`, `source/association/lapjv_solver.cpp`,
  `include/SushiTrack/filtering/kalman_filter_registry.hpp`,
  `tests/tracking/test_tracker/test_duplicate_suppression.cpp`,
  `tests/tracking/test_tracker/test_tentative_lifecycle.cpp`,
  `tests/association/test_lapjv_solver.cpp`, `tests/filtering/test_kalman_filter_registry.cpp`.
- **Acceptance:** a duplicate drop logs one DEBUG naming both track ids, a low-pool tentative
  confirmation logs the same DEBUG as the high pool, an empty matrix with non-zero counts logs an
  ERROR, a second registration of one Kalman id logs a WARN.
- **Build:** `st test` on the four suites above.
- **Tests carried:** a `feat` test per record, using the scenarios the existing suites build
  (`tests/tracklet_builders.hpp`, `tests/c_detection_builders.hpp`).
- **Pages made false:** `docs/architecture/OVERVIEW.md` "Logging" if the DEBUG list is named.

### A4 Recurring records, after SushiLog can limit them

- **Waits on:** SushiLog gap L2 (owner extends SushiLog), the SushiLog pin bump (orchestrator),
  decision 16.
- **Covers:** C2, C11, C12, C13, C14, C16, C19.
- **Files:** `source/filtering/kalman_filter.cpp`, `source/filtering/imm_filter.cpp`,
  `source/tracking/tracklet.cpp`, `source/association/lapjv_solver.cpp`,
  `source/c_api/sushitrack_c.cpp`, `source/configuration/config_loader.cpp`, the tests of each.
- **Acceptance:** each row's event logs once or at the limited rate, ends with the suppressed
  count, and the three existing per-call WARNs at `kalman_filter.cpp:180`, `:222`, `:253` plus the
  silent `:312-315`, `:362-365` and the IMM sites use one wording.
- **Build:** `st test` on `Small_KalmanFilter*`, `Small_IMMFilter*`, `Small_Tracklet*`.
- **Tests carried:** a test per row that triggers it a thousand times and counts the records.
- **Pages made false:** `docs/architecture/OVERVIEW.md` "Logging".
- **Orchestrator step:** `cli/sushistack.deps.toml` SushiLog pin (build file).

### A5 Frame context on lifecycle records

- **Waits on:** SushiLog gap L3.
- **Covers:** C21.
- **Files:** `source/tracking/tracker.cpp`, `tests/tracking/test_tracker/test_end_to_end.cpp`.
- **Acceptance:** every DEBUG record made inside one `Tracker::update` carries that frame's id.
- **Build:** `st test` on `Small_TrackerEndToEnd*`.
- **Tests carried:** one `feat` test over a two-frame scenario.
- **Pages made false:** none.
- **Orchestrator step:** the SushiLog pin bump, shared with A4.

## Part B: logging in applications, Python and the CLI

### B1 Tracker bridge and adapters

- **Waits on:** decision 14.
- **Covers:** C22 to C25 and P16, P17.
- **Files:** `applications/tracker_bridge/tracker_bridge.cpp`, `tracker_bridge.h`,
  `sushitrack_adapter.hpp`, `byte_track_adapter.hpp`, `oc_sort_adapter.hpp`, `json_settings.hpp`,
  `tracker_registry.hpp`, the matching files under `tests/applications/tracker_bridge/`.
- **Acceptance:** `tracker_process_c` and `create_tracker` report each refusal, truncation and
  exception through the stream decision 14 chose, and no exception leaves an `extern "C"`
  function.
- **Build:** `st test` on `Small_TrackerBridge*` (the application test program).
- **Tests carried:** a `fix` test for the escaping exception, seen failing; a test per record.
- **Pages made false:** `applications/tracker_bridge/README.md`, `docs/guides/INFERENCE.md`.
- **Orchestrator step:** `applications/tracker_bridge/CMakeLists.txt` links SushiLog into
  `tracker_adapters` when decision 14 chooses the project macros.

### B2 MOT runner inputs

- **Waits on:** decision 14 (same stream).
- **Covers:** C26 to C29.
- **Files:** `applications/benchmarks/mot_engine/mot_sequence.hpp`, `mot_runner.cpp`, `main.cpp`,
  `tests/applications/benchmarks/mot_engine/test_mot_sequence.cpp`, `test_mot_runner.cpp`.
- **Acceptance:** an unreadable `det.txt`, skipped rows, a defaulted frame rate and an empty
  sequence each print the named message with the file and the counts, and an empty sequence prints
  no `[ PERF ]` line.
- **Build:** `st test` on `Small_MotSequence*`, `Small_MotRunner*`.
- **Tests carried:** `fix` tests for the zero-frame PERF line (it breaks `evaluate.py:59-60`) and
  for the missing `is_open` check, each seen failing.
- **Pages made false:** `docs/guides/EVALUATION.md`, `applications/benchmarks/README.md`.

### B3 Python binding

- **Waits on:** decision 18 (home of tests and of the logging helper), SushiLog gap L1 for P5/P6.
- **Covers:** P1 to P6.
- **Files:** `bindings/python/sushitrack.py`, a new test file in the home decision 18 names.
- **Acceptance:** a missing symbol raises at load naming it, the chosen library path is logged at
  DEBUG, a narrowed override and an accepted overflow each log once, and a failing callback does
  not vanish.
- **Build:** the Python test file, run through `st`.
- **Tests carried:** one test per row against a fake library object.
- **Pages made false:** `bindings/python/README.md`, `docs/reference/C_API.md` if the overflow
  surface changes.

### B4 Inference pipeline

- **Waits on:** decision 11 only for the profiling parts; this task is logging only.
- **Covers:** P7 to P15.
- **Files:** `applications/inference/pipeline.py`, `demo.py`, `tracker_bridge.py`, a test file in
  the home decision 18 names.
- **Acceptance:** a truncated image sequence, a missing checkpoint, a CPU fallback, a zero-frame
  run and a swallowed `TypeError` each print the named message; the `except TypeError` is narrowed.
- **Build:** the Python test file, run through `st`.
- **Tests carried:** `fix` tests for the truncated sequence and the swallowed `TypeError`, seen
  failing; the rest as `feat` tests with fake readers and detectors.
- **Pages made false:** `docs/guides/INFERENCE.md`.

### B5 Evaluator and CLI

- **Waits on:** nothing.
- **Covers:** P18 to P21.
- **Files:** `applications/benchmarks/mot_engine/evaluator/data_loader.py`, `trackeval_runner.py`,
  `cli/sushitrack_cli/services/label.py`, `config.py`, `env.py`, `cli/tests/services/test_label.py`,
  `cli/tests/test_config.py`, `cli/tests/test_env.py`.
- **Acceptance:** a skipped PERF line, an ignored config key and a bad config integer each print a
  `console.warn`; a missing timing is no longer written as `0.0`.
- **Build:** `st test --cli`.
- **Tests carried:** a test per row; the `0.0` change is a `fix` test seen failing.
- **Pages made false:** `docs/reference/CLI.md`, `docs/guides/EVALUATION.md`.

## Part C: profiling

### C1 Pin SushiProfiler (orchestrator)

- **Waits on:** SushiProfiler `v0.1.0` tagged, decisions 2 and 3.
- **Files:** `cli/sushistack.deps.toml` (`[sushisystems.sushiprofiler]`, and `depends_on`),
  `CMakeLists.txt`, a new `cmake/SushiTrackSushiProfiler.cmake` shaped like
  `cmake/SushiTrackSushiLog.cmake`, `source/CMakeLists.txt` (link, and the option),
  `.github/actions/sushiprofiler/action.yml`, `.github/workflows/ci.yml`, `nightly.yml`.
- **Acceptance:** `st setup` fetches the pinned commit, `SUSHIPROFILER_DIR` reaches CMake
  (`sushicore/provision/sushi_sources.py:39` names it that way), and `SushiProfiler::SushiProfiler`
  links into `sushitrack` and `sushitrack_static` with no call in the source yet.
- **Build:** `st build`.
- **Tests carried:** none; the existing suites stay green. `cli/tests/services/test_build.py`
  gains a case for the new `-DSUSHIPROFILER_DIR` only if the CLI changes.
- **Pages made false:** `docs/getting_started/INSTALL.md`, `docs/getting_started/BUILD.md`,
  `docs/architecture/BUILD_SYSTEM.md`, `docs/guides/DEPLOY_PACKAGES.md` if a runtime file ships.

### C2 Profiling front

- **Waits on:** C1, decisions 6 and 7.
- **Files:** `include/SushiTrack/profiling/profiling.hpp` and, per decision 6, a key header,
  `tests/profiling/test_profiling.cpp`.
- **Acceptance:** the macros take SushiTrack's level and expand to nothing when
  `SUSHITRACK_ENABLE_PROFILER` is 0 in the translation unit, and a zone recorded under an open
  session appears in the record under its group.
- **Build:** `st test` on `Small_Profiling*`.
- **Tests carried:** a `feat` test for the compiled-out expansion (a translation unit that defines
  the switch to 0 before the include, as the logging front's ceiling is proved), one for a recorded
  zone, one for a counter per operation.
- **Pages made false:** none yet.

### C3 Level 1 zones

- **Waits on:** C2.
- **Covers:** the level 1 rows of SPEC 1.3.
- **Files:** `source/tracking/tracker.cpp`, `source/c_api/sushitrack_c.cpp`,
  `tests/tracking/test_tracker/test_profiling.cpp`.
- **Acceptance:** a three-frame scenario under a session records every level 1 zone name in each
  active frame, and the tracks the scenario outputs are equal with the gate open and closed.
- **Build:** `st test` on `Medium_TrackerProfiling*`.
- **Tests carried:** a `feat` test for the zone names and one for output equality.
- **Pages made false:** `docs/architecture/OVERVIEW.md` ("Per-frame pipeline", a new "Profiling"
  section), the module facts for `profiling/`.

### C4 Counters and instants

- **Waits on:** C3.
- **Covers:** SPEC 1.4.
- **Files:** `source/tracking/tracker.cpp`, `source/c_api/sushitrack_c.cpp`,
  `tests/tracking/test_tracker/test_profiling.cpp`.
- **Acceptance:** for a scripted scenario the recorded per-frame counters equal the counts the
  scenario's tracks give (detections in, matches, lost, confirmed, alive, written, overflow).
- **Build:** `st test` on `Medium_TrackerProfiling*`.
- **Tests carried:** a `feat` test per counter family, with the expected counts written in the test.
- **Pages made false:** the same "Profiling" section.

### C5 Level 2 zones

- **Waits on:** C3.
- **Files:** `source/association/lapjv_solver.cpp`, `source/tracking/tracker.cpp`,
  `tests/association/test_lapjv_solver.cpp`.
- **Acceptance:** `solve` and `cost_matrix` zones are absent from a record made at level 1 and
  present at level 2.
- **Build:** `st test` on `Small_LAPJVSolver*`.
- **Tests carried:** a `feat` test over both levels.
- **Pages made false:** none.

### C6 Environment contributor

- **Waits on:** C2, decision 7 (library or application), S4 answered (date fact).
- **Files:** `include/SushiTrack/profiling/` and `source/profiling/` (or the application folder per
  decision 7), `tests/profiling/test_environment.cpp`.
- **Acceptance:** a record made through the contributor holds the library name, its version, the
  commit and the date of the run, and `sp record compare` accepts two such records.
- **Build:** `st test` on `Small_ProfilingEnvironment*`.
- **Tests carried:** a `feat` test for each fact.
- **Orchestrator step:** a compile definition carrying the commit (`source/CMakeLists.txt`), a
  build file; `RW-17` already asks for one version source, so it follows that entry.
- **Pages made false:** `docs/reference/CONFIGURATION.md`.

### C7 Benchmark runner records

- **Waits on:** C3, C6, decisions 5, 8 and 9.
- **Files:** `applications/benchmarks/mot_engine/mot_runner.cpp`, `mot_runner.hpp`, `main.cpp`,
  `tests/applications/benchmarks/mot_engine/test_mot_runner.cpp`.
- **Acceptance:** one run over a fixture sequence writes a record that `read_record` accepts, whose
  `timeline:` subject holds one period per frame and whose drop counts are zero; `RunResult` keeps
  the frame count and loses the microsecond sum.
- **Build:** `st test` on `Medium_MotRunner*`.
- **Tests carried:** a `feat` test over the fixture, a `refactor`-safe test that the prediction
  file is byte-identical with and without profiling.
- **Orchestrator step:** `applications/benchmarks/mot_engine/CMakeLists.txt` links SushiProfiler
  into `mot_engine_runner` (and, per decision 3, builds `mot_engine` against `sushitrack_static`).
- **Pages made false:** `docs/guides/EVALUATION.md`, `applications/benchmarks/README.md`,
  `docs/reference/CLI.md`.

### C8 CLI and evaluator read the record

- **Waits on:** C7, decisions 9 and 10, SushiProfiler gap S5 if decision 10 chooses (b).
- **Files:** `cli/sushitrack_cli/services/label.py`, `evaluate.py`,
  `applications/benchmarks/mot_engine/evaluator/trackeval_runner.py`, `report_generator.py`,
  `cli/tests/services/test_label.py`, `test_evaluate.py`.
- **Acceptance:** `st eval` prints each latency from the record's median with its interval, a
  sequence without a record prints `N/A`, and `_parse_perf` is gone or kept per decision 9.
- **Build:** `st test --cli`.
- **Tests carried:** a `feat` test over a fixture record file, a `fix` test that no latency of `0`
  is printed for a missing record, seen failing.
- **Pages made false:** `docs/guides/EVALUATION.md`, `docs/reference/CLI.md`.

### C9 Overhead and drops, measured

- **Waits on:** C7.
- **Files:** none in the tree; a baseline record and a profiled record of one benchmark.
- **Acceptance:** `REPORT.md` of this folder holds, for MOT17 and DanceTrack, the machine, the
  commit, the date, the per-frame cost with the profiler off, with the gate closed and recording,
  and the drops; the recording cost is read against decision 17's budget.
- **Build:** `st eval` twice (one build already made by C7).
- **Tests carried:** none; `perf` rule: existing tests unchanged and green.
- **Pages made false:** none.

### C10 Tables from records (orchestrator)

- **Waits on:** C8, C9, decision 10.
- **Files:** `docs/reference/BENCHMARKS.md`, `BENCHMARKS_IMM.md`, `BENCHMARKS_LINEAR_KALMAN.md`,
  `docs/design/REMAINING_WORK.md` (RW-19), `docs/reference/CHANGELOG.md`; manual and backlog are
  the orchestrator's.
- **Acceptance:** each table names the date, the commit and the machine of its record, and RW-19
  closes.
- **Build:** `st eval` already run; the checkers `check_docs_truth.py` and `check_changelog.py`.
- **Tests carried:** none.

### C11 Python profiling

- **Waits on:** decisions 4 and 11; SushiProfiler P9 if decision 11 picks (a).
- **Files:** depends on the option: `include/SushiTrack/c_api/sushitrack_c.h`,
  `source/c_api/sushitrack_c.cpp`, `bindings/python/sushitrack.py`,
  `applications/inference/pipeline.py`, with tests in the home decision 18 names.
- **Acceptance:** not writable before decisions 4 and 11; the task stands here so the dependency
  of the Python stages on them stays visible.
- **Build:** one, named when the owner has chosen.
- **Pages made false:** `docs/reference/C_API.md`, `docs/guides/INFERENCE.md`.

### C12 Nightly gate (orchestrator)

- **Waits on:** RW-24 (a runner that holds the sequences), C7.
- **Files:** `.github/workflows/nightly.yml`, a budget file beside the benchmark.
- **Acceptance:** the nightly job runs `sp record check` on the record and fails on a budget miss.
- **Build:** the workflow run itself.

## What each wave could hold

The owner chooses inline or waves. If waves, these tasks share no file and wait on nothing but the
decisions shown:

| Wave | Tasks | File sets | Closing build |
| --- | --- | --- | --- |
| 1 | A3, B2, B5 | library tracker and solver, MOT application, evaluator and CLI | `st test` for the three; `st test --cli` |
| 2 | A1, A2 | `config_loader.cpp`, `sushitrack_c.cpp`; decisions 12 and 13 first | `st test` |
| 3 | C1 (orchestrator), then C2 | build files, then the profiling front | `st build`, then `st test` |
| 4 | C3 then C4 and C5 | `tracker.cpp` and `sushitrack_c.cpp` are shared by C3 and C4, so they run in order; C5 touches the solver | `st test` |
| 5 | C6, C7, C8, C9 | in order | `st test`, `st test --cli`, `st eval` |
| later | A4, A5, B1, B3, B4, C11, C10, C12 | each waits on a decision or on SushiLog | one each |

A3 and C3 both change `tracker.cpp`, and A2, A4 and C3 all change `sushitrack_c.cpp`, so those
tasks never share a wave.

## Decisions I took

Each is a choice the rulings of SPEC section 9 leave open; the reason is one line.

| # | Choice | Reason |
| --- | --- | --- |
| I1 | One category header per area under `include/SushiTrack/<area>/`; the project moved to `logging/sushitrack_log_project.hpp`; `K_SUSHITRACK_LOG_CATEGORY` is removed | The logging skill gives each module its own category header, and a kept alias would hide which area logs |
| I2 | Recurring events use a per-call-site `LogThrottle` (first event, then one in a thousand, with the suppressed count), not a counter in `Tracker` | The filters, the solver and the tracklet have no `Tracker` to report through; the throttle is deleted when SushiLog RW-3 lands |
| I3 | `SUSHITRACK_CONFIG` naming a missing file is refused like a named path | It is the caller's explicit choice, and refusing needs no rate limit |
| I4 | A configuration file that exists but cannot be opened returns `SUSHITRACK_RUNTIME_ERROR` | The read failed, which is not an invalid argument |
| I5 | `sushitrack_create_from_config` returns null when the load is refused | Decision 12 refuses the file; building defaults would hide the refusal |
| I6 | Applications log under `tracker_bridge` and `mot_engine`, each with its own category header | Decision 14 sends applications to the project macros and every module owns one category |
| I7 | `tracker_process_c` returns `tracker_bridge_status_t`: OK, INVALID_ARGUMENT, TRUNCATED, ERROR | Decision 14 asks for a status; the truncation needs its own value |
| I8 | `RunResult` keeps `total_tracking_us` | Decision 9 keeps the `[ PERF ]` line beside the record, and the line needs the figure |
| I9 | A sequence with no detections prints no `[ PERF ]` line and the exit code stays 0 | The run did not fail; the missing line is what `evaluate.py` can read as "not measured" |
| I10 | Eight counters and the stage zones are level 1; the other counters, instants and the solve and cost zones are level 2 | SPEC 1.8 says to cut counters at level 1 first to stay inside the budget |
| I11 | The tracker and the sequence are environment facts (`mot.tracker`, `mot.sequence`), not period tags | A period tag takes a lock each frame (SushiProfiler S8) |
| I12 | The record is written with `write_record` by `FrameRecorder`, not through a writer | It needs no terminal stream and no registry writer |
| I13 | Python tasks B3, B4, C11 and the Python half of C8 stay open; only the `tracker_bridge.py` binding of the new status and the empty-frame fix are written | Decisions 4, 11 and 18 leave them no home or workaround; decision 14 forces the binding change |
| I14 | `log_throttle.hpp` is included by the files that use it, not by `logger.hpp` | A widely included public header gains no include and nothing rebuilds for it |
| I15 | `LAPJVSolver::solve` tests finiteness before the threshold and puts the unmatchable cost in place of a non-finite cell | Under clang `-ffast-math` a NaN compares as at or below the threshold, so the count stayed 0 and the NaN reached the solver |
| I16 | `Tracker::update` ignores a detection with a non-finite box or score, with one throttled WARN | The NaN and Inf malformed-detection tests document the contract; dropping the input is the only answer that keeps every other track finite |
| I17 | `sushitrack_create_ex` returns `SUSHITRACK_INITIALIZATION_FAILURE` for any non-allocation exception | Everything inside its try block builds a component, which is what the status names; it returned `SUSHITRACK_RUNTIME_ERROR` and the status was never used |
| I18 | `is_non_finite` lives in the private header `source/detection/finite_value.hpp`, shared by the solver and the tracker | Two copies of a bit test would drift; it is not part of the public surface |
