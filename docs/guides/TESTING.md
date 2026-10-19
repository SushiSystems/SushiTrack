# Test suites

## Unit tests (`unit_test`)

GTest-based, deterministic, no external data. Targets compiled when `-DBUILD_UNIT_TEST=ON`. Cover:

- `TestKalmanFilter`: initiate/predict/update, Mahalanobis distance, gate flags, LLT cache
- `TestTracklet`: lifecycle transitions, feature EMA
- `TestRect`: TLWH/TLBR/XYWH conversions
- `TestReID`: cosine similarity, max-history lookup
- `TestCostMatrix`: resize semantics, sentinel preservation
- `TestTimingPolicies`: frame vs wall-clock conversion and expiry symmetry
- `TestDistanceStrategies`: each cost calculator (IoU/DIoU/GIoU/SIoU/Cosine/ReIDFirst/PureReID/Fused/GatedIoU/FuseScoreDecorator)
- `TestAssignmentSolvers`: LAPJV correctness on dense and sparse matrices
- `TestTrackerTentative`: adaptive confirmation logic
- `TestTrackerBuffers`: track_buffer expiry under both timing modes
- `TestLogger`: callback dispatch, va_list path
- `TestSushiTrackC` / `TestCAPIErrors`: C-API surface and error codes
- `TestTracker` / `TestIntegration`: end-to-end flow
- `TestBoundaryConditions`: empty inputs, oversized output buffer, null args
- `TestConfiguration`: config-driven parameter parsing

Direct invocation:

```bash
st run unit_test
st run unit_test -- --gtest_filter="TrackletTest.*"
st test --suite unit                    # ctest --test-dir build -L '^unit$'
```

## Regression tests (`regression_test`)

Parameterised GTest binary that iterates over `Scenario × tracker_factory`. For each pair it:

1. Reads `det.txt` frames via `MOTRunner`.
2. Drives the tracker through `ITrackerAdapter` (`SushiTrackAdapter`, `ByteTrackAdapter`, `OCSORTAdapter`).
3. Writes predictions to `<sequence>/pred/<tracker>/<sequence>.txt`.
4. Emits a `[ PERF <tracker> ] <sequence>: <us/frame> us/frame` line read back by the CLI (`cli/sushitrack_cli/services/label.py`).

Config resolution: SushiTrack loads `sushitrack.json` through the library (`$SUSHITRACK_CONFIG`, then an upward search); ByteTrack and OCSORT read `tests/bytetrack.json` and `tests/ocsort.json`.

Direct invocation:

```bash
./build/bin/Release/regression_test
./build/bin/Release/regression_test --gtest_filter="*MOT17_04_DPM_sushitrack*"
SUSHITRACK_CONFIG=./assets/sushitrack.json ./build/bin/Release/regression_test
```

## Integration tests (`integration_test`)

GTest binary that drives the tracker end-to-end exclusively through the C API, as a downstream FFI consumer would. Targets compiled when `-DBUILD_INTEGRATION_TEST=ON`. Cover:

- `TestByteAssociation`: two-stage BYTE association: low-confidence sustain, new-track seeding rules
- `TestOcclusionRecovery`: lost-track buffering, within/beyond-buffer recovery, no id transfer
- `TestMultiObject`: identity separation, counter-moving pass, output capacity/priority contract
- `TestConfigVariations`: IoU variant sweep, XYAH coordinates, NSA Kalman
- `TestAdvancedPath`: ReID + Mahalanobis gating
- `TestFullStack`: full advanced stack enabled at once
- `TestKinematicState`: IMM-derived regime reported end-to-end (Stationary/Cruising/Maneuvering/Coasting)
- `TestObservationMomentum`: observation-centric momentum re-ranking and ORU recovery
- `TestDuplicateSuppression`: cross-pool duplicate removal by track lifetime
- `TestTimeAware`: wall-clock (`time_aware`) stepping and expiry

Direct invocation:

```bash
st run integration_test
st test --suite integration             # ctest --test-dir build -L '^integration$'
```

## CLI tests (`cli/tests/`)

A pytest suite for the `st` package. It needs no build.

`sushitrack_cli` imports `sushicore` at module load, so the suite cannot be collected without it.
sushicore is an ordinary PyPI dependency of the CLI, not a checkout to be found on disk, so
`cli/tests/conftest.py` searches for nothing: sushicore is installed or it is not, and
`conftest.py` raises when it is not.

The package under test sits in `cli/`, one level above `conftest.py`, and the suite is run from the
repository root as often as from `cli/`. `conftest.py` puts `cli/` on `sys.path` so the same
command works from either.

Nothing in the fixtures reaches the machine. A test that runs a real tool, prints through the
shared console, or reads the developer's own project says something different on someone else's
machine.

Direct invocation:

```bash
python -m pytest cli/tests -q
```
