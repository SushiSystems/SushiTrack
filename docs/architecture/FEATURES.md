# Features

## Two-stage ByteTrack association

High-confidence detections (`score >= track_thresh`) match first; remaining tracks then attempt to absorb low-confidence detections.

## Tentative validation

When `enable_tentative=1`, new tracks must accumulate `tentative_confirm_hits` consecutive matches before promotion. Confirmation hits are adaptive: `tentative_fast_thresh` (default 0.85) allows single-hit confirmation; `tentative_slow_thresh` (default 0.40) adds `tentative_slow_hits_offset` extra hits; below the slow threshold adds `tentative_vslow_hits_offset` hits. `tentative_match_thresh` governs tentative-pass cost. Expiry uses `tentative_max_miss_frames` in frame mode (`time_aware=0`) or `tentative_max_age_seconds` in wall-clock mode (`time_aware=1`), and `tentative_init_suppress_iou` drops a new-track detection overlapping a confirmed track.

## IoU variants

`iou_type` selects `IoUCostCalculator` (0), `DIoUCostCalculator` (1, default), `GIoUCostCalculator` (2), or `SIoUCostCalculator` (3) for the primary/secondary stages. Tentative and duplicate stages are pinned to plain IoU so their thresholds keep their calibration.

## Score fusion

`FuseScoreDecorator` rescales the primary cost matrix by detection score: `fused = 1 − (1 − inner) * score`. Sentinel cells are preserved. Applied only to the primary stage. Toggled with `fuse_score`.

## Visual ReID

`enable_reid=1` activates one of three paths depending on `enable_mahalanobis`:
- ReID-first (default): `ReIDFirstCostCalculator` for the primary/secondary passes with an IoU fallback, plus `PureReIDCostCalculator` for lost recovery.
- Fused: `FusedCostCalculator` combines IoU, cosine appearance and Mahalanobis gating (when `enable_mahalanobis=1`).
- Lost recovery: `PureReIDCostCalculator` (`reid_recovery_thresh`), active in all ReID paths.

Visual features are L2-normalised and exponentially smoothed (`feature_momentum`, default 0.9). A per-tracklet feature history holds up to `max_feature_history` (default 100) vectors and `getMaxFeatureSimilarity` returns the max cosine similarity against the history.

## Mahalanobis gating

`enable_mahalanobis=1` filters association candidates via a chi-squared test on the projected state distribution. `gating_thresh` defaults to 9.4877 (95 % CI for 4 DoF). `GatedIoUCostCalculator` combines IoU with the gate; `motion_cost_weight` blends the squared distance into the cost.

> **Why the gate uses only the box (`xyah`), not velocity.**
> The Kalman state carries eight numbers: the four box numbers (`[cx, cy, a, h]`, or `[x, y, w, h]`) plus their four velocities. A detector only ever reports a box, never a velocity, so the only quantity a track can be compared against is its predicted box. The Mahalanobis gate therefore projects the 8-D state down to the 4-D box it would produce and measures the distance in that 4-D box space, scaling the box residual by the predicted box uncertainty. The velocities are used to *predict* where the box goes, but are never part of the distance, because there is no measured velocity to compare them to. This is why the chi-squared test has **4** degrees of freedom (hence the 9.4877 default = 95 % for 4 DoF) and not 8: feeding in the velocity dimensions would be comparing against numbers no detection contains, which would distort the gate. This holds identically for the IMM estimator (`kalman_type=2`): its internal model bank is collapsed to the same 4-D box estimate before gating, so the gate sees `xyah` only, exactly as the plain Kalman path does. This 4-D-measurement convention is the standard one shared by SORT, DeepSORT, ByteTrack and OC-SORT.

## NSA Kalman filter

`kalman_type=1` selects `NSAKalmanFilter`, a variant that scales measurement noise by detection confidence in both `initiate` and `project`.

## IMM motion estimator

`kalman_type=2`. A four-model Interacting Multiple Model filter (constant velocity, constant acceleration, near-constant-position, and a nonlinear coordinated-turn mode handled with an unscented update) replaces the single Kalman model. Its mode probabilities yield a continuous `maneuver_probability` (mass on the manoeuvring models) and a discrete `kinematic_state` (`Stationary`/`Cruising`/`Maneuvering`, or `Coasting` while Lost), consumed by association cost shaping, lost-buffer scaling and OCM weighting. See the IMM section of [the architecture overview](OVERVIEW.md).

## Coordinate representations

`enable_xyah=0` (default) uses TLWH state (`[x, y, w, h, vx, vy, vw, vh]`); `enable_xyah=1` uses centroid + aspect ratio + height (`[cx, cy, a, h, …]`). `iou_plus_one=1` (default) uses inclusive box dimensions consistent with the ByteTrack reference.

## Primary association mode

`primary_assoc_mode=1` (default) runs a single global LAPJV solve over the high-confidence pool. `primary_assoc_mode=0` retains a legacy age-cascade path.

## Duplicate suppression

`duplicate_iou_thresh` (default 0.15, i.e. IoU > 0.85) removes redundant tracklets after each frame using a plain-IoU calculator.

## Observation-centric momentum (OCM)

`enable_ocm=1` re-ranks the primary cost with a bounded velocity-direction term (OC-SORT VDC). For a track with a fresh, fast-enough, eligible-regime observed direction, it subtracts `clamp(ocm_vdc_weight * cos(theta), -ocm_vdc_weight, +ocm_vdc_weight)` from each in-gate primary cost cell, where `theta` is the angle between the track's observed direction of travel (measured over `ocm_delta_t` observed frames, not from Kalman velocity) and the direction from the last observation to the candidate. The term is bounded, so it only re-ranks candidates already inside the gate. `ocm_min_velocity`, `ocm_max_age`, and `ocm_maneuver_weight_scale` gate and scale the term by observed speed, staleness, and motion regime (Cruising full weight, Maneuvering scaled, Stationary/Coasting skipped).

## Observation-centric re-update (ORU)

`enable_oru=1` repairs the velocity that drifts while a track coasts through an unobserved gap. At each observation the estimator state is checkpointed; on recovery after a gap of at least two frames the estimator is rolled back to the checkpoint and the gap is replayed as predict/update steps along a straight-line virtual trajectory from the pre-loss observation to the recovery detection.

## LAPJV solver

`LAPJVSolver` runs the Jonker–Volgenant algorithm with active-row/column reduction. Workspace buffers are reused across frames to remove per-frame heap pressure.

## Timing policies

`time_aware=0` uses `FrameTimingPolicy` (dt=1.0 each call, expiry by frame count). `time_aware=1` uses `WallClockTimingPolicy`: dt is treated as real seconds and multiplied by `frame_rate` to obtain a KF step, capped at 5.0. Track expiry compares accumulated wall time against `track_buffer / frame_rate` seconds (strict >, with a 1e-4 epsilon for symmetry with the frame-counted path).

## C API

`sushitrack_c.h` exposes an opaque `sushitrack_tracker_t` plus `sushitrack_get_default_params`, `sushitrack_create`, `sushitrack_create_ex`, `sushitrack_update`, `sushitrack_destroy`, `sushitrack_set_log_callback`. Suitable for FFI integration from Python, Rust, or any C-FFI language.

## Logging

`SUSHITRACK_LOG_DEBUG/INFO/WARN/ERROR` macros route to a single global callback registered via `sushitrack_set_log_callback`. The callback is stored atomically; setting/clearing it is thread-safe.

## Thread safety

`Tracklet` state access is mutex-protected for `getMean`/`getCovariance`/`getMeasurement`. The `KalmanFilter` LLT projection cache is mutex-protected.
