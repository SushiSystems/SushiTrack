# Benchmarks

Results on the MOT17 and MOT20 train sets and on DanceTrack, comparing SushiTrack with the
bundled ByteTrack and OC-SORT references at matched parameters. Metrics are from TrackEval.
Latency is the mean per-frame `update()` time, single-threaded, excluding I/O.

The page holds two runs. The first ran SushiTrack on the linear Kalman path (`kalman_type=0`)
with the tentative lifecycle, OCM and ORU enabled and appearance ReID off. The second changed
only `kalman_type` to 2, the IMM estimator: 45.8 % overall HOTA against 45.7 %, at 218 against
57 µs per frame. The date, the commit and the machine of the runs were not recorded.

## Summary of the linear Kalman run

### MOT17

| Tracker | HOTA | DetA | AssA | MOTA | IDF1 | IDSW | FP | FN | Lat (µs/frame) |
|---|---|---|---|---|---|---|---|---|---|
| SushiTrack | 47.2 % | 48.2 % | 47.5 % | 55.0 % | 55.5 % | 2849 | 7092 | 185931 | 37 |
| ByteTrack | 44.7 % | 48.5 % | 42.4 % | 55.1 % | 52.5 % | 4773 | 12806 | 177427 | 87 |
| OC-SORT | 44.1 % | 46.4 % | 43.2 % | 53.0 % | 52.2 % | 2836 | 9172 | 191275 | 167 |

### MOT20

| Tracker | HOTA | DetA | AssA | MOTA | IDF1 | IDSW | FP | FN | Lat (µs/frame) |
|---|---|---|---|---|---|---|---|---|---|
| SushiTrack | 44.9 % | 43.9 % | 50.9 % | 50.6 % | 50.1 % | 2904 | 13740 | 1185170 | 318 |
| ByteTrack | 41.9 % | 42.7 % | 46.9 % | 49.9 % | 46.3 % | 3680 | 26843 | 1184486 | 506 |
| OC-SORT | 38.7 % | 39.0 % | 41.5 % | 45.1 % | 44.0 % | 2972 | 29224 | 1254585 | 1181 |

### DanceTrack

| Tracker | HOTA | DetA | AssA | MOTA | IDF1 | IDSW | FP | FN | Lat (µs/frame) |
|---|---|---|---|---|---|---|---|---|---|
| SushiTrack | 44.9 % | 72.0 % | 29.3 % | 85.1 % | 46.5 % | 3568 | 7215 | 42428 | 18 |
| ByteTrack | 37.9 % | 65.2 % | 23.3 % | 82.0 % | 42.8 % | 3863 | 15913 | 43195 | 34 |
| OC-SORT | 45.6 % | 69.7 % | 31.1 % | 80.2 % | 48.5 % | 2949 | 15976 | 50485 | 71 |

### Overall

| Tracker | HOTA | DetA | AssA | MOTA | IDF1 | IDSW | FP | FN | Lat (µs/frame) |
|---|---|---|---|---|---|---|---|---|---|
| SushiTrack | 45.7 % | 60.3 % | 38.3 % | 70.4 % | 50.2 % | 9321 | 28047 | 1413529 | 57 |
| ByteTrack | 40.8 % | 56.7 % | 32.8 % | 68.7 % | 46.8 % | 12316 | 55562 | 1405108 | 103 |
| OC-SORT | 44.3 % | 57.9 % | 36.7 % | 66.5 % | 49.4 % | 8757 | 54372 | 1496345 | 223 |

## Per-sequence results

| Run | Page |
| --- | --- |
| Linear Kalman, `kalman_type=0` | [Linear Kalman run](BENCHMARKS_LINEAR_KALMAN.md) |
| IMM, `kalman_type=2` | [IMM run](BENCHMARKS_IMM.md) |
