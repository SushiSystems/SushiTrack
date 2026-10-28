# Install release

## Remaining work

| ID | Pri | State | Title | Done when | Blocked by | Source | Opened |
| --- | --- | --- | --- | --- | --- | --- | --- |
| RW-17 | P2 | open | Generate every version number from CMakeLists.txt | the binding and the CLI read the version `project()` declares, and `__version__ = "2.0.0"` is gone |  |  | 2026-10-19 |
| RW-18 | P2 | open | Check for GoogleTest in st doctor | the GoogleTest fragment in `cli/sushistack.deps.toml` carries a check command and describes the default `st build` truly |  |  | 2026-10-19 |
| RW-22 | P2 | blocked | Release v0.1.0 with the inference on SushiAI and OpenCV | the `chore(release): v0.1.0` commit carries the tag `v0.1.0` | RW-17, RW-23 |  | 2026-10-19 |

## Known issues

| ID | Origin | Symptom | Workaround | Fix | Source | Opened |
| --- | --- | --- | --- | --- | --- | --- |
