# Tracker

## Remaining work

| ID | Pri | State | Title | Done when | Blocked by | Source | Opened |
| --- | --- | --- | --- | --- | --- | --- | --- |
| RW-10 | P2 | unmeasured | Suppress duplicates within the tracked pool | a benchmark run shows `remove_duplicate_tracklets` over the tracked pool drops the same-object duplicate identities of dense scenes without losing HOTA |  |  | 2026-10-19 |
| RW-11 | P2 | unmeasured | Gate the IMM on the mixture innovation covariance | a benchmark run compares the gate on the mode-weighted covariance with the gate on the collapsed 8-D estimate |  |  | 2026-10-19 |
| RW-12 | P2 | unmeasured | Run the coordinated-turn update only under a manoeuvre | a benchmark run shows the IMM cost per frame falls with tracking output unchanged |  |  | 2026-10-19 |

## Known issues

| ID | Origin | Symptom | Workaround | Fix | Source | Opened |
| --- | --- | --- | --- | --- | --- | --- |
