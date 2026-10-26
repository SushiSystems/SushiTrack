# Configuration

## Tracker config files

Each tracker owns a dedicated JSON file:

| File | Tracker | Loaded by |
|---|---|---|
| `assets/sushitrack.json` | SushiTrack | the **library itself** |
| `applications/tracker_bridge/bytetrack.json` | ByteTrack | MOT benchmark |
| `applications/tracker_bridge/ocsort.json` | OCSORT | MOT benchmark |

`sushitrack.json` is bound to the library, not the test harness: the C API parses it directly (`source/configuration/config_loader.cpp`), so a compiled `sushitrack.dll`/`.so` is retuned by editing the file with **no recompilation**. Resolution order: the `$SUSHITRACK_CONFIG` environment variable, then an upward search for `sushitrack.json` from the working directory. Inside the repository the file is `assets/sushitrack.json`, which no upward search reaches, so `st run`, `st test`, `st eval` and `st infer` set `SUSHITRACK_CONFIG` to it when the variable is unset. Any missing key falls back to the compiled default from `sushitrack_get_default_params`.

The schema is layered `basic` / `advanced`. The outer `"sushitrack"` wrapper is optional: a file whose root is already the `basic`/`advanced` object is also accepted:

```json
{
  "sushitrack": {
    "basic": {
      "track_thresh": 0.2, "high_thresh": 0.5, "match_thresh": 0.7,
      "frame_rate": 25, "track_buffer": 30,
      "enable_tentative": 1, "enable_reid": 0,
      "enable_mahalanobis": 0
    },
    "advanced": {
      "association":  { "primary_assoc_mode": 1, "fuse_score": 1, "iou_match_thresh": 0.5 },
      "coordinate_system": { "iou_type": 1, "enable_xyah": 0, "iou_plus_one": 1, "kalman_type": 0 }
    }
  }
}
```

C API entry points (`include/SushiTrack/c_api/sushitrack_c.h`):

- `sushitrack_load_params_from_json(path, &params)`: overlay a file onto a seeded `sushitrack_params_t` (`path = NULL` uses the resolution order above). It returns `SUSHITRACK_INVALID_ARGUMENT`, with `params` untouched and one ERROR in the log, for a null `params`, a named file that does not exist (the path argument or `$SUSHITRACK_CONFIG`), a key no section reads, a value of the wrong kind, and a number with a fraction in an integer field. A file that is not valid JSON gives `SUSHITRACK_RUNTIME_ERROR`. With nothing named and nothing found it succeeds.
- `sushitrack_create_from_config(path)`: seed defaults, overlay the file, and create in one call. It returns no tracker when the file is refused.
- `sushitrack_create(NULL)`: shorthand for `sushitrack_create_from_config(NULL)`. Passing an explicit `sushitrack_params_t*` bypasses file loading entirely (programmatic control).

The Python binding mirrors this: `Tracker(config="auto")` resolves `sushitrack.json` through the library, `config="<path>"` loads an explicit file, and keyword overrides still win over the file.

## Parameters

All knobs are exposed both on `sushitrack_params_t` (C API, `include/SushiTrack/c_api/sushitrack_c.h`) and `SushiTrack::TrackerConfig` (C++, `include/SushiTrack/tracking/tracker_config.hpp`). Defaults below are from `sushitrack_get_default_params`.

### Detection thresholds

| Key | Default | Description |
|---|---|---|
| `track_thresh` | 0.2 | Splits detections into high (`score ≥ track_thresh`) and low pools |
| `high_thresh` | 0.5 | Minimum score to initialise a new track |

### Association cost limits

| Key | Default | Description |
|---|---|---|
| `match_thresh` | 0.7 | Maximum cost for the primary stage |
| `iou_match_thresh` | 0.5 | Maximum cost for the secondary stage |
| `unconfirmed_iou_thresh` | 0.7 | Maximum cost for unconfirmed tracks (`enable_tentative=0`) |
| `duplicate_iou_thresh` | 0.15 | Cost cutoff for duplicate suppression (IoU > 0.85) |

### Track lifecycle

| Key | Default | Description |
|---|---|---|
| `frame_rate` | 25 | Target frequency; drives noise scaling and wall-clock conversion |
| `track_buffer` | 30 | Frames a lost track is kept before deletion |
| `enable_tentative` | 0 | Enables tentative state |
| `tentative_match_thresh` | 0.3 | Cost cutoff for tentative association |
| `tentative_confirm_hits` | 3 | Consecutive matches required to confirm |
| `tentative_max_age_seconds` | 0.08 | Wall-clock tentative expiry budget, used only when `time_aware=1` (≈ 2 frames at 25 fps) |
| `tentative_max_miss_frames` | 2 | Frame-counted tentative expiry budget, used when `time_aware=0` |
| `tentative_init_suppress_iou` | 0.7 | IoU above which a new-track detection overlapping a confirmed track is suppressed |
| `tentative_fast_thresh` | 0.85 | Above this score, `tentative_fast_hits` hits are enough |
| `tentative_fast_hits` | 1 | Hit count for fast-confirmation regime |
| `tentative_slow_thresh` | 0.40 | Below this score, extra hits are required |
| `tentative_slow_hits_offset` | 1 | Extra hits added between `tentative_slow_thresh` and `high_thresh` |
| `tentative_vslow_hits_offset` | 2 | Extra hits added below `tentative_slow_thresh` |

### Motion model

| Key | Default | Description |
|---|---|---|
| `enable_xyah` | 0 | 0 = TLWH state, 1 = centroid+aspect+height state |
| `iou_plus_one` | 1 | Inclusive bounding-box dimensions |

`kalman_type` is documented under [Motion estimator](#motion-estimator) below.

`KalmanFilter` baseline constants (compile-time, see `include/SushiTrack/filtering/kalman_filter.hpp`): `K_STD_WEIGHT_POSITION=1/20`, `K_STD_WEIGHT_VELOCITY=1/160`, `K_INIT_POSITION_MULT=2.0`, `K_INIT_VELOCITY_MULT=10.0`. These replace earlier configurable hyper-parameters and match the ByteTrack reference tuning.

### Association strategy

| Key | Default | Description |
|---|---|---|
| `primary_assoc_mode` | 1 | 0 = age-cascade legacy, 1 = global LAPJV |
| `fuse_score` | 1 | Score-fusion decorator on the primary cost matrix |
| `iou_type` | 1 | 0=IoU, 1=DIoU, 2=GIoU, 3=SIoU |
| `enable_mahalanobis` | 0 | Apply Mahalanobis gating |
| `gating_thresh` | 9.4877 | Chi-squared threshold (95 % CI, 4 DoF) |
| `motion_cost_weight` | 0.15 | Weight of squared Mahalanobis distance in gated IoU |
| `max_area_ratio` | 50.0 | Suppress pairs with extreme area disparity |

### Observation-centric momentum (OCM / ORU)

| Key | Default | Description |
|---|---|---|
| `enable_ocm` | 0 | Enable the observation-centric momentum re-ranking term on the primary stage |
| `enable_oru` | 0 | Enable observation-centric re-update (rollback and replay) on lost-track recovery |
| `ocm_vdc_weight` | 0.1 | Bound on the momentum term subtracted from each in-gate primary cost cell |
| `ocm_delta_t` | 3 | Observed-frame window over which the direction of travel is measured |
| `ocm_min_velocity` | 1.0 | Minimum observed speed (px per observed frame) below which momentum is suppressed |
| `ocm_max_age` | 1 | Maximum frames since the last observation for which momentum is applied |
| `ocm_maneuver_weight_scale` | 0.15 | Fraction of `ocm_vdc_weight` applied to Maneuvering tracks (0 restricts momentum to Cruising) |

### ReID

| Key | Default | Description |
|---|---|---|
| `enable_reid` | 0 | Enable appearance matching |
| `feature_momentum` | 0.9 | EMA weight for the smoothed feature |
| `max_feature_history` | 100 | Per-tracklet feature-vector gallery size |
| `reid_high_thresh` | 0.3 | Direct-association similarity threshold (high stage) |
| `reid_high_iou_weight` | 0.3 | IoU fallback weight (high stage) |
| `reid_low_thresh` | 0.5 | Direct-association similarity threshold (low stage) |
| `reid_low_iou_weight` | 0.2 | IoU fallback weight (low stage) |
| `reid_recovery_thresh` | 0.4 | Similarity threshold for PureReID lost recovery |
| `reid_direct_assignment_weight` | 0.2 | Scaling for high-confidence direct visual matches |

### Motion estimator

| Key | Default | Description |
|---|---|---|
| `kalman_type` | 0 | Motion estimator: `0` linear Kalman, `1` NSA, `2` IMM multi-model. With `2` each track reports a `kinematic_state` and continuous `maneuver_probability`; the IMM noise/transition parameters are built-in (see [the architecture overview](../architecture/OVERVIEW.md)). |

### Timing

| Key | Default | Description |
|---|---|---|
| `time_aware` | 0 | 0 = treat dt as 1 step; 1 = treat dt as real seconds (capped at 5 KF-steps) |
