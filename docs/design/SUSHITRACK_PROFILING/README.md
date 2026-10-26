# SushiTrack profiling

**Status:** Open — P1 to P7 written and awaiting the first build, P8 waits on SushiProfiler P9, P9 and P10 wait on a measurement
**About:** SushiTrack records its frame, stages, counters and track events through SushiProfiler, and the benchmark reads its latency from the record.
**Modules:** none

## Problem

The timings of SushiTrack live in three places that nothing joins: a `steady_clock` around the
adapter's update in `mot_engine`, the Python pipeline's `time.time()`, and benchmark tables typed
by hand with no date, commit or machine. None of them says which stage of a frame costs what, and
the tables cannot be compared with a later run. SushiProfiler gives a library zones, counters and
period timelines, and a record that two runs can be compared by. This topic is SushiProfiler's phase P8,
the adoption by SushiTrack.

## Decision

SushiProfiler is optional: a CMake option, on by default, and `[sushisystems.sushiprofiler]` sits
in `cli/sushistack.deps.toml` either way. It is linked as the shared library, so one process holds
one collector and a session in `mot_engine` sees the zones inside `sushitrack.dll`. The public C
ABI gains no profiling call, so the Python path waits for SushiProfiler P9.

The host marks the frame period and the library records zones and counters only: `mot_engine`
marks `sushitrack.frame` around each `adapter.update`. One record is written per tracker and
sequence, each a session of its own, under `build/profiles/`, which git ignores. Level 1 is
compiled into release builds and level 2 into debug builds; the budget is under 1 % with the gate
closed. The `[ PERF ]` line stays for one release beside the record, and `st eval` reads the record
file itself, with no dependency on `sp`. The benchmark tables stay by hand, each with a link to its
record, until SushiProfiler writes tables.

## Design

The macros `SUSHITRACK_ZONE`, `SUSHITRACK_COUNTER` and `SUSHITRACK_INSTANT` sit in
`source/private_include/SushiTrack/profiling/profiling.hpp`, which no installed header includes. They
take the level of `SUSHITRACK_PROFILE_LEVEL` and expand to nothing when `SUSHITRACK_ENABLE_PROFILER`
is 0; both come from the generated `profile_config.hpp`, not from compile definitions. The names
are in `include/SushiTrack/profiling/profiler_keys.hpp` and listed in
[Profiler keys](../../reference/PROFILER_KEYS.md); the groups are `Associate`, `Filter`, `Detect` and
`Track`. The environment contributor, with the library's name, version, commit and the run's date,
lives in the library (`sushitrack_profiler_parts.cpp`); its public header forward-declares
`SushiProfiler::Registry` and names no other SushiProfiler type, so SushiProfiler links PRIVATE. The stage zones wrap each step of `Tracker::update`; the `solve` and `cost_matrix`
zones and the per-track instants are level 2 because mode 0 of the primary stage solves once per
track age.

`FrameRecorder` in `applications/benchmarks/mot_engine/` starts the session, marks one period per
frame, writes the record and reports what the capture lost. A second session starts after the first
stopped, once per pair. Flows and lanes are not adopted: the library is single-threaded and a
detection has no identity across frames.

## Phases

| Id | Phase | State |
| --- | --- | --- |
| P1 | The profiling front and its keys: `profiling.hpp`, `profiler_keys.hpp` | building |
| P2 | Level 1 stage zones in `Tracker::update` and the output | building |
| P3 | Counters and instants of the frame | building |
| P4 | Level 2 zones: `solve` and `cost_matrix` | building |
| P5 | The environment contributor and `register_profiler_parts` | building |
| P6 | `mot_engine` marks `sushitrack.frame` and writes a record per tracker and sequence | building |
| P7 | `st eval` reads the latency from the record | building |
| P8 | The Python pipeline and binding record through SushiProfiler | planned |
| P9 | Overhead and drops measured on MOT17 and DanceTrack against the budget | planned |
| P10 | The benchmark tables carry the date, commit and machine of their record | planned |

### Work

<!-- work:begin -->
- [2026_10_26_PROFILER_AND_LOG_ADOPTION](../../agent/2026_10_26_PROFILER_AND_LOG_ADOPTION/SPEC.md): P1, P2, P3, P4, P5, P6, P7
<!-- work:end -->

## Open questions

- Whether a second `Session::start` after `stop` in one process works. SushiProfiler states it
  nowhere (its RW-20); `mot_engine` and the tests need it, and the first build's tests confirm it.
- Whether recording costs more than 1 % of a DanceTrack frame. P9 measures it; the estimate is
  about 3 % while recording and 0.3 % with the gate closed.
- Whether a ring of 262,144 events a thread keeps a benchmark loop from dropping events before the
  consumer's wait adapts. `FrameRecorder` reports the drops and a test asserts none on a short run.
