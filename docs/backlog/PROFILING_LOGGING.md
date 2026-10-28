# Profiling logging

## Remaining work

| ID | Pri | State | Title | Done when | Blocked by | Source | Opened |
| --- | --- | --- | --- | --- | --- | --- | --- |
| RW-26 | P2 | open | Measure the profiling overhead and the drops of a benchmark run | a baseline record and a profiled record of MOT17 and DanceTrack give the per-frame cost with the profiler off, with the gate closed and recording, and the… |  | `docs/design/SUSHITRACK_PROFILING/README.md` | 2026-10-26 |
| RW-27 | P2 | blocked | Record the Python pipeline and binding through SushiProfiler | `applications/inference/pipeline.py` records decode, detect, ReID and draw as zones of one record with the tracker's | external: SushiProfiler P9, the Python binding of its zones and records | `docs/design/SUSHITRACK_PROFILING/README.md` | 2026-10-26 |
| RW-28 | P2 | blocked | Log from the Python binding and the inference scripts | `bindings/python/sushitrack.py` and `applications/inference/` log through the Python helper of SushiLog's RW-12, with tests in the home RW-3 names | RW-3 | `docs/design/SUSHITRACK_PROFILING/README.md` | 2026-10-26 |

## Known issues

| ID | Origin | Symptom | Workaround | Fix | Source | Opened |
| --- | --- | --- | --- | --- | --- | --- |
