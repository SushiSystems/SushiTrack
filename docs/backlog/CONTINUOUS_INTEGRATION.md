# Continuous integration

## Remaining work

| ID | Pri | State | Title | Done when | Blocked by | Source | Opened |
| --- | --- | --- | --- | --- | --- | --- | --- |
| RW-24 | P2 | blocked | Run the MOT benchmarks in the nightly workflow | `nightly.yml` runs `st eval` on a runner that holds the sequences and records the figures | external: no runner holds the MOT17, MOT20 and DanceTrack sequences, which are not in git |  | 2026-10-21 |

## Known issues

| ID | Origin | Symptom | Workaround | Fix | Source | Opened |
| --- | --- | --- | --- | --- | --- | --- |
| KI-18 | own | A Debug tree fails to configure on Linux: the distribution GTest names no Debug import | Build with ST_BUILD_TYPE=release or relwithdebinfo, as ci.yml does | none, the Linux lanes build Release and no Debug tree is needed there | `cmake/SushiTrackTests.cmake` | 2026-10-30 |
| KI-19 | dependency | st setup installs no SushiLog or SushiProfiler CLI, so st build says 'cannot run sl' | Run python -m pip install ~/.sushisystems/sources/*/*/cli after st setup, as ci.yml does |  | `.github/workflows/ci.yml` | 2026-10-30 |
