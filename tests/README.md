# tests

The tests of SushiTrack. The tree mirrors the public headers: the test of
`include/SushiTrack/<area>/<name>.hpp` is `tests/<area>/test_<name>.cpp`, and a unit whose tests
do not fit one file keeps them in `tests/<area>/test_<name>/`, one file per aspect. Every suite
name opens with its size, `Small_`, `Medium_` or `Large_`, which is its label; the build compiles
every file into `sushitrack_tests`. How to run them is in
[Test suites](../docs/guides/TESTING.md).

## Shards

`cmake/SushiTrackTests.cmake` registers the cases of a program in shards, because a case run in a
process of its own pays the process and runtime start-up, which is most of a small case. Per
program and per size there are N CTest tests named `<program>.<size>_shard_<i>of<N>`, each running
the program with `--gtest_filter=<prefix>*:*/<prefix>*` (without the quarantined cases),
`GTEST_TOTAL_SHARDS=N` and `GTEST_SHARD_INDEX=i`. GoogleTest splits the filtered cases between the
shards, so every case runs in exactly one. N is the case count of the size divided by the cases one
shard holds, rounded up: `SUSHITRACK_SHARD_CASES_SMALL` (300), `_MEDIUM` (25) and `_LARGE` (1). The
count comes from the `TEST`, `TEST_F`, `TEST_P`, `TYPED_TEST` and `TYPED_TEST_P` declarations and
the `INSTANTIATE_*` lines of the program's sources, so N is a floor. A shard's timeout is the
size's timeout times the cases it holds. The quarantined cases of a size run apart in
`<program>.<size>_quarantined`.

Each shard writes its cases to `test_results/gtest/<shard>.xml` in the build tree, which
`tools/testing/test_budget.py --cases <folder>` reads. A suite name belongs to one fixture in a
program: `TEST` and `TEST_P` cannot share a suite name. The name a parameter generator returns
holds no `-`, which a GoogleTest filter reads as the start of its exclusions.

## Helpers

| Helper | What it gives the tests |
| --- | --- |
| `captured_log.hpp` | `CapturedLog`, which collects SushiTrack's log records for one test through SushiLog's scoped in-memory sink, `LogCapture`, and puts back the levels it set |
| `c_detection_builders.hpp` | `CDetections`: C API detections, the frame calls and the lookups of the tracker scenarios |
| `gating_log_fixture.hpp` | `GatingLogFixture`, which counts the gating log lines of one cost matrix call |
| `measurement_builders.hpp` | `make_box`, the measurement the Kalman filter tests feed |
| `support/default_params.hpp` | `TestSupport::default_params`, the library's default parameters, read from no file |
| `support/kalman_registry_builders.hpp` | `TestSupport::library_kalman_registry`, a `KalmanFilterRegistry` the test owns that holds the three library filters |
| `support/tolerance.hpp` | `TestSupport::measure_*_error` and `is_within`, which derive a floating-point tolerance from a rounding bound; a copy of SushiSkills' `tools/testing/support/tolerance.hpp` |
| `tracklet_builders.hpp` | `make_t` and `make_feat_t`, the tracklets the cost calculator tests build |
