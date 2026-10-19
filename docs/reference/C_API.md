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
int n = sushitrack_update(hdl, dets, n_dets, tracks, MAX_TRACKS, dt);

sushitrack_destroy(hdl);
```

Source: not compiled

`sushitrack_create` returns `NULL` on failure; `sushitrack_create_ex` returns a `sushitrack_status_t` code and writes the handle through `out_hdl`. `sushitrack_update` returns the number of tracks written. Output tracks are sorted by lifecycle priority (`Tracked` < `Tentative` < `Lost` < `Removed`); the buffer is truncated if too small, with a warning logged.

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
