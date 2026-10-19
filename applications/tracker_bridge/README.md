# tracker_bridge

Runs SushiTrack, ByteTrack and OC-SORT behind one interface, `TrackerAdapter`, for the MOT
benchmark and the Python inference pipeline. It is a library, not a program.

| File | Holds |
| --- | --- |
| `tracker_adapter.hpp` | `TrackerAdapter`, the interface |
| `sushitrack_adapter.hpp` | `SushiTrackAdapter`, which drives SushiTrack through its public C API |
| `byte_track_adapter.hpp`, `oc_sort_adapter.hpp` | The reference trackers from `third_party/bytetrack` and `third_party/ocsort` |
| `mot_detection.hpp`, `track_result.hpp` | What an adapter takes in and hands back |
| `json_settings.hpp` | Reads the reference trackers' settings |
| `bytetrack.json`, `ocsort.json` | Those settings, for the benchmark |
| `tracker_registry.hpp` | `TrackerRegistry`, which builds an adapter from its name |
| `tracker_bridge.h`, `tracker_bridge.cpp` | The C entry points `create_tracker`, `tracker_process_c` and `delete_tracker` |

The build makes `tracker_adapters`, an interface target the benchmark links, and
`tracker_bridge`, the shared library `applications/inference/tracker_bridge.py` loads with
`ctypes`. Both depend on the shared `sushitrack` library, never on its internals.
