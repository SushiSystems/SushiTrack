# SushiTrack Python binding

`sushitrack.py` is a `ctypes` binding to the public C API
(`include/SushiTrack/sushitrack_c.h`) with no dependency outside the standard library. It mirrors
`sushitrack_object_t`, `sushitrack_track_t` and `sushitrack_params_t` field for field and wraps
the entry points in the `Tracker` class, so a caller never writes `ctypes`. `SushiTrack` is an
alias of `Tracker`.

The module depends on the compiled `sushitrack` shared library and on nothing else in this
repository. A change to the C structs must change `SushiTrackObject`, `SushiTrackTrack` and
`SushiTrackParams` here in the same commit; see
[C ABI stability](../../docs/CONTRIBUTING.md#c-abi-stability).

## Installing

Build the library first (`st build`), then install the module:

```bash
pip install .
```

`pip install .` installs `sushitrack.py` only. It does not build or bundle the shared library,
and an installed copy no longer sits beside this repository's `build/` tree, so point it at the
library with `SUSHITRACK_LIB` or `lib_path`; see [Finding the library](#finding-the-library).
Used in place, with `bindings/python` on `PYTHONPATH`, the module finds the library under
`build/` by itself.

`st deploy python` assembles a package that carries both; see
[Deploy packages](../../docs/guides/DEPLOY_PACKAGES.md).

## Use

```python
from sushitrack import Tracker, TrackState

# Any sushitrack_params_t field is a keyword override; the rest keep their defaults.
tracker = Tracker(frame_rate=30, enable_tentative=1, iou_type=1)

for frame in stream:
    # detections: (x, y, w, h, score, label) with top-left x, y
    # (optionally a 7th element: a 1-D appearance embedding for ReID)
    dets   = [(x, y, w, h, score, label), ...]
    tracks = tracker.update(dets, dt=1.0 / 30.0)
    for t in tracks:
        if t.state is TrackState.TRACKED:
            print(t.track_id, t.x, t.y, t.width, t.height)

tracker.close()                       # or use `with Tracker(...) as tracker:`
```

`update` returns a list of `Track` records, a frozen dataclass holding `track_id`, the box,
`score`, `label`, `state`, `kinematic_state`, `age`, `last_seen_frame`, `lost_time`,
`is_activated`, `is_confirmed`, `hits`, `frames_since_last_update`, `maneuver_probability` and
`feat`. `Status`, `TrackState`, `KinematicState` and `LogLevel` are `IntEnum`s.
`set_log_callback(fn)` registers a `fn(level, message)` logger.

## Constructor arguments

| Argument | Effect |
| --- | --- |
| `lib_path` | Path of the shared library; tried before every other location |
| `max_tracks` | Size of the output buffer, 256 by default; `update` returns at most this many tracks |
| `config_path`, `config` | A `sushitrack.json` path, or `"auto"` to let the library resolve the file; `config_path` wins when both are given |
| any `sushitrack_params_t` field | Overrides that field; an unknown name raises `AttributeError` |

With a config and no field override, the library creates the tracker from the file. With field
overrides, the binding seeds the defaults, overlays the config file when one is given, applies
the overrides and creates the tracker from the result, so an override wins over the file.

## Finding the library

`find_library` returns the first of these that exists:

1. the `lib_path` argument;
2. the `SUSHITRACK_LIB` environment variable;
3. the directory that holds `sushitrack.py`, which is where a deploy package puts the library;
4. the `build/` folder two levels above the module, which is this repository's when the module
   is used in place: `bin/Release`, `bin/Debug`, `bin`, `lib`, `Release`,
   `Debug`, then `build/` itself.

It raises `FileNotFoundError` when none exists.
