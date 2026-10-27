# C API

Header: `include/SushiTrack/c_api/sushitrack_c.h`. Link against the compiled `sushitrack` shared library.

Minimal usage:

```c
#include <SushiTrack/c_api/sushitrack_c.h>

sushitrack_params_t params;
sushitrack_get_default_params(&params);
params.kalman_type      = 2;
params.enable_tentative  = 1;
params.frame_rate        = 25;

sushitrack_tracker_t hdl;
sushitrack_status_t s = sushitrack_create_ex(&params, &hdl);
if (s != SUSHITRACK_SUCCESS) { /* handle */ }

sushitrack_object_t  dets[MAX_DETS];   /* fill in per frame */
sushitrack_track_t   tracks[MAX_TRACKS];
int n = 0;
s = sushitrack_update(hdl, dets, n_dets, tracks, MAX_TRACKS, dt, &n);
if (s != SUSHITRACK_SUCCESS && s != SUSHITRACK_BUFFER_OVERFLOW) { /* handle */ }

sushitrack_destroy(hdl);
```

Source: not compiled

`sushitrack_create` returns `NULL` on failure. `sushitrack_create_ex` returns a `sushitrack_status_t` and writes the handle through `out_tracker`, which it sets to `NULL` on every failure. It returns `SUSHITRACK_INVALID_ARGUMENT` for a refused argument or an unregistered `kalman_type`, `SUSHITRACK_OUT_OF_MEMORY` when an allocation fails, and `SUSHITRACK_INITIALIZATION_FAILURE` when a component of the tracker fails to build.

`sushitrack_update` and `sushitrack_get_tracklets` return a `sushitrack_status_t` and write the number of tracks through `out_count`. Output tracks are sorted by lifecycle priority (`Tracked` < `Tentative` < `Lost` < `Removed`). `out_tracks` may be `NULL` only when `max_tracks` is 0. A detection whose box or score is NaN or infinite is ignored, and the tracker logs a WARN.

| Status | Meaning for `sushitrack_update` | `out_count` |
|---|---|---|
| `SUSHITRACK_SUCCESS` | The frame was applied and every track written | Tracks written |
| `SUSHITRACK_BUFFER_OVERFLOW` | The frame was applied; tracks past `max_tracks` were left out, lowest priority first, and a warning logged | `max_tracks` |
| `SUSHITRACK_INVALID_ARGUMENT` | A null handle or `out_count`, a negative count, or a null array with a positive count; nothing was applied | 0 |
| `SUSHITRACK_OUT_OF_MEMORY`, `SUSHITRACK_RUNTIME_ERROR` | The update failed and logged an error; the frame may be partly applied | 0 |

`sushitrack_get_tracklets` reports the same statuses for the tracks the tracker holds.

The track-output `feat` pointer references memory owned by the tracker and is only valid until the next `sushitrack_update` or `sushitrack_destroy`.

## Status codes (`sushitrack_status_t`)

| Code | Value |
|---|---|
| `SUSHITRACK_SUCCESS` | 0 |
| `SUSHITRACK_INVALID_ARGUMENT` | -1 |
| `SUSHITRACK_INITIALIZATION_FAILURE` | -2 |
| `SUSHITRACK_OUT_OF_MEMORY` | -3 |
| `SUSHITRACK_RUNTIME_ERROR` | -4 |
| `SUSHITRACK_BUFFER_OVERFLOW` | -5 |

## Logging

The C API holds no logging call. The library logs through SushiLog, and a C host takes the records with SushiLog's `sl_set_log_sink` from `SushiLog/log/log_c_sink.h`; see [Overview](../architecture/OVERVIEW.md#logging).

## Track output fields

`sushitrack_track_t` includes `track_id`, `age` (tracklet length in frames), `last_seen_frame`, `feat`/`feat_dim`, `x/y/width/height`, `score`, `lost_time` (seconds or frames depending on `time_aware`), `label`, `state` (TrackState enum integer), `kinematic_state` (`0=Stationary, 1=Cruising, 2=Maneuvering, 3=Coasting`), `is_activated`, `is_confirmed`, `hits`, `frames_since_last_update`, and `maneuver_probability`.
