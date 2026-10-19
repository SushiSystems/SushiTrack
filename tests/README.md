# tests

The tests of SushiTrack. The tree mirrors the public headers: the test of
`include/SushiTrack/<area>/<name>.hpp` is `tests/<area>/test_<name>.cpp`, and a unit whose tests
do not fit one file keeps them in `tests/<area>/test_<name>/`, one file per aspect. Every suite
name opens with its size, `Small_`, `Medium_` or `Large_`, which is its label; the build compiles
every file into `sushitrack_tests`. How to run them is in
[Test suites](../docs/guides/TESTING.md).

## Helpers

| Helper | What it gives the tests |
| --- | --- |
| `captured_log.hpp` | `CapturedLog`, which collects SushiTrack's log records for one test |
| `c_detection_builders.hpp` | `CDetections`: C API detections and the lookups of the tracker scenarios |
| `gating_log_fixture.hpp` | `GatingLogFixture`, which counts the gating log lines of one cost matrix call |
| `measurement_builders.hpp` | `make_box`, the measurement the Kalman filter tests feed |
| `tracklet_builders.hpp` | `make_t` and `make_feat_t`, the tracklets the cost calculator tests build |
