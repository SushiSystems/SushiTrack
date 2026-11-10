# Continuous integration

## Remaining work

| ID | Pri | State | Title | Done when | Blocked by | Source | Opened |
| --- | --- | --- | --- | --- | --- | --- | --- |
| RW-24 | P2 | blocked | Run the MOT benchmarks in the nightly workflow | `nightly.yml` runs `st eval` on a runner that holds the sequences and records the figures | external: no runner holds the MOT17, MOT20 and DanceTrack sequences, which are not in git |  | 2026-10-21 |
| RW-30 | P2 | open | Clear the known findings of the shared checkers | tools/known_findings.toml holds no entry and tools/check_all.py is clean without it |  | `tools/check_all.py` | 2026-11-05 |
| RW-42 | P2 | open | Register the test cases as CTest shards per size and run them once | Each program's Small_, Medium_ and Large_ cases run in shards that write XML to test_results/gtest |  | `cmake/SushiTrackTests.cmake` | 2026-11-10 |

## Known issues

| ID | Origin | Symptom | Workaround | Fix | Source | Opened |
| --- | --- | --- | --- | --- | --- | --- |
| KI-18 | own | A Debug tree fails to configure on Linux: the distribution GTest names no Debug import | Build with ST_BUILD_TYPE=release or relwithdebinfo, as ci.yml does | none, the Linux lanes build Release and no Debug tree is needed there | `cmake/SushiTrackTests.cmake` | 2026-10-30 |
| KI-19 | dependency | st setup installs no SushiLog or SushiProfiler CLI, so st build says 'cannot run sl' | Run python -m pip install ~/.sushisystems/sources/*/*/cli after st setup, as ci.yml does |  | `.github/workflows/ci.yml` | 2026-10-30 |
