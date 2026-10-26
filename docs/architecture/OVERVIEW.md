# SushiTrack Architecture

This document describes the internal structure and features of SushiTrack: the
per-frame processing pipeline, the interfaces that compose it, the motion
estimators, the association stages, the track lifecycle, and the configuration
and integration surfaces. It is a reference for engineers reading or extending
the code. The compiled defaults are those set by `sushitrack_get_default_params`
(`source/c_api/sushitrack_c.cpp`) and mirrored in `TrackerConfig` (`include/SushiTrack/tracking/tracker_config.hpp`).

## Contents

- [Scope](#scope)
- [Per-frame pipeline](#per-frame-pipeline)
- [Interfaces and wiring](#interfaces-and-wiring)
- [Data model](#data-model)
- [Coordinate systems](#coordinate-systems)
- [Motion estimators](#motion-estimators)
- [Association](#association)
  - [Two-stage BYTE association](#two-stage-byte-association)
  - [Primary association mode](#primary-association-mode)
  - [Cost calculators](#cost-calculators)
  - [Score fusion](#score-fusion)
  - [Mahalanobis gating](#mahalanobis-gating)
  - [Appearance / ReID](#appearance--reid)
  - [Observation-centric momentum](#observation-centric-momentum)
- [Assignment solver](#assignment-solver)
- [Track lifecycle](#track-lifecycle)
  - [States](#states)
  - [Tentative confirmation](#tentative-confirmation)
  - [Lost buffer and expiry](#lost-buffer-and-expiry)
  - [Duplicate suppression](#duplicate-suppression)
  - [Initialization suppression](#initialization-suppression)
  - [Observation-centric re-update](#observation-centric-re-update)
- [Timing policies](#timing-policies)
- [Configuration system](#configuration-system)
- [C API and bindings](#c-api-and-bindings)
- [Logging](#logging)
- [Profiling](#profiling)
- [Thread safety](#thread-safety)
- [Testing and evaluation](#testing-and-evaluation)
- [Future work](#future-work)

---

## Scope

SushiTrack is a C++17 multi-object tracking library. It consumes per-frame
bounding-box detections (optionally with an appearance embedding) and produces
persistent track identities. The pipeline follows the ByteTrack two-stage
association design and extends it with optional appearance matching, statistical
gating, alternative motion models, and observation-centric re-ranking and
re-update terms.

The library builds as a shared object and exposes a stable C ABI
(`include/SushiTrack/c_api/sushitrack_c.h`). A Python `ctypes` binding, a MOTChallenge/DanceTrack
benchmark, a GoogleTest suite that mirrors the headers, and a YOLOX video
inference pipeline are included.

Algorithm components are selected at runtime through configuration flags. The
default (compiled) configuration runs the core path: two-stage IoU/DIoU
association, Kalman prediction, lost-track buffering, and LAPJV assignment. The
following are optional and off in the compiled defaults: the tentative
lifecycle, appearance ReID, Mahalanobis gating, the NSA and IMM estimators,
observation-centric momentum (OCM), observation-centric re-update (ORU),
wall-clock timing. The shipped `sushitrack.json` enables the tentative
lifecycle, OCM, and ORU; a caller that constructs `sushitrack_params_t`
programmatically gets the compiled defaults instead.

---

## Per-frame pipeline

`Tracker::update(objects, dt)` (`source/tracking/tracker.cpp`) runs one
frame. The steps are:

1. **Timestep resolution.** In frame mode (`time_aware=0`) the internal step is
   `1 / frame_rate` regardless of the passed `dt`. In wall-clock mode
   (`time_aware=1`) the passed `dt` is used as elapsed seconds.
2. **Detection split.** Each detection becomes a `Tracklet`. Detections with
   `score >= track_thresh` go to the high pool; the rest go to the low pool.
3. **Prediction.** Every confirmed track and every lost track is propagated to
   the current timestamp via its predictor. Tentative tracks are predicted
   separately.
4. **Lost/active split.** When a lost-recovery metric is configured, `Tracked`
   tracks and `Lost` tracks are separated so recovery runs on the lost set only.
5. **High-confidence association.** Confirmed/active tracks match against the
   high pool ([primary stage](#two-stage-byte-association)).
6. **Low-confidence association.** Tracks unmatched in step 5 match against the
   low pool. Tracks that remain unmatched are marked `Lost`.
7. **Lost recovery** (when enabled). Lost tracks match against the
   high-confidence detections still unclaimed, using the recovery metric.
8. **Tentative stage** (when `enable_tentative=1`). Tentative tracks match the
   remaining detections; matched tentatives accumulate hits and confirm when
   the [dynamic hit threshold](#tentative-confirmation) is met; unmatched
   high-confidence detections seed new tentative tracks after
   [initialization suppression](#initialization-suppression). When
   `enable_tentative=0`, an unconfirmed-track path runs instead.
9. **Lost cleanup.** Lost tracks past their [scaled buffer](#lost-buffer-and-expiry)
   are removed.
10. **List finalization.** The tracked, lost, and removed pools are rebuilt and
    cross-pool [duplicates](#duplicate-suppression) are removed.

`update` returns all tracklets across states (Tracked, Lost, Tentative, Removed
for the current frame). The C API layer (`sushitrack_update`) sorts them by
lifecycle priority (Tracked < Tentative < Lost < Removed), copies as many as
the caller's buffer holds into the output struct, and returns
`SUSHITRACK_BUFFER_OVERFLOW` when some did not fit.

---

## Interfaces and wiring

The tracker depends on abstract interfaces; concrete implementations are
constructed by the C-API entry point (`sushitrack_create_ex` in
`source/c_api/sushitrack_c.cpp`) from the resolved `sushitrack_params_t` and injected into
`Tracker`. The orchestrator references only the interfaces.

| Interface | Responsibility | Implementations |
|---|---|---|
| `StatePredictor` | Propagate/update motion state | `KalmanFilter`, `NSAKalmanFilter`, `IMMFilter` |
| `GateValidator` | Mahalanobis gating | `KalmanFilter`, `NSAKalmanFilter`, `IMMFilter`, `NullGateValidator` |
| `GatedStatePredictor` | Predictor + validator union | `KalmanFilter`, `NSAKalmanFilter`, `IMMFilter` |
| `KalmanFactory` | Per-tracklet predictor factory | `GenericKalmanFactory` (registry-backed) |
| `CostCalculator` | Pairwise association cost | IoU/DIoU/GIoU/SIoU, Cosine, ReIDFirst, PureReID, Fused, GatedIoU, and the FuseScore and ObservationMomentum decorators |
| `AssignmentSolver` | Bipartite assignment | `LAPJVSolver` |
| `TimingPolicy` | `dt` normalization and expiry | `FrameTimingPolicy`, `WallClockTimingPolicy` |

`KalmanFilterRegistry` is a singleton registry keyed by the integer
`kalman_type`. Each predictor registers itself with a `KalmanFilterRegistrar<T>`
static: `KalmanFilter` at 0, `NSAKalmanFilter` at 1, `IMMFilter` at 2. The
factory and the gating path both resolve predictors through the registry, so a
new estimator becomes selectable by registering a new type id without editing
the factory.

---

## Data model

**`Object`** (`include/SushiTrack/detection/object.hpp`) is one input detection: a `Rect`
box, a class `label`, a confidence `score`, and an optional appearance feature
vector.

**`Rect<T>`** (`include/SushiTrack/detection/rect.hpp`) is a bounding box templated on the
scalar type. It stores top-left `x, y` plus `width, height` and converts among
TLWH, TLBR, and XYWH (centroid) forms.

**`Tracklet`** (`include/SushiTrack/tracking/tracklet.hpp`) holds one track: the estimator
state mean/covariance, the current box, lifecycle flags, score (raw and
EMA-smoothed), identifiers, feature history, the IMM-derived regime, the
observation-centric direction estimate, and the ORU checkpoint. Detections are
also wrapped in `Tracklet` instances so the association stages operate on a
single type. State accessors (`get_mean`/`get_covariance`/`get_measurement`) are
mutex-protected.

**`sushitrack_track_t`** (`include/SushiTrack/c_api/sushitrack_c.h`) is the C output record:
identifier, box, score, label, lifecycle `state`, `kinematic_state`, `age`,
`hits`, `lost_time`, `frames_since_last_update`, `maneuver_probability`, the
feature pointer, and activation/confirmation flags. `maneuver_probability` is
`-1` when the active predictor is not an IMM, distinguishing "no estimate" from
"confidently not manoeuvring". The feature pointer aliases tracker-owned memory
and is valid only until the next `update`/`destroy`.

---

## Coordinate systems

Two state parameterizations are supported, selected by `enable_xyah`:

- `enable_xyah=0` (default): TLWH state `[x, y, w, h, vx, vy, vw, vh]`.
- `enable_xyah=1`: centroid/aspect/height state `[cx, cy, a, h, ...]`.

The measurement handed to the estimator is 4-D in both cases. `iou_plus_one=1`
(default) uses inclusive box dimensions (`+1` on width/height and on
intersection extents), consistent with the ByteTrack reference; `iou_plus_one=0`
uses exclusive dimensions.

---

## Motion estimators

The linear Kalman filter, its noise-scale-adaptive variant, the IMM estimator and the kinematic
regime they report are in [Motion estimators](MOTION_ESTIMATORS.md).

## Association

### Two-stage BYTE association

The primary stage matches confirmed tracks against high-confidence detections
under the `match_thresh` cost limit. The secondary stage matches tracks left
unmatched by the primary stage against the low-confidence detection pool under
`iou_match_thresh`. A track unmatched after both stages is marked `Lost`. This
is the ByteTrack two-stage design: low-confidence detections can sustain an
existing track but do not seed new ones.

### Primary association mode

`primary_assoc_mode` selects the primary-stage solve:

- `1` (default): a single global LAPJV over the union of candidate tracks and
  high-confidence detections, matching the reference BYTETracker assignment.
- `0`: a legacy age-cascade: tracks are bucketed by frames-since-last-seen and
  each bucket is solved independently, favouring the freshest tracks in
  contested matches.

Lost recovery uses an age-cascade over the lost set regardless of this flag.

### Cost calculators

Cost calculators implement `CostCalculator` and populate a `CostMatrix`
(contiguous 2-D storage, workspace reused across frames). Cells that violate a
constraint are set to a sentinel (`>= 1e3`) so the solver rejects them; the
`max_area_ratio` constraint sets `1e4` for pairs with extreme area disparity.

Geometry metrics, selected by `iou_type` for the primary and secondary stages:

- **IoU** (`0`): cost `1 - IoU`.
- **DIoU** (`1`, default): cost `1 - IoU + d^2 / c^2`, where `d` is the
  centroid distance and `c` the diagonal of the smallest enclosing box.
- **GIoU** (`2`): cost `1 - GIoU`, `GIoU = IoU - (area_enclosing - area_union) /
  area_enclosing`.
- **SIoU** (`3`): cost `1 - SIoU`, combining the IoU with an angle-aware
  distance cost and a shape cost.

The tentative and duplicate stages are pinned to plain IoU regardless of
`iou_type`, because their thresholds are calibrated against the plain
`1 - IoU` scale and the DIoU centroid penalty would skew them.

Appearance and fused metrics (see [Appearance / ReID](#appearance--reid)):
`CosineCostCalculator`, `ReIDFirstCostCalculator`, `PureReIDCostCalculator`,
`FusedCostCalculator`, `GatedIoUCostCalculator`. Two decorators wrap the primary
metric: `FuseScoreDecorator` and `ObservationMomentumDecorator`.

### Score fusion

`FuseScoreDecorator` (`fuse_score=1`, default) rescales each valid primary-cost
cell by the detection score: `cost' = 1 - (1 - cost) * score`. Sentinel cells
are left untouched. It is applied to the primary stage only, matching the
ByteTrack reference; wrapping the low-confidence, tentative, duplicate, or
recovery stages would double-penalize pairs that are low-score by construction
or operate on a different cost scale.

### Mahalanobis gating

With `enable_mahalanobis=1`, association candidates are filtered by a chi-squared
test on the predicted box distribution. `gating_thresh` defaults to `9.4877`
(95% for 4 degrees of freedom). The gate is measured in 4-D box space: the 8-D
state is projected to the box it would produce and the distance is taken there,
because a detection reports a box but never a velocity. This holds for the IMM
path too: its bank collapses to the same 4-D box estimate before gating. This
is the 4-D-measurement convention shared by SORT, DeepSORT, ByteTrack, and
OC-SORT.

`GatedIoUCostCalculator` combines the gate with IoU; `motion_cost_weight`
(default 0.15) blends the normalized squared distance into the cost. Regime-aware
row policies (`state_assoc_mods`) tighten the gate for Stationary tracks and widen
it for Maneuvering/Coasting.

### Appearance / ReID

`enable_reid=1` activates appearance matching. Detections carry an external
appearance embedding as a 7th tuple element (the library does not compute
embeddings). Features are L2-normalized and exponentially smoothed
(`feature_momentum`, default 0.9); a per-tracklet gallery holds up to
`max_feature_history` (default 100) vectors, and `get_max_feature_similarity`
returns the maximum cosine similarity against the gallery.

The active path depends on `enable_mahalanobis`:

- **ReID-first** (default): `ReIDFirstCostCalculator` for the primary/secondary
  stages: a direct appearance match below `reid_high_thresh`/`reid_low_thresh`
  wins immediately (scaled by `reid_direct_assignment_weight`), otherwise the
  cost blends appearance distance with an IoU fallback weighted by
  `reid_high_iou_weight`/`reid_low_iou_weight`.
- **Fused** (with `enable_mahalanobis=1`): `FusedCostCalculator` combines gated
  motion distance and cosine appearance distance under regime-aware weights.
- **Lost recovery** (both paths): `PureReIDCostCalculator` recovers lost tracks
  from appearance alone under `reid_recovery_thresh`, gated when a validator is
  present.

### Observation-centric momentum

`ObservationMomentumDecorator` (`enable_ocm=1`) is the outermost primary-stage
decorator (applied after score fusion). It re-ranks the primary cost with a
bounded directional term inspired by OC-SORT's velocity-direction consistency.

For a track with a fresh, fast-enough, eligible-regime observed direction, and
each candidate detection, it subtracts `clamp(weight * cos(theta), -weight,
+weight)` from the cost cell, where `theta` is the angle between the track's
observed direction of travel (a unit vector computed from raw observations over
`ocm_delta_t` frames, not from Kalman velocity) and the direction from the
track's last observation to the candidate. The term is bounded by
`ocm_vdc_weight` (default 0.1), so it can only re-rank candidates already inside
the gate; it cannot rescue a gated-out pair or overpower geometry. Sentinel
cells are skipped.

Eligibility (per `effective_weight`): the observed speed must exceed
`ocm_min_velocity`; the track must have been observed within `ocm_max_age`
frames; the regime must be Cruising (full weight) or Maneuvering (weight scaled
by `ocm_maneuver_weight_scale`, default 0.15). Stationary and Coasting tracks
are skipped because their stored direction is meaningless.

---

## Assignment solver

`LAPJVSolver` (`source/association/lapjv_solver.cpp`, `source/association/lapjv.cpp`) solves the linear
assignment with the Jonker–Volgenant algorithm (column reduction,
reduction-transfer, augmenting-path search with dual updates). Pairs at or above
the stage cost limit are treated as non-edges, so the solver returns matches
plus the unmatched rows and columns. Workspace buffers are retained across
frames to avoid per-frame heap allocation.

---

## Track lifecycle

### States

`TrackState` (`include/SushiTrack/tracking/tracklet.hpp`): `New (0)`, `Tracked (1)`,
`Lost (2)`, `Removed (3)`, `Tentative (4)`. A detection begins as `New`; it
becomes `Tentative` (when the tentative lifecycle is on) or directly `Tracked`.
A confirmed track that misses observations becomes `Lost`, is recoverable within
the buffer, and is `Removed` on expiry.

### Tentative confirmation

With `enable_tentative=1`, a new high-confidence detection starts a tentative
track that must accumulate a required number of consecutive matches before
promotion. The required count is adaptive to the smoothed score of the matched
detection (`get_dynamic_hits` in `match_tentative`):

- `score >= tentative_fast_thresh` → `tentative_fast_hits` (single-hit
  confirmation),
- `score >= high_thresh` → `tentative_confirm_hits`,
- `score >= tentative_slow_thresh` → `tentative_confirm_hits +
  tentative_slow_hits_offset`,
- below `tentative_slow_thresh` → `tentative_confirm_hits +
  tentative_vslow_hits_offset`.

Two counters guard promotion: a monotonic lifetime `hits_` and a
`consecutive_hits_` streak that resets whenever the track survives a frame with
no observation. Confirmation is gated on the streak, so a strobing false
positive that matches on alternate frames never accumulates enough streak. The
confirmation score is EMA-smoothed (momentum 0.7) so a one-frame spike cannot
collapse the required-hit count. The required count is evaluated against the
last matched detection's score, so a late high-confidence observation confirms a
track that had accumulated low-confidence hits.

### Lost buffer and expiry

A lost track is retained for `track_buffer` (default 30) before removal. The
buffer is scaled by the pre-Coasting regime under an IMM: Stationary 1.5×,
Maneuvering 1.2×, others 1.0× (1.0× with no IMM regime). Expiry is evaluated by
the timing policy: frame mode compares frames-since-last-seen against the scaled
buffer; wall-clock mode compares accumulated lost seconds against
`scaled_buffer / frame_rate`.

Tentative tracks age out on a separate, tighter budget:
`tentative_max_miss_frames` (default 2) in frame mode, or
`tentative_max_age_seconds` (default 0.08 s ≈ 2 frames at 25 fps) in wall-clock
mode.

### Duplicate suppression

After each frame, cross-pool tracklet pairs with `1 - IoU < duplicate_iou_thresh`
(default 0.15, i.e. IoU > 0.85) are treated as duplicates and the shorter-lived
track is dropped. Track lifetime is the span from the start frame to the current
observation frame (matching the ByteTrack policy), not a freshness measure, so a
just-recovered long-lived track is not displaced by a young overlapping one.

### Initialization suppression

Before seeding new tentative tracks, `suppress_overlapping_inits` drops any
init-candidate detection whose IoU with a track confirmed this frame exceeds
`tentative_init_suppress_iou` (default 0.7). This prevents a freshly initialized
tentative from duplicating an already-tracked target and splitting its identity
when it later confirms.

### Observation-centric re-update

`enable_oru=1` repairs the velocity that drifts while a track coasts through an
unobserved gap. At each real observation the estimator state is checkpointed. On
recovery after a gap of at least two frames, the estimator is rolled back to the
checkpoint and the gap is replayed as predict/update steps along a straight-line
virtual trajectory interpolated from the pre-loss observation to the recovery
detection. Routing the replay through the predictor's standard predict/update
applies the repair to every IMM mode without mode-specific code. When no
checkpoint exists or the gap is too small, recovery falls back to a single
update.

---

## Timing policies

`TimingPolicy` normalizes the timestep and defines expiry:

- **`FrameTimingPolicy`** (`time_aware=0`): each `update` is one step; expiry is
  by frame count.
- **`WallClockTimingPolicy`** (`time_aware=1`): `dt` is real seconds, multiplied
  by `frame_rate` to obtain a Kalman step (capped at 5.0). Expiry compares
  accumulated wall time against `track_buffer / frame_rate` seconds (strict `>`
  with a `1e-4` epsilon for symmetry with the frame-counted path).

The marking frame is counted in the lost-time budget (`mark_as_lost` seeds the
accumulator with that frame's `dt`) so the two modes expire a track on the same
frame.

---

## Configuration system

Every tunable parameter has three synchronized representations
(`docs/CONTRIBUTING.md`): the C++ struct `TrackerConfig`
(`include/SushiTrack/tracking/tracker_config.hpp`), the C ABI struct `sushitrack_params_t`
(`include/SushiTrack/c_api/sushitrack_c.h`), and the JSON schema (`sushitrack.json`). The Python
binding's `SushiTrackParams` mirrors the C struct field-for-field in declaration
order; a missing or reordered field silently corrupts every field after it.

`source/configuration/config_loader.cpp` overlays a JSON file onto a seeded `sushitrack_params_t`. The
schema is layered `basic` / `advanced.<section>`; the outer `"sushitrack"`
wrapper is optional. Any key absent from the file keeps the compiled default. The
reader refuses what the format does not hold: a key no section reads, a value of the
wrong kind, a number with a fraction in an integer field, and a named file that does
not exist, whether the path argument or `SUSHITRACK_CONFIG` names it. A refusal
returns `SUSHITRACK_INVALID_ARGUMENT`, logs one ERROR with the dotted path of the key,
and leaves the parameters untouched; `sushitrack_create_from_config` then returns no
tracker. A file that is not valid JSON returns `SUSHITRACK_RUNTIME_ERROR`. With nothing
named and nothing found, the call succeeds and logs INFO. Resolution order: the
`SUSHITRACK_CONFIG` environment variable, then an upward search for `sushitrack.json`
from the working directory.
Because the library parses the file itself, a compiled build is retuned by
editing `sushitrack.json` with no recompilation.

`sushitrack_create(NULL)` builds from the resolved config file; passing an
explicit `sushitrack_params_t*` bypasses file loading entirely for programmatic
control.

---

## C API and bindings

`include/SushiTrack/c_api/sushitrack_c.h` exposes an opaque `sushitrack_tracker_t` and the entry points
`sushitrack_get_default_params`, `sushitrack_create`, `sushitrack_create_ex`
(status-returning), `sushitrack_create_from_config`, `sushitrack_load_params_from_json`,
`sushitrack_update` and `sushitrack_destroy_tracker`. The ABI
is dependency-free (no Eigen or C++ headers leak through), so a C consumer needs
only `sushitrack_c.h`. Status codes are `sushitrack_status_t`.

`bindings/python/sushitrack.py` is a dependency-free `ctypes` binding that mirrors
the structures and entry points and wraps them in a `Tracker` class.
The shared library is located via an explicit path, the `SUSHITRACK_LIB`
environment variable, the directory next to the binding module (for deploy
packages), or the `build/` output tree.

`st build --deploy {cpp,python}` assembles a self-contained
`package/` with the compiled library, the headers or binding, a default config,
and a runnable example. The deploy is platform-specific (it copies the artefacts
for the host OS).

The library also integrates as a drop-in tracker behind an Ultralytics YOLO
model through a `ctypes` adapter over the C API; the recipe is documented in the
README.

---

## Logging

SushiTrack logs through SushiLog, the one logger of every Sushi library, with the macros of
`include/SushiTrack/logging/logger.hpp`. Every call names the category of its area,
`K_TRACKING_LOG_CATEGORY`, `K_ASSOCIATION_LOG_CATEGORY`, `K_FILTERING_LOG_CATEGORY`,
`K_CONFIGURATION_LOG_CATEGORY` or `K_C_API_LOG_CATEGORY`, each declared in its area's folder under
the project `sushitrack`. `SUSHITRACK_LOG_LEVEL` sets their levels, `warn` when unset, for all
of them or by area: `warn,association=trace`. `debug` adds the track lifecycle events, each
starting with its frame number, and `trace` the per-frame association summaries, which only a
debug build compiles. The applications log under `tracker_bridge` and `mot_engine`.

An event that can recur every frame, such as a covariance that fails to factor, a feature of the
wrong length or an overflowing output buffer, writes its first record and then one in a thousand,
each ending with the number it left out (`SUSHITRACK_LOG_WARN_LIMITED`,
`include/SushiTrack/logging/log_throttle.hpp`). Records go to stderr. A C or Python host takes them with
SushiLog's `sl_set_log_sink`; the Python binding's `Tracker.set_log_callback` does that for a
callable. `SUSHITRACK_THROW` throws without logging, and the C entry point that catches the
exception logs it once.

---

## Profiling

SushiTrack records its frame through SushiProfiler, linked as the shared library so a process
holds one collector. The host marks the period: `mot_engine` marks the timeline `sushitrack.frame`
once per frame, before the tracker's update, and the library records zones, counters and instants
inside it (`include/SushiTrack/profiling/profiling.hpp`). A host that starts no session pays
one relaxed load per site.

| Level | Records |
| --- | --- |
| 1 | The stage zones `update`, `split`, `predict`, `match_high`, `match_low`, `lost_recovery`, `tentative` or `unconfirmed`, `cleanup_lost`, `finalize` and `output`, and nine counters |
| 2 | Adds the `solve` and `cost_matrix` zones, the other counters and the track instants |

`SUSHITRACK_PROFILE_LEVEL` is the level compiled in, 1 in a release build and 2 in a debug
build; the session's `level` filters at run time. With `SUSHITRACK_ENABLE_PROFILER` off the macros
expand to nothing and the library builds without SushiProfiler. The names, groups and units are in
[Profiler keys](../reference/PROFILER_KEYS.md). `register_profiler_parts`
(`include/SushiTrack/profiling/sushitrack_profiler_parts.hpp`) adds the library's name, version,
commit, profile level and run date to a record's environment. The design and its phases are in
`docs/design/SUSHITRACK_PROFILING/README.md`.

---

## Thread safety

`Tracklet` state accessors (`get_mean`, `get_covariance`, `get_measurement`) are
mutex-protected. The `KalmanFilter` projected-LLT cache is mutex-protected. A single tracker instance is not designed for concurrent
`update` calls; construct one instance per stream.

---

## Testing and evaluation

The tests mirror the public headers under `tests/` and build into one program, each suite labelled
by its size; [Test suites](../guides/TESTING.md) describes them. The MOT benchmark and its
TrackEval metrics are in [Evaluation pipeline](../guides/EVALUATION.md), and the YOLOX video
pipeline in [Inference pipeline](../guides/INFERENCE.md).

## Future work

Deferred changes to the tracker are listed in [Remaining work](../design/REMAINING_WORK.md).
