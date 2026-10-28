# Profiler and log adoption

**Status:** Open — analysis written 2026-10-26; section 9 holds the decisions; implementation runs with every build at the end.

**Implements:** SUSHITRACK_PROFILING#P1, SUSHITRACK_PROFILING#P2, SUSHITRACK_PROFILING#P3, SUSHITRACK_PROFILING#P4, SUSHITRACK_PROFILING#P5, SUSHITRACK_PROFILING#P6, SUSHITRACK_PROFILING#P7

SushiTrack is read at commit `main`; SushiProfiler at `633cacd` (v0.1.0 not tagged yet);
SushiLog at tag `v0.2.3`, the version `cli/sushistack.deps.toml:17` pins. A line marked
"unverified" was not run or measured. Nothing in this folder was built, and no source file changed.

## 1. Profiling

### 1.1 What the code is today

SushiTrack has three timed paths and no record of any of them.

| Path | Where | What it measures | What it prints |
| --- | --- | --- | --- |
| C++ benchmark | `applications/benchmarks/mot_engine/mot_runner.cpp:65-78` | `steady_clock` around `adapter.update(dets, dt)` for each frame | `[ PERF <tracker> ] <sequence>: <us> us/frame (<n> frames)` at `main.cpp:192-194` |
| Python inference | `applications/inference/pipeline.py:399-436` | `time.time()` around detect, ReID, the `det.txt` write and the tracker call | `Processed N frames at X FPS (avg)` at `pipeline.py:458-459` |
| Benchmark tables | `docs/reference/BENCHMARKS.md:7-8` | the figures above, typed by hand | "The date, the commit and the machine of the runs were not recorded" |

The timed span in `mot_runner.cpp:68` is not `Tracker::update`. It is `SushiTrackAdapter::update`,
which also builds the object array and copies the activated tracks
(`applications/tracker_bridge/sushitrack_adapter.hpp:84-114`). Any figure a record replaces was
measured around that span, so the new frame zone must wrap the same span or the numbers do not
carry over.

The Python span at `pipeline.py:399-436` includes the `det.txt` write when `--mot-label` is on
(`pipeline.py:420-423`), so the printed FPS depends on disk speed. It excludes decode
(`pipeline.py:378-386`) and drawing (`pipeline.py:439-447`).

The stdout line is a protocol: `cli/sushitrack_cli/services/label.py:84-103` parses it,
`cli/sushitrack_cli/services/evaluate.py:55-62` turns it into FPS,
`applications/benchmarks/mot_engine/evaluator/trackeval_runner.py:165-169` turns FPS into
`Latency_us`, and `report_generator.py:121-124` and `:289-319` print and export it. A record
replaces the whole chain, not only `mot_runner.cpp`.

### 1.2 The frame as a period

`Tracker::update` runs once per frame (`include/SushiTrack/tracking/tracker.hpp`, called at
`source/c_api/sushitrack_c.cpp:538`). The library has no thread of its own
(`git grep` for `std::thread`, `std::async` and `omp` in `source/`, `include/` and
`applications/` finds only the two mutexes at `include/SushiTrack/filtering/kalman_filter.hpp:172`
and `include/SushiTrack/tracking/tracklet.hpp:421`), and `docs/architecture/OVERVIEW.md:472-476`
says one tracker serves one stream. One timeline therefore covers it.

SushiProfiler gives a period no end call: `PeriodTimeline::mark()` opens the next period and the
next mark closes it (`modules/capture/periods/README.md`, and the loop in
`modules/session/session/README.md` that marks once more after the last frame). A period "per
call of `update`" needs the host to mark after the last frame, or SushiProfiler needs a scoped
period (gap S2).

Which side marks decides what the record can compare:

- Marked by `mot_runner.cpp`, the same timeline covers SushiTrack, ByteTrack and OC-SORT
  (`main.cpp:99-111` builds all three adapters), so `sp record compare` reads one subject,
  `timeline:<name>`, for all trackers. ByteTrack and OC-SORT are third-party and take no zones.
- Marked inside `Tracker::update`, the period belongs to the library and a host that does not
  start a session still pays the mark. It also leaves out the adapter's marshalling
  (`sushitrack_adapter.hpp:84-114`).

This is decision 5.

### 1.3 Zones per stage

The pipeline has no decode and no detect stage in C++; both live in `applications/inference/`
(`pipeline.py:378-386` decode, `:402` detect). The C++ stages are the ten steps of
`Tracker::update` (`docs/architecture/OVERVIEW.md`, "Per-frame pipeline"):

| Stage zone | Code | Level |
| --- | --- | --- |
| `update` (whole call) | `source/tracking/tracker.cpp:96-271` | 1 |
| `split` (high and low detection pools) | `tracker.cpp:118-131` | 1 |
| `predict` | `tracker.cpp:143` or `:155-160`; body at `:279-299` | 1 |
| `match_high` | `tracker.cpp:185`; body at `:306-451` | 1 |
| `match_low` | `tracker.cpp:193`; body at `:459-501` | 1 |
| `lost_recovery` (only with ReID) | `tracker.cpp:203`; body at `:508-569` | 1 |
| `tentative` or `unconfirmed` | `tracker.cpp:224` and `:246`; bodies at `:577-700`, `:762-809` | 1 |
| `cleanup_lost` | `tracker.cpp:251`; body at `:815-850` | 1 |
| `finalize` (includes `remove_duplicate_tracklets`) | `tracker.cpp:253`; body at `:857-884`, duplicates `:966-1002` | 1 |
| `output` (sort and copy to the caller's buffer) | `source/c_api/sushitrack_c.cpp:80-146` | 1 |
| `solve` (one assignment) | `source/association/lapjv_solver.cpp:49-181` | 2 |
| `cost_matrix` (one `calculate`) | the calls at `tracker.cpp:336`, `:403`, `:468`, `:537`, `:599`, `:643`, `:770`, `:971` | 2 |

`decode`, `detect`, `reid` and `draw` exist only in `pipeline.py:378-447`. They are Python
stages; where they are recorded depends on decision 11. If inference moves to SushiAI and OpenCV
(`docs/backlog/README.md`, RW-23), `detect` is SushiAI's zone group `Device` and
`Evaluation` (SushiProfiler `CLIENTS.md`, "SushiAI"), not SushiTrack's.

Level 3 (per track or per pair) is not proposed. The pair loops sit inside the calculators
(`source/association/iou_cost_calculator.cpp:49-90`, `fused_cost_calculator.cpp:70-140`), and a
zone per pair would multiply the event count by the matrix size.

`primary_assoc_mode` 0 solves once per track age (`tracker.cpp:381-450`, default mode is 1 at
`sushitrack_c.cpp:215`), so a level 2 `solve` zone can fire dozens of times a frame in mode 0.

### 1.4 Counters, values and instants

Per frame, each readable at the line given, recorded as `set` counters unless noted:

| Subject | Where it is known |
| --- | --- |
| detections in, high pool, low pool | `tracker.cpp:127-131` |
| tracks in the pool, tentative, non-active | `tracker.cpp:163-165` |
| matches high stage (`add`) | `tracker.cpp:188-191` |
| tracks newly lost, low-pool leftovers | `tracker.cpp:196-198` |
| tracks recovered from Lost | `tracker.cpp:211-212` |
| tentative confirmed, removed, new initialised | `tracker.cpp:238-242` |
| tracks alive by state (tracked, lost, tentative) and removed this frame | `tracker.cpp:256-259` |
| output tracks written, tracks that did not fit | `sushitrack_c.cpp:103-112` |
| cost matrix size per stage | `tracker.cpp:338`, `:405`, `:470`, `:539` |

Instants: "track lost" (`tracker.cpp:492`), "track confirmed" (`:619`), "track removed"
(`:681`, `:793`, `:842`), "track recovered" (`:355`, `:426`, `:553`). They already log at DEBUG;
an instant costs one event and keeps the time, the log line does not survive a closed level.

A value (every sample kept) suits per-frame cost-matrix fill, the share of cells over the
threshold. It is optional.

Counters that need a figure the code does not compute today (matches of the low stage as a
number, `activated_tracklets` is shared across stages) need a size read before and after each
stage. The implementer reads `activated_tracklets.size()` around each call; no new state.

### 1.5 Flows and lanes across threads

None exists. The library is single-threaded (section 1.2), the C API takes one tracker per call
(`sushitrack_c.cpp:508-568`), and the Python pipeline runs decode, detect and track on one thread
(`pipeline.py:377-447`). A detection has no identity across frames: `Tracker::update` rebuilds a
`Tracklet` for each object each frame (`tracker.cpp:118-131`). Flows and lanes are not adopted.
If `st infer` later threads decode and detect apart from tracking, a frame becomes a flow, and one
thread serving several tracker instances becomes a lane.

### 1.6 Every timing printout and what a record does to it

| Existing | Where | Replaced by |
| --- | --- | --- |
| `RunResult::total_tracking_us`, `mean_us()` | `mot_runner.hpp:40-50`, `mot_runner.cpp:67-71` | the period or the `update` zone of the record; `RunResult` keeps `frames_processed` only |
| `[ PERF ]` line | `main.cpp:192-194` | a record file per run; the line may stay for one release (decision 9) |
| `_parse_perf` | `label.py:84-103` | a read of the record, swallows malformed lines today (`:100-101`) |
| `perf_fps` into `evaluate()` | `evaluate.py:55-62`, `trackeval_runner.py:133-169` | the record's `timeline:` median, with its interval |
| `Latency_us` and `FPS` columns | `report_generator.py:121-124`, `:172`, `:310`, `trackeval_runner.py:434-435` | a latency column from the record; `0.0` stops standing for "not measured" |
| Python `Processed N frames at X FPS` | `pipeline.py:458-459` | a record written by the pipeline, or nothing (decision 11) |
| Hand-typed tables | `docs/reference/BENCHMARKS.md`, `BENCHMARKS_IMM.md`, `BENCHMARKS_LINEAR_KALMAN.md` | tables generated from records plus TrackEval output (decision 10); closes `REMAINING_WORK.md` RW-19 |

RW-10, RW-11 and RW-12 (`docs/backlog/README.md`) are all "unmeasured" and each needs a
per-frame cost to compare; a record with `sp record compare` gives them that.

### 1.7 The Python side and P9

SushiProfiler writes no Python and has no C ABI before P9 (`PROFILER/README.md`, phase table).
Where that matters:

1. **Decode, detect, ReID and draw zones** are Python code (`pipeline.py:378-447`). With no
   Python zone call they cannot enter a record. The record format is published and a Python
   program may write a record by hand (`docs/reference/RECORD_FORMAT.md`; `RECORD_AND_ANALYSES.md`
   says so for "SushiTrack's Python pipeline"), but each key, id table and zone tree is then a
   second copy of SushiProfiler's format in this repository.
2. **The tracker zones** run inside `sushitrack.dll`, loaded by ctypes
   (`bindings/python/sushitrack.py:367`). A session started in the host exe cannot see them (gap
   S1); a Python process has no exe-side profiler at all.
3. **A C export of session control** would let Python start and stop a session inside the DLL
   without P9. It is a new public C surface (decision 4), and a Python stage zone would pass
   `perf_counter_ns` values to an external-zone call. Whether `perf_counter_ns` and
   `std::chrono::steady_clock` share a base on Windows and Linux is unverified.
4. **`tracker_bridge.py`** takes the same route (`applications/inference/tracker_bridge.py:47-50`
   for the binding, `:53-60` for the C shim); the shim `tracker_bridge.dll` is a second C surface
   (`tracker_bridge.h`).
5. **Reading records from Python**, for tables, is possible today because the format is JSON, but
   the statistics (median, bootstrap interval) are C++ (`modules/record/statistics`); a Python
   reader would recompute them or parse `sp record show` text (gap S8).

Before P9, the Python path can do only these: nothing, a hand-written Python record, or
tracker-only records through a SushiTrack C export. Decision 11.

### 1.8 Overhead

Budget rows come from `PROFILER/README.md`, "Performance budget": gate closed under 2 ns a
zone, recording under 25 ns a zone, no instruction when compiled out. Frame costs come from
`docs/reference/BENCHMARKS.md`: linear Kalman, DanceTrack 18 us, MOT17 37 us, MOT20 318 us per
frame, IMM overall 218 us.

Arithmetic, not a measurement: ten stage zones and about twelve counters are about 22 events a
frame, 0.55 us when recording and 0.05 us with the gate closed. That is 3.0 % of an 18 us
DanceTrack frame while recording and 0.3 % with the gate closed; 0.2 % on MOT17; under 0.2 % on
MOT20. The SushiAI row in the budget is "under 1 % slower"; DanceTrack recording would miss it by
this arithmetic. The fix inside the budget is fewer counters at level 1, not fewer zones.
Measuring it is task T9 in `PLAN.md`.

Ring size: the default is 65,536 events a thread (`CONFIG_FORMAT.md`). At 22 events a frame a
2000-frame MOT20 sequence writes about 44,000 events, which fits; a 54 ms fill at DanceTrack's
frame rate (S9) depends on the consumer's wait, which doubles while the rings are under a quarter
full. The record counts drops per thread. Whether a benchmark loop overflows before the consumer
adapts is unverified; the run sets `buffers.ring_events` and a check asserts zero drops
(decision 8, task T9).

`-ffast-math` and `/fp:fast` are on for every target (`cmake/SushiTrackBuildPolicy.cmake:28`,
`:31`). That does not affect the profiler's integer clock reads.

## 2. Logging: where the code is silent

Rules applied: `logging` skill, "What is logged", "What is not logged", "Errors and silence".
SushiTrack has one project and one category, `sushitrack`
(`include/SushiTrack/logging/sushitrack_log_category.hpp:36-41`, a test pins the names at
`tests/logging/test_sushitrack_log_category.cpp:38`). Every proposed record below is under it;
decision 13 asks whether the areas get categories of their own.

What is already well covered, so the implementer does not duplicate it: the C API catches
`bad_alloc`, `std::exception` and `...` and logs each at ERROR
(`sushitrack_c.cpp:452-469`, `:541-567`, `:590-614`); a failed config parse logs ERROR
(`config_loader.cpp:309-321`); lifecycle events log at DEBUG
(`tracker.cpp:355`, `:426`, `:492`, `:553`, `:619`, `:683-692`, `:719`, `:745`, `:784`, `:793`,
`:807`, `:844`); an overflowing output buffer logs WARN (`sushitrack_c.cpp:103-110`).

Gap ids `L1` to `L5` refer to section 3.

### 2.1 C++ library

| ID | Where | What is silent | Record to make | Needs |
| --- | --- | --- | --- | --- |
| C1 | `config_loader.cpp:192-199`, `:286-291` | An explicit path that does not exist returns "", and the caller logs only the INFO "No configuration file found -- using provided parameters". The caller asked for a file. A test pins the success return (`tests/configuration/test_config_loader.cpp:506`) | WARN: `configuration file {path} does not exist; using the parameters given` | |
| C2 | `config_loader.cpp:201-206` | `SUSHITRACK_CONFIG` set to a missing file falls through to the directory search without a word | WARN once: `SUSHITRACK_CONFIG={value} does not exist; searching from the current directory` | L2 |
| C3 | `config_loader.cpp:209` | `fs::current_path(ec)` ignores `ec`; the search then ends empty | DEBUG: the error code and that the search ended | |
| C4 | `config_loader.cpp:47-74`, `:77-178` | A value of the wrong kind returns the default (`:63`, `:71`); a float for an int is truncated (`:61`); a key the code does not read is never examined, so a typo is accepted | WARN per key: `{path}: expected a number, got {kind}; keeping {default}`; WARN per unknown key | decision 12 |
| C5 | `config_loader.cpp:329-335` | `sushitrack_create_from_config` drops the status of `sushitrack_load_params_from_json` (`:333`). The ERROR logged inside says the parse failed; the tracker is then built from defaults with no record of that | WARN: `configuration {path} failed to load; building the tracker from default parameters` | |
| C6 | `config_loader.cpp:237-259` | `load_config_from_json` returns false at `:242`, `:249` and `:258` and the `catch` at `:256` records nothing. It has no caller (`git grep load_config_from_json` finds the declaration and the definition) | ERROR at the catch, or delete the function | decision 15 |
| C7 | `sushitrack_c.cpp:255-259`, `:513-518`, `:578-582` | `INVALID_ARGUMENT` is returned from three entry points with no record; `sushitrack_create` (`:240-241`) turns any failure status into a null handle | ERROR: `sushitrack_update refused: tracker is null` (name the argument) | L2 |
| C8 | `sushitrack_c.cpp:319-321`, `kalman_filter_registry.hpp:78-88`, `generic_kalman_factory.cpp:50-53`, `tracklet.cpp:111`, `:139` | An unregistered `kalman_type` with Mahalanobis gating off is not checked at create. The registry logs ERROR each call and returns null; `activate` then calls `kf_->initiate` on a null pointer. This is a crash, not only a missing record. The gating-on path warns once at `:336-337` and then fails the same way | At create: ERROR and return `INVALID_ARGUMENT`; the registry ERROR stays | decision 13 |
| C9 | `sushitrack_c.cpp:348-367` | An `iou_type` other than 1, 2, 3 falls back to plain IoU | WARN: `iou_type {n} is unknown; using plain IoU` | |
| C10 | `tracker.cpp:101`, `:832-834` | `frame_rate` below 1 and `track_buffer` below 1 are clamped to 1 | WARN at create naming the value and the clamp | |
| C11 | `sushitrack_c.cpp:531-532`, `cosine_cost_calculator.cpp:52`, `:71-72`, `pure_reid_cost_calculator.cpp:68-69`, `:81-82`, `:115-117`, `fused_cost_calculator.cpp:126-128`, `:189-191` | With `enable_reid` on, a detection with no feature is accepted and every appearance cost for it is a sentinel; matching drops to IoU with no record. With `enable_reid` off, a supplied feature is dropped (`:531`) | WARN once per tracker: `enable_reid is on but {n} of {m} detections carry no feature`; a counter per frame | L2 |
| C12 | `tracklet.cpp:434-435`, `:446`, `:406-409`, `:56-62` | A feature of another length than the history is skipped in the similarity; a changed length resets the smoothed feature; a zero-norm feature is stored unnormalised | WARN once per pair of lengths: `feature length {a} does not match the track's {b}; appearance ignored` | L2 |
| C13 | `kalman_filter.cpp:180`, `:222`, `:253` | Three conditioning failures warn on every call, once per track pair per frame; the same regularisation at `:312-315` and `:362-365` and in `imm_filter.cpp:283-287`, `:494-499` is silent. A matrix that fails to factor on one track floods the log; on another path it is invisible | One WARN wording, rate limited, naming the filter; a counter per frame | L2 |
| C14 | `imm_filter.cpp:523-530` | Mode probabilities that sum to nearly zero are reset to the prior | WARN rate limited: `IMM likelihoods collapsed; mode probabilities reset to the prior` | L2 |
| C15 | `lapjv_solver.cpp:54-65` | `n_rows` and `n_cols` above zero with an empty matrix returns everything unmatched, which hides a size mismatch between the matrix and the counts | ERROR: `cost matrix is empty for {r} x {c}` | |
| C16 | `lapjv_solver.cpp:79` | A NaN cost never satisfies `<= thresh`, so the pair is silently unmatched. Whether the NaN survives the fast-math build is unverified | WARN rate limited when a frame holds non-finite costs; a counter | L2 |
| C17 | `tracker.cpp:980-987`, `:990-1001` | `remove_duplicate_tracklets` drops a track from the pools without marking it removed. Every other lifecycle exit logs (section 2 intro) | DEBUG: `track {id} dropped as a duplicate of {id}` | L3 |
| C18 | `tracker.cpp:658-662` | The low-pool tentative confirmation has no DEBUG; the high-pool one has (`:619`) | DEBUG: `tentative track {id} confirmed` | |
| C19 | `sushitrack_c.cpp:103-110` | The overflow WARN fires on every frame while the buffer is too small | WARN rate limited, with the count of frames affected | L2 |
| C20 | `kalman_filter_registry.hpp:67-70`, `applications/tracker_bridge/tracker_registry.hpp:58-61` | A second registration under one id replaces the first | WARN once: `kalman type {n} registered twice; the later one wins` | |
| C21 | `tracker.cpp` frame and every DEBUG at `:355`, `:426`, `:492`... | The DEBUG lines name a track but not the frame; the frame appears only in the TRACE at `:104-106` | Carry the frame id on every record from one `update` | L3 |

### 2.2 C++ applications

| ID | Where | What is silent | Record to make |
| --- | --- | --- | --- |
| C22 | `tracker_bridge.cpp:72-77` | `create_tracker` returns null for a null or unknown name | ERROR naming the name; the registry knows the registered names |
| C23 | `tracker_bridge.cpp:84-114` | `tracker_process_c` returns `void`. It returns on a null argument (`:87-91`), truncates at `max_results` without a word (`:109-110`), and has no `try` around `update` (`:104-105`) although the adapters throw (`sushitrack_adapter.hpp:99`, `:124`), so an exception crosses an `extern "C"` boundary | ERROR on each refusal and each exception; a status return or an error out-parameter (decision 14) |
| C24 | `sushitrack_adapter.hpp:55-58` | A failed load resets the parameters to defaults; the library logged the parse error, the adapter logs nothing about the fallback | WARN: `using default parameters` |
| C25 | `byte_track_adapter.hpp:68-90`, `oc_sort_adapter.hpp:68-85`, `json_settings.hpp:47-48`, `:64-69` | ByteTrack prints "config not found" on stdout, OC-SORT says nothing when its file is missing, a file without its key says nothing in either (`byte_track_adapter.hpp:72-84`), a missing file returns an empty object (`json_settings.hpp:47-48`) and a wrong-typed number falls back (`:66-68`) | One policy for the three adapters, on one stream (decision 14) |
| C26 | `mot_sequence.hpp:47-79` | An unreadable `det.txt` returns an empty map (`:50` never tests `is_open`), a bad token ends the row (`:63-64`), a short row is skipped (`:67-68`) | WARN with the file, the line count read and the rows skipped |
| C27 | `mot_sequence.hpp:85-100` | A missing `seqinfo.ini` or `frameRate` falls back to 30 (`mot_runner.cpp:48`), which sets `dt` for the whole run | WARN naming the file and the fallback |
| C28 | `mot_runner.cpp:53-54`, `main.cpp:192-194`, `evaluate.py:55-62` | A sequence with no detections returns a zero `RunResult`; `main.cpp` prints `0 us/frame (0 frames)` as a successful run; `evaluate.py:59-60` divides by that zero, and the `except Exception` at `:63-65` reports "Evaluation failed: float division by zero" | WARN `{sequence}: no detections; skipped`, and a non-zero exit or no PERF line |
| C29 | `main.cpp:117`, `:175` | `directory_iterator` can throw outside the `try` at `:188` | ERROR with the path |

### 2.3 Python

The Python rules (`logging` skill, "Python") make a library module log through
`logging.getLogger(__name__)`; a command shows output through the console layer, which
`tools/settings.py:24` names for `applications/` and `cli/sushitrack_cli/console.py`. No Python
file in this repository calls `logging`. `bindings/python/sushitrack.py` is the library; the
`applications/` scripts are entry points.

| ID | Where | What is silent | Record to make | Needs |
| --- | --- | --- | --- | --- |
| P1 | `bindings/python/sushitrack.py:262-267`, `:276-331` | `_resolve_fn` returns `None` for a symbol that is not exported, including the aliases `sushitrack_c.h` does not declare, such as `sushitrack_Create`; the failure surfaces at the first `update` as `'NoneType' object is not callable` (`:445`) | ERROR at load naming the missing required symbols | |
| P2 | `sushitrack.py:231-259` | The library is taken from an argument, `SUSHITRACK_LIB`, the module folder or the build tree, in that order; which one won is not recorded | DEBUG: `loaded {path}` | |
| P3 | `sushitrack.py:371-376`, `config_loader.cpp:289` | `Tracker(config="path")` with a missing file succeeds, because the native side logs INFO and returns success (C1); Python believes the file loaded | follows C1; Python adds nothing | |
| P4 | `sushitrack.py:388-392` | `field_type(value).value` truncates a float override to an int field (`frame_rate=29.97` becomes 29) | WARN: `{key}: {value} narrowed to {int}` | |
| P5 | `sushitrack.py:448-450` | `BUFFER_OVERFLOW` is accepted and the truncated list returned; the native WARN goes to stderr or the sink, the Python caller sees nothing | WARN rate limited from Python, or a flag on the returned list | L1, L2 |
| P6 | `sushitrack.py:491-498` | A callback exception inside the native thread is printed by ctypes and lost; `LogLevel(value.level)` raises on an unknown level | catch and write to `sys.stderr` once | L1 |
| P7 | `applications/inference/pipeline.py:65-66` | A CUDA provider that is missing makes onnxruntime run on CPU; `session.get_providers()` is never read | WARN: `requested CUDAExecutionProvider, running on {providers}` | |
| P8 | `pipeline.py:162-165`, `demo.py:48` | `--ckpt` defaults to `None`; the detector then runs with randomly initialised weights and says nothing | WARN: `no checkpoint; weights are randomly initialised` | |
| P9 | `pipeline.py:231-234` | Every class but 0 is dropped | INFO once at construction: `keeping class 0 only` | |
| P10 | `pipeline.py:78-79`, `:413-414` | An empty or degenerate crop becomes a black image and its embedding enters the track history | counter per interval; WARN once | L2 |
| P11 | `pipeline.py:301`, `:303`, `:316-319`, `:355-356` | The frame rate becomes 30 and the image folder falls back without a record when `seqinfo.ini` or the camera property is missing | WARN naming the fallback | |
| P12 | `pipeline.py:378-389` | An image that fails `cv2.imread` ends the loop as if the sequence ended; `total_frames` is known (`:336`) and compared with nothing. The run prints "Processed N frames" and success | ERROR: `stopped at frame {n} of {m}: cannot read {path}` | |
| P13 | `pipeline.py:431-433` | `except TypeError` retries without embeddings on any `TypeError`, including one raised inside the tracker; ReID turns off silently | WARN once; narrow the `except` | L2 |
| P14 | `pipeline.py:458-459`, `demo.py:128` | Zero frames prints "0.00 FPS" and "completed without errors" | WARN when `frame_id == 0` | |
| P15 | `demo.py:130-132`, `evaluate.py:63-65` | A broad `except` prints the message and exits; the traceback is lost | ERROR with `exc_info`, then the console line | |
| P16 | `applications/inference/tracker_bridge.py:109-113`, `:137-146`, `:148`, `tracker_bridge.cpp:109-110` | An embedding list shorter than the detection list drops features silently; the shim's result buffer is `max(2n, 100)` and the C side truncates silently | WARN once | L2 |
| P17 | `tracker_bridge.py:123-124` | An empty frame skips `update` on the shim path, so lost tracks never age; the binding path (`:105-119`) does call it. This is a behaviour difference, not only a missing log | DEBUG now; the divergence goes to the owner (decision 14) | |
| P18 | `evaluator/data_loader.py:58-59`, `:68-69`, `:104-105`, `:110-111` | A missing file returns `{}`, a short row is skipped, with no count | WARN with counts (the other loader already reports counts, `trackeval_runner.py:260-264`) | |
| P19 | `trackeval_runner.py:434-435` | A missing timing is stored as `0.0`, exported (`report_generator.py:289-319`) and printed as a latency | a missing value, and WARN | |
| P20 | `cli/sushitrack_cli/services/label.py:100-101` | A malformed PERF line is skipped; a format change drops every timing | `console.warn` per skipped line | |
| P21 | `cli/sushitrack_cli/config.py:178-179`, `:188-192`, `env.py:82-83` | A bad int keeps the current value; an unknown key in a config file is ignored; a cache folder that cannot be made is skipped | `console.warn` (output, not a log record) | |

`applications/inference/messages.py` and `applications/benchmarks/mot_engine/evaluator/messages.py`
differ only in their docstring (checked with `diff`): one function set, two copies.

### 2.4 Silent on purpose, not logged

`Tracklet::re_activate` falling back to a plain update when the gap is under two frames or no
checkpoint exists (`tracklet.cpp:527-528`) is the designed path. Per-frame, per-track and
per-pair events (the matches, the `solve` calls) stay counters and TRACE.

## 3. SushiLog gaps

Read from `D:/Projects/sushilog` at `v0.2.3`: `docs/guides/LOGGING_GUIDE.md`,
`modules/foundation/log/include/SushiLog/log/log_record.hpp`, `log_c_sink.h`, `log_calls.hpp`,
`log_front.hpp`.

| ID | What SushiLog lacks | Evidence | Needed by |
| --- | --- | --- | --- |
| L1 | **A Python binding.** The C interface has `sl_set_log_sink`, `sl_set_log_synchronous` and `sl_forward_record` (`log_c_sink.h:76`, `:82`, `:88`) and nothing to set or read a level, register a category or emit a record. `bindings/python/sushitrack.py:182-204` copies `SushiLogRecord` by hand and `:334-342` loads the library by name. A Python program can neither write into SushiLog's stream nor set a project level except through the environment variable. A package would add `sl_set_log_level(project, level)`, `sl_log_level`, a runtime category (`make_log_category` is constexpr, `log_front.hpp`), `sl_log(...)`, and a `logging.Handler` in both directions | `LOGGING_GUIDE.md` "Sending records elsewhere" | P5, P6, P7 to P16 if Python records go to one stream |
| L2 | **A rate limit or log-once.** `SL_LOG_FRONT_AT` has no per-site state besides the level slot (`log_front.hpp`). The logging skill sends recurring events to a counter reported once per interval, and SushiLog has no counter to report. A `log once` and a `log at most every N` call site, each ending with an "N suppressed" line like the queue-drop WARN it already writes, would serve | `LOGGING_GUIDE.md` "Where records go" (drop WARN) | C2, C7, C11, C12, C13, C14, C16, C19, P5, P10, P13, P16 |
| L3 | **A scoped context field.** `LogRecord` holds level, project, module, file, line, thread, time and one message (`log_record.hpp:39-52`). A tracker wants the frame id on every record made inside one `update`; today every call passes it by hand, and most do not (C21). A thread-local scope that SushiLog prints and hands the sink as a field would cover it. Whether to add structured key-value fields instead is the owner's call, not argued here | `log_record.hpp` | C17, C21 |
| L4 | **A record count by level.** The number of WARN and ERROR records since start, and the drop count, can be read only by a sink that counts them (`tests/captured_log.hpp` does, for tests). A benchmark that wants to write "this run logged 0 warnings" into its record has no query | `LOGGING_GUIDE.md` | `mot_engine`, nightly gate (RW-24) |
| L5 | **A shipped capture sink for tests.** `tests/captured_log.hpp` is SushiTrack's own class. Whether SushiRuntime, SushiAI and SushiDSP copy it was not checked | `tests/captured_log.hpp` | tests of every row of section 2 |

## 4. SushiProfiler gaps

Read from `D:/Projects/sushiprofiler` at `633cacd`: `README.md`, `docs/design/PROFILER/*.md`,
the module READMEs named in the task, `docs/reference/RECORD_FORMAT.md`,
`docs/reference/CONFIG_FORMAT.md`, `cmake/SushiProfilerLibrary.cmake`.

| ID | What v0.1.0 lacks | Evidence | Tied to |
| --- | --- | --- | --- |
| S1 | **One collector per image.** `Collector` is the one process-wide object and the library builds static (`modules/capture/collector/README.md`, `cmake/SushiProfilerLibrary.cmake`, design open question 3). `sushitrack` is a shared library with hidden symbols (`source/CMakeLists.txt:12`, `cmake/SushiTrackBuildPolicy.cmake:24-25`). Zones inside `sushitrack.dll` write to the DLL's collector; a `Session` in `mot_engine.exe` opens the exe's. They do not meet. Options are in decision 3 | `CAPTURE_AND_SESSION.md` "The hot path" | every zone in 1.3; `mot_runner.cpp`; `sushitrack.py:367` |
| S2 | **A scoped period.** `mark()` opens the next period; the last needs one more mark. A call-shaped period needs a RAII form or an `end` | `modules/capture/periods/README.md` | `mot_runner.cpp:65-78`, `Tracker::update` |
| S3 | **No C ABI, no Python record writer, no Python zone call** (P9), and no run-time site registration for a caller whose names are not literals | `PROFILER/README.md` phases; `zones/README.md` | `pipeline.py`, `sushitrack.py`, `tracker_bridge.py` |
| S4 | **No date, commit or hostname fact.** `host_facts()` gives `os`, `cpu` and `hardware_threads`; a contributor may add the rest (`modules/session/environment/README.md`). The commit is the library's own to supply, the date is not any one library's | `RW-19`, `BENCHMARKS.md:7-8` | `EnvironmentContributor` of SushiTrack |
| S5 | **No table writer that a program can read.** The record writers are `terminal` and `record` (`modules/record/record_writers/README.md`); `sp record show` prints text. `report_generator.py` needs the medians and intervals, and either parses text or recomputes C++ statistics in Python | `report_generator.py:103-135`, `:289-319` | decision 10 |
| S6 | **"Record every period".** `window.active` runs 1 to 2^32 - 1 (`CONFIG_FORMAT.md`); a sequence of unknown length takes the maximum | `CONFIG_FORMAT.md` | `mot_engine` config |
| S7 | **A second `Session::start` after `stop` in one process is not stated.** `mot_engine` loops trackers by sequences (`main.cpp:178-202`); one record per pair needs it. Unverified | `modules/session/session/README.md` "Start" | decision 8 |
| S8 | **No run-level tag.** The record has a `tags` table "from the configuration" (`CAPTURE_AND_SESSION.md`), but `ProfilerConfig` has no tags field (`modules/session/config/include/SushiProfiler/config/`, `CONFIG_FORMAT.md`). The tracker and the sequence can be period tags only, and `PeriodTimeline::tag` takes a lock (`periods/README.md`), once per frame in a one-record run | `CONFIG_FORMAT.md` | decision 8 |
| S9 | **Ring and drain for a burst.** The wait between drains is 1 ms while a ring is over a quarter full and doubles up to 1 s while every ring is under a quarter (`modules/session/consumer/README.md`). A DanceTrack frame of 18 us with 22 events is about 1.2 million events a second, which fills a ring of 65,536 in about 54 ms; the wait would have to stay below that. Unverified | `CAPTURE_AND_SESSION.md` "The session" | 1.8, task T9 |
| S10 | **`device_idle`, `top_spans`, `roofline` and the span sources do not apply**; SushiTrack records no spans. Not a gap, listed so nobody builds one | `RECORD_AND_ANALYSES.md` | |

## 5. Decisions for the owner

Each is a question. The options are not ranked.

1. **Where does the intent live?** (a) A new design topic `SUSHITRACK_PROFILING` with a phase
   table that carries P8; (b) a phase added to `SUSHITRACK_MIGRATION`; (c) backlog entries in
   `REMAINING_WORK.md` only. Also: before or after the `v0.1.0` release (RW-22), which RW-17 and
   RW-23 already block.
2. **Is SushiProfiler required or optional?** (a) Optional: a CMake option, on by default; off,
   the macros expand to nothing and the library builds without SushiProfiler present (as
   `PROFILER/CLIENTS.md` proposes for every library); `[sushisystems.sushiprofiler]` sits in
   `cli/sushistack.deps.toml` either way. (b) Required, like SushiLog (`depends_on` at
   `cli/sushistack.deps.toml:12`).
3. **Who owns the collector?** (a) `sushitrack.dll` embeds the static profiler, and session
   control is exported from the DLL (decision 4); (b) hosts that profile link `sushitrack_static`
   (`source/CMakeLists.txt:24-34`) so one image holds one collector, and the DLL path records
   nothing; (c) SushiProfiler adds a shared build and SushiTrack links it; (d) the library records
   nothing and only the host's own zones exist.
4. **Do we add profiling calls to the public C ABI** (`sushitrack_c.h`)? (a) Start, mark, zone and
   stop calls; (b) a single `sushitrack_profile_set_config` that takes a JSON string; (c) none,
   and the Python path waits for SushiProfiler P9. Either of (a) and (b) is an `api-stability`
   change.
5. **Who marks the frame period?** (a) `mot_runner.cpp` and `pipeline.py`, the host; the library
   records zones and counters only; (b) `Tracker::update`; (c) both, on two timelines.
6. **Names the records carry.** The public macro prefix (`SUSHITRACK_ZONE` beside
   `SUSHITRACK_LOG_*`, or `ST_ZONE`), the compiled-level option, the timeline name, the zone
   groups (`Associate`, `Filter`, ...), and where the attribute and counter keys are listed (a
   page like SushiProfiler's `PROFILER_KEYS.md`). They are a contract with the analyses.
7. **Where does the profiling front live?** (a) A new area `include/SushiTrack/profiling/` with
   `profiling.hpp` beside `logging/logger.hpp`; (b) inside `tracking/`. And where does the
   `EnvironmentContributor` (version, commit, date) live: (a) the library, which then includes
   the session tier of SushiProfiler; (b) the application that starts the session, so the library
   includes only the capture tier.
8. **Record granularity and place.** (a) One record per tracker and sequence, a session each;
   (b) one record per `mot_engine` run, period tags for tracker and sequence. The output folder
   (`build/profiles/` as SushiAI uses, or under `applications/benchmarks/<benchmark>/`), whether a
   record is committed or ignored, and whether the nightly job (RW-24) runs `sp record check`
   with a budget file.
9. **Does the `[ PERF ]` line stay?** (a) Removed with the first record; (b) kept for one release
   beside the record; and does `st eval` call `sp`, adding a tool dependency to the CLI, or read the
   record file itself.
10. **How are the benchmark tables made?** (a) A SushiTrack tool joins records and TrackEval
    results into `docs/reference/BENCHMARKS*.md`; (b) SushiProfiler gains a JSON or CSV table
    writer (S5) and the join stays in `report_generator.py`; (c) the tables stay by hand with a
    record link beside each (RW-19's weaker reading).
11. **The Python pipeline.** (a) No records until P9; (b) a hand-written Python record by the
    published format; (c) tracker-only records through a C export (decision 4) with external zones
    for the Python stages; (d) RW-23 moves inference to SushiAI and OpenCV and the question
    disappears. Which of these, and does `st infer` mark a SushiTrack timeline or SushiAI's
    `inference.batch`?
12. **How strict is the configuration reader?** (a) Warn on a wrong-typed value and on an unknown
    key, keep the default; (b) refuse both with `INVALID_ARGUMENT`, as SushiProfiler's reader does.
    The same question covers C1 (a missing explicit file: warn and succeed, or fail) and C8 (an
    unregistered `kalman_type` at create: refuse). A test pins today's success
    (`test_config_loader.cpp:506`).
13. **Log categories.** Keep the one `sushitrack` category, or declare one per area
    (`tracking`, `association`, `filtering`, `configuration`, `c_api`) so
    `SUSHITRACK_LOG_LEVEL=warn,association=trace` works. `tests/logging/test_sushitrack_log_category.cpp:38`
    pins the current names.
14. **The tracker bridge.** (a) Applications link SushiLog and log through the project macros;
    (b) they keep the console, which `tools/settings.py:24` allows, on one policy; and does
    `tracker_process_c` gain a status return (a change to a shared library that
    `tracker_bridge.py:89-97` binds)? Also which behaviour the empty-frame shortcut at
    `tracker_bridge.py:123-124` should have.
15. **Delete or fix `load_config_from_json`.** It is exported with `SUSHITRACK_API`
    (`config_loader.hpp:51`) and has no caller.
16. **Recurring warnings before SushiLog has a rate limit.** (a) Wait for SushiLog L2 and bump the
    pin; (b) a counter in `Tracker` reported as one line every N frames, which duplicates a
    SushiLog feature.
17. **Overhead policy.** Is level 1 compiled into release builds by default, and is "under 1 %"
    the SushiTrack budget (1.8 estimates 3 % on DanceTrack while recording)?
18. **Python tests and logging home.** There is no pytest configuration outside `cli/`
    (`cli/pyproject.toml:42`), and the Python binding's home is RW-3. Where do tests for the
    binding, the inference scripts and the evaluator live, and does the Python logging helper
    (L1) live in SushiLog or in this repository?

## 6. What this analysis saw and did not change

SOLID and dependency observations, none fixed here:

- `Tracker::update` increments `frame_id_` (`tracker.cpp:108`) and mutates tracks before
  `LAPJVSolver` can throw (`lapjv_solver.cpp:228`, `:248`), so a thrown `update` leaves the
  tracker half advanced; the C API reports it (`sushitrack_c.cpp:550-557`) and the next call
  proceeds on that state.
- `config_loader.cpp` holds path resolution, JSON overlay and three C entry points in one file;
  `overlay` at `:76-178` is a 100-line function over twelve sections.
- `include/SushiTrack/configuration/config_loader.hpp:43`, `:51` export `resolve_config_path` and
  `load_config_from_json` as public API; the second is dead (C6).
- `applications/tracker_bridge/sushitrack_adapter.hpp:132` fixes a 4096-track buffer, a number
  `mot_engine` depends on; an overflow throws (`:97-102`) rather than growing.
- `applications/inference/messages.py` and `applications/benchmarks/mot_engine/evaluator/messages.py`
  are the same file twice.
- `cli/sushitrack_cli/services/evaluate.py:55-62` and `label.py:84-103` couple the CLI to the
  text of a benchmark's stdout.
- `sushitrack.py:276-331` keeps alias names that `sushitrack_c.h` does not declare
  (`sushitrack_Create`, `sushitrack_GetDefaultParams`, `sushitrack_CreateFromConfig`), a
  dependency on a past API.
- `Tracklet::predict` takes a mutex per track per frame (`tracklet.cpp:229`) in a library the
  documentation calls single-stream (`OVERVIEW.md:472-476`); the cost is not measured here.
- `DISABLED_` tests at `tests/c_api/test_sushitrack_c/test_error_handling.cpp:473`, `:483`, `:494`
  leave three status codes untested.

Not covered: the `third_party/` code (ByteTrack, OC-SORT, YOLOX, TrackEval), `cli/` outside the
files cited, `tools/`, `.github/`, `cmake/SushiTrackSanitizers.cmake`, the guides under
`docs/guides/`, the other cost calculators line by line, and any run or measurement.

## 9. Decisions

The owner delegated every question of section 5 to the orchestrator on 2026-10-26, asking for the
engineering choice that holds as the stack grows, all code written first and every build at the end.

| # | Decision |
| --- | --- |
| 1 | (a) A design topic `SUSHITRACK_PROFILING` carrying P8; it does not wait for the v0.1.0 release |
| 2 | (a) Optional, a CMake option on by default; the pin sits in `cli/sushistack.deps.toml` either way |
| 3 | (c) SushiProfiler v0.2.0 is a shared library; SushiTrack links it |
| 4 | (c) No profiling call in the C ABI; the Python path waits for SushiProfiler P9 |
| 5 | (a) The host marks the frame period (`mot_runner.cpp`); the library records zones and counters only |
| 6 | `SUSHITRACK_ZONE` beside `SUSHITRACK_LOG_*`; option `SUSHITRACK_PROFILE_LEVEL`; timeline `sushitrack.frame`; one group per area (`Associate`, `Filter`, `Detect`, `Track`); keys listed in `docs/reference/PROFILER_KEYS.md` |
| 7 | (a) `include/SushiTrack/profiling/profiling.hpp` beside `logging/logger.hpp`; the contributor lives in the library |
| 8 | (a) One record per tracker and sequence, a session each, under `build/profiles/`, ignored by git; the nightly check waits for RW-24 |
| 9 | (b) `[ PERF ]` stays for one release beside the record; `st eval` reads the record file itself, with no tool dependency on `sp` |
| 10 | (c) The tables stay by hand, each with a link to its record, until SushiProfiler RW-18 writes tables |
| 11 | (a) No Python records until P9 |
| 12 | (b) The reader refuses a wrong-typed value, an unknown key, a missing explicit file and an unregistered `kalman_type` with `INVALID_ARGUMENT`; the test pinning the old success changes |
| 13 | One category per area: `tracking`, `association`, `filtering`, `configuration`, `c_api` |
| 14 | (a) Applications log through the project macros; `tracker_process_c` returns a status; an empty frame still runs the tracker's update, so tracks age |
| 15 | Delete `load_config_from_json`, a breaking entry |
| 16 | (b) A counter in `Tracker` reported as one line every N frames, until SushiLog RW-3 and RW-4 |
| 17 | Level 1 in release and 2 in debug; the budget is under 1 % with the gate closed |
| 18 | Python tests wait for RW-3's binding home; Python logging is SushiLog RW-12's |
