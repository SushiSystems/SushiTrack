# tracker_bridge

Runs SushiTrack, ByteTrack and OC-SORT behind one interface, `TrackerAdapter`, for the MOT
benchmark and the Python inference pipeline. It is a library, not a program.

| File | Holds |
| --- | --- |
| `tracker_adapter.hpp` | `TrackerAdapter`, the interface |
| `sushitrack_adapter.hpp` | `SushiTrackAdapter`, which drives SushiTrack through its public C API, from given parameters or from the defaults overlaid with a configuration file |
| `byte_track_adapter.hpp`, `oc_sort_adapter.hpp` | The reference trackers from `third_party/bytetrack` and `third_party/ocsort`, built from a settings file or from a parsed settings document |
| `throwing_tracker_adapter.hpp` | `ThrowingTrackerAdapter`, an in-memory adapter whose `update` always throws, for the tests of a caller's failure handling |
| `mot_detection.hpp`, `track_result.hpp` | What an adapter takes in and hands back |
| `json_settings.hpp` | Reads the reference trackers' settings from a file or a stream and logs a missing file, a missing section and a key that is not a number |
| `tracker_bridge_log_category.hpp` | `K_TRACKER_BRIDGE_LOG_CATEGORY`, under which the adapters and the C entry points log |
| `bytetrack.json`, `ocsort.json` | Those settings, for the benchmark |
| `tracker_registry.hpp` | `TrackerRegistry`, which builds an adapter from its name; `create_tracker` uses the process-wide one, and a caller may own another |
| `default_trackers.hpp` | `register_default_trackers`, which registers `sushitrack`, `bytetrack` and `ocsort` in a registry |
| `tracker_builder.hpp` | `build_tracker`, which builds the adapter a registry holds under a name and logs a null or unknown name |
| `tracker_bridge.h`, `tracker_bridge.cpp` | The C entry points `create_tracker`, `tracker_process_c` and `delete_tracker`; `tracker_process_c` returns a `tracker_bridge_status_t` and no exception leaves an entry point |

The build makes `tracker_adapters`, an interface target the benchmark links, and
`tracker_bridge`, the shared library `applications/inference/tracker_bridge.py` loads with
`ctypes`. Both depend on the shared `sushitrack` library, never on its internals.
