# SushiTrack

A C++17 multi-object tracking library built on the ByteTrack two-stage
association pipeline. It uses an interface-driven, dependency-injected
architecture: motion models, cost calculators, gating, and the assignment
solver are selected at runtime through the C API.

The default configuration runs the core path (IoU/DIoU two-stage association,
tentative validation, Kalman prediction, lost-track buffering, LAPJV
assignment). The following extensions are optional and **disabled by default**:
four IoU variants, appearance ReID fusion, Mahalanobis gating, a
Noise-Scale-Adaptive Kalman filter, a four-model IMM motion estimator
(`kalman_type=2`), observation-centric momentum and re-update, real-time
(`time_aware`) operation.

A stable C API, a dependency-free Python CLI (`ds`), a MOTChallenge and DanceTrack
regression harness, a GTest unit suite, and a YOLOX-driven video inference
pipeline are included.

---

## Contents

- [Architecture](#architecture)
- [Features](#features)
- [Requirements](#requirements)
- [Project Structure](#project-structure)
- [Installation](#installation)
  - [CLI installer (PowerShell / Bash)](#cli-installer-powershell--bash)
  - [Conda environment](#conda-environment)
  - [Docker](#docker)
- [Build](#build)
- [CLI Reference](#cli-reference)
  - [project build](#project-build)
  - [project test all](#project-test-all)
  - [project test unit](#project-test-unit)
  - [project test integration](#project-test-integration)
  - [project test label](#project-test-label)
  - [project test inference](#project-test-inference)
  - [project clean](#project-clean)
  - [docker build / docker run](#docker-build--docker-run)
- [C API](#c-api)
- [Python Binding](#python-binding)
- [Deploy Packages](#deploy-packages)
  - [`--deploy cpp` layout](#--deploy-cpp-layout)
  - [`--deploy python` layout](#--deploy-python-layout)
  - [How discovery works](#how-discovery-works)
- [Data Layout](#data-layout)
- [Configuration Parameters](#configuration-parameters)
- [Test Suites](#test-suites)
- [Evaluation Pipeline](#evaluation-pipeline)
- [Inference Pipeline](#inference-pipeline)
- [Ultralytics Integration](#ultralytics-integration)
- [Internal Tools](#internal-tools)
- [Benchmark Results](#benchmark-results)
- [License](#license)

---

## Architecture

The runtime is organised around interfaces that the tracker depends on. Concrete implementations are wired up by the C-API entry point (`sushitrack_create_ex` in `source/SushiTrackC.cpp`) based on the `sushitrack_params_t` flags. The core orchestrator `Tracker` (see `include/SushiTrack/Tracker.h`) consumes only the abstractions.

| Interface | Purpose | Concrete implementations |
|---|---|---|
| `IStatePredictor` | State propagation/update | `KalmanFilter`, `NSAKalmanFilter` |
| `IGateValidator` | Mahalanobis gating | `KalmanFilter`, `NSAKalmanFilter`, `NullGateValidator` |
| `IKalmanFilter` | Unified predictor + validator | `KalmanFilter`, `NSAKalmanFilter` |
| `IKalmanFactory` | Per-tracklet predictor factory | `GenericKalmanFactory` (registry-driven) |
| `ICostCalculator` | Pairwise association cost | `IoUCostCalculator`, `DIoUCostCalculator`, `GIoUCostCalculator`, `SIoUCostCalculator`, `CosineCostCalculator`, `ReIDFirstCostCalculator`, `PureReIDCostCalculator`, `FusedCostCalculator`, `GatedIoUCostCalculator`, `FuseScoreDecorator` |
| `IAssignmentSolver` | Bipartite assignment | `LAPJVSolver` (Jonker–Volgenant with active-row/column reduction) |
| `ITimingPolicy` | dt normalisation, expiry | `FrameTimingPolicy`, `WallClockTimingPolicy` |

`KalmanFilterRegistry` is a singleton plug-in registry. New Kalman variants register themselves through `KalmanFilterRegistrar<T>` and become selectable by `kalman_type`.

Track lifecycle is governed by `TrackState` (`New`, `Tracked`, `Lost`, `Removed`, `Tentative`). When the IMM estimator is selected (`kalman_type=2`), each track also reports a motion regime (`Stationary`, `Cruising`, `Maneuvering`, or `Coasting`) derived from the IMM mode probabilities, plus a continuous `maneuver_probability` on the output track.

---

## Features

**Two-stage ByteTrack association** — high-confidence detections (`score >= track_thresh`) match first; remaining tracks then attempt to absorb low-confidence detections.

**Tentative validation** — when `enable_tentative=1`, new tracks must accumulate `tentative_confirm_hits` consecutive matches before promotion. Confirmation hits are adaptive: `tentative_fast_thresh` (default 0.85) allows single-hit confirmation; `tentative_slow_thresh` (default 0.40) adds `tentative_slow_hits_offset` extra hits; below the slow threshold adds `tentative_vslow_hits_offset` hits. `tentative_match_thresh` governs tentative-pass cost. Expiry uses `tentative_max_miss_frames` in frame mode (`time_aware=0`) or `tentative_max_age_seconds` in wall-clock mode (`time_aware=1`), and `tentative_init_suppress_iou` drops a new-track detection overlapping a confirmed track.

**IoU variants** — `iou_type` selects `IoUCostCalculator` (0), `DIoUCostCalculator` (1, default), `GIoUCostCalculator` (2), or `SIoUCostCalculator` (3) for the primary/secondary stages. Tentative and duplicate stages are pinned to plain IoU so their thresholds keep their calibration.

**Score fusion** — `FuseScoreDecorator` rescales the primary cost matrix by detection score: `fused = 1 − (1 − inner) * score`. Sentinel cells are preserved. Applied only to the primary stage. Toggled with `fuse_score`.

**Visual ReID** — `enable_reid=1` activates one of three paths depending on `enable_mahalanobis`:
- ReID-first (default): `ReIDFirstCostCalculator` for the primary/secondary passes with an IoU fallback, plus `PureReIDCostCalculator` for lost recovery.
- Fused: `FusedCostCalculator` combines IoU, cosine appearance and Mahalanobis gating (when `enable_mahalanobis=1`).
- Lost recovery: `PureReIDCostCalculator` (`reid_recovery_thresh`), active in all ReID paths.

Visual features are L2-normalised and exponentially smoothed (`feature_momentum`, default 0.9). A per-tracklet feature history holds up to `max_feature_history` (default 100) vectors and `getMaxFeatureSimilarity` returns the max cosine similarity against the history.

**Mahalanobis gating** — `enable_mahalanobis=1` filters association candidates via a chi-squared test on the projected state distribution. `gating_thresh` defaults to 9.4877 (95 % CI for 4 DoF). `GatedIoUCostCalculator` combines IoU with the gate; `motion_cost_weight` blends the squared distance into the cost.

> **Why the gate uses only the box (`xyah`), not velocity.**
> The Kalman state carries eight numbers: the four box numbers (`[cx, cy, a, h]`, or `[x, y, w, h]`) plus their four velocities. A detector only ever reports a box, never a velocity, so the only quantity a track can be compared against is its predicted box. The Mahalanobis gate therefore projects the 8-D state down to the 4-D box it would produce and measures the distance in that 4-D box space, scaling the box residual by the predicted box uncertainty. The velocities are used to *predict* where the box goes, but are never part of the distance, because there is no measured velocity to compare them to. This is why the chi-squared test has **4** degrees of freedom (hence the 9.4877 default = 95 % for 4 DoF) and not 8 — feeding in the velocity dimensions would be comparing against numbers no detection contains, which would distort the gate. This holds identically for the IMM estimator (`kalman_type=2`): its internal model bank is collapsed to the same 4-D box estimate before gating, so the gate sees `xyah` only, exactly as the plain Kalman path does. This 4-D-measurement convention is the standard one shared by SORT, DeepSORT, ByteTrack and OC-SORT.

**NSA Kalman filter** — `kalman_type=1` selects `NSAKalmanFilter`, a variant that scales measurement noise by detection confidence in both `initiate` and `project`.

**IMM motion estimator** — `kalman_type=2`. A four-model Interacting Multiple Model filter (constant velocity, constant acceleration, near-constant-position, and a nonlinear coordinated-turn mode handled with an unscented update) replaces the single Kalman model. Its mode probabilities yield a continuous `maneuver_probability` (mass on the manoeuvring models) and a discrete `kinematic_state` (`Stationary`/`Cruising`/`Maneuvering`, or `Coasting` while Lost), consumed by association cost shaping, lost-buffer scaling and OCM weighting. See the IMM section of [architecture/OVERVIEW.md](architecture/OVERVIEW.md).

**Coordinate representations** — `enable_xyah=0` (default) uses TLWH state (`[x, y, w, h, vx, vy, vw, vh]`); `enable_xyah=1` uses centroid + aspect ratio + height (`[cx, cy, a, h, …]`). `iou_plus_one=1` (default) uses inclusive box dimensions consistent with the ByteTrack reference.

**Primary association mode** — `primary_assoc_mode=1` (default) runs a single global LAPJV solve over the high-confidence pool. `primary_assoc_mode=0` retains a legacy age-cascade path.

**Duplicate suppression** — `duplicate_iou_thresh` (default 0.15, i.e. IoU > 0.85) removes redundant tracklets after each frame using a plain-IoU calculator.

**Observation-centric momentum (OCM)** — `enable_ocm=1` re-ranks the primary cost with a bounded velocity-direction term (OC-SORT VDC). For a track with a fresh, fast-enough, eligible-regime observed direction, it subtracts `clamp(ocm_vdc_weight * cos(theta), -ocm_vdc_weight, +ocm_vdc_weight)` from each in-gate primary cost cell, where `theta` is the angle between the track's observed direction of travel (measured over `ocm_delta_t` observed frames, not from Kalman velocity) and the direction from the last observation to the candidate. The term is bounded, so it only re-ranks candidates already inside the gate. `ocm_min_velocity`, `ocm_max_age`, and `ocm_maneuver_weight_scale` gate and scale the term by observed speed, staleness, and motion regime (Cruising full weight, Maneuvering scaled, Stationary/Coasting skipped).

**Observation-centric re-update (ORU)** — `enable_oru=1` repairs the velocity that drifts while a track coasts through an unobserved gap. At each observation the estimator state is checkpointed; on recovery after a gap of at least two frames the estimator is rolled back to the checkpoint and the gap is replayed as predict/update steps along a straight-line virtual trajectory from the pre-loss observation to the recovery detection.

**LAPJV solver** — `LAPJVSolver` runs the Jonker–Volgenant algorithm with active-row/column reduction. Workspace buffers are reused across frames to remove per-frame heap pressure.

**Timing policies** — `time_aware=0` uses `FrameTimingPolicy` (dt=1.0 each call, expiry by frame count). `time_aware=1` uses `WallClockTimingPolicy`: dt is treated as real seconds and multiplied by `frame_rate` to obtain a KF step, capped at 5.0. Track expiry compares accumulated wall time against `track_buffer / frame_rate` seconds (strict >, with a 1e-4 epsilon for symmetry with the frame-counted path).

**C API** — `sushitrack_c.h` exposes an opaque `sushitrack_tracker_t` plus `sushitrack_get_default_params`, `sushitrack_create`, `sushitrack_create_ex`, `sushitrack_update`, `sushitrack_destroy`, `sushitrack_set_log_callback`. Suitable for FFI integration from Python, Rust, or any C-FFI language.

**Logging** — `SUSHITRACK_LOG_DEBUG/INFO/WARN/ERROR` macros route to a single global callback registered via `sushitrack_set_log_callback`. The callback is stored atomically; setting/clearing it is thread-safe.

**Thread safety** — `Tracklet` state access is mutex-protected for `getMean`/`getCovariance`/`getMeasurement`. The `KalmanFilter` LLT projection cache is mutex-protected.

---

## Requirements

| Component | Version |
|---|---|
| C++ compiler | C++17 (MSVC 19.20+, GCC 9+, Clang 10+) |
| CMake | ≥ 3.14 |
| Eigen | vendored at `third_party/eigen` |
| GoogleTest | provided via conda or system package |
| Ninja | provided via conda |
| Python | 3.11 |
| Docker (optional) | Engine ≥ 20.10 with NVIDIA Container Toolkit for CUDA inference |

Python runtime dependencies are defined by `environment.yml` (the source of
truth): `numpy==2.4.4`, `pandas==3.0.2`, `scipy==1.17.1`, `pytest==9.0.3`, `absl-py==2.4.0`, `loguru==0.7.3`, `matplotlib==3.10.9`, `opencv-python==4.13.0.92`, `pillow==12.2.0`, `pycocotools==2.0.11`, `rich==15.0.0`, `tqdm==4.67.3`. PyTorch (`torch`/`torchvision`/`torchaudio`) is installed from the conda `pytorch` channel with `pytorch-cuda=12.1`. The `Dockerfile` mirrors the same package set via pip and pins PyTorch to `torch==2.5.1 torchvision==0.20.1 torchaudio==2.5.1` from the cu121 wheel index.

Compiler flags: MSVC compiles with `/W3 /O2 /Ot /fp:fast /arch:AVX2 /DNDEBUG`; GCC/Clang compile with `-Wall -O3 -ffast-math -funroll-loops -DNDEBUG`. Library is built `SHARED`; on Windows `CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS` is `ON`. Outputs are unified under `${CMAKE_BINARY_DIR}/bin` and `${CMAKE_BINARY_DIR}/lib`.

---

## Project Structure

```
sushitrack/
├── CMakeLists.txt                # Top-level build; defines library + 3rdparty subdirs + tests
├── Dockerfile                    # CUDA 12.1 base, system deps, Python wheels, alias setup
├── environment.yml               # Conda env: python 3.11, cmake, gtest, ninja, pip wheels
├── sushitrack.json                # Runtime config loaded by the library (see Tracker config files)
│
├── include/
│   ├── sushitrack_c.h              # C-compatible public API
│   └── SushiTrack/
│       ├── Tracker.h    # Core orchestrator
│       ├── types.hpp      # Status codes, log levels, callback typedef
│       ├── TrackerConfig.h       # C++ config struct (mirror of sushitrack_params_t)
│       ├── Tracklet.h            # Per-track state, lifecycle, feature history
│       ├── IMMFilter.h          # IMM multi-model estimator
│       ├── KalmanFilter.h        # Linear Kalman filter
│       ├── NSAKalmanFilter.h     # Noise-Scale Adaptive variant
│       ├── KalmanFilterRegistry.h# Plug-in registry
│       ├── KalmanTypes.h         # Eigen matrix typedefs
│       ├── KinematicState.h      # Motion-regime enum (Stationary/Cruising/Maneuvering/Coasting)
│       ├── IKalmanFilter.h       # IStatePredictor + IGateValidator
│       ├── IKalmanFactory.h      # Predictor factory
│       ├── IStatePredictor.h     # Predict/update interface
│       ├── IGateValidator.h      # Gating interface + NullGateValidator
│       ├── ICostCalculator.h     # Cost-matrix interface
│       ├── IAssignmentSolver.h   # Solver interface
│       ├── DistanceStrategies.h  # All cost calculators
│       ├── AssignmentSolvers.h   # LAPJVSolver
│       ├── LAPJV.h               # Low-level Jonker–Volgenant routine
│       ├── CostMatrix.h          # Contiguous 2D cost storage
│       ├── ITimingPolicy.h       # Timing strategy interface
│       ├── TimingPolicies.h      # Frame + WallClock policies
│       ├── Logger.h              # Macros + callback dispatcher
│       ├── Object.h              # Detection input struct
│       └── Rect.h                # Bounding-box template
│
├── source/                          # Library implementations (mirrors include/)
│
├── bindings/
│   └── python/
│       └── sushitrack.py          # ctypes binding to the public C API
│
├── third_party/
│   ├── eigen/                    # Header-only linear algebra
│   ├── nlohmann/                 # Header-only JSON (config parsing)
│   ├── bytetrack/                # Reference ByteTrack for regression comparison
│   ├── ocsort/                   # Reference OC-SORT for regression comparison
│   ├── TrackEval/                # Official MOTChallenge evaluator (used optionally)
│   ├── yolox/                    # YOLOX detector (used by the inference pipeline)
│   ├── weight/                   # Detector / ReID model weights
│   └── video/                    # Sample input clips
│
├── tests/
│   ├── CMakeLists.txt            # Options BUILD_UNIT_TEST / BUILD_REGRESSION_TEST / BUILD_INTEGRATION_TEST
│   ├── bytetrack.json            # ByteTrack regression config
│   ├── ocsort.json               # OCSORT regression config
│   ├── unit/                     # GTest deterministic unit tests
│   │   ├── CMakeLists.txt
│   │   ├── UnitTest.cpp          # GTest main
│   │   ├── TestKalmanFilter.cpp
│   │   ├── TestTracklet.cpp
│   │   ├── TestRect.cpp
│   │   ├── TestReID.cpp
│   │   ├── TestCostMatrix.cpp
│   │   ├── TestTimingPolicies.cpp
│   │   ├── TestDistanceStrategies.cpp
│   │   ├── TestAssignmentSolvers.cpp
│   │   ├── TestTrackerTentative.cpp
│   │   ├── TestTrackerBuffers.cpp
│   │   ├── TestLogger.cpp
│   │   ├── TestSushiTrackC.cpp
│   │   ├── TestTracker.cpp
│   │   ├── TestCAPIErrors.cpp
│   │   ├── TestIntegration.cpp
│   │   ├── TestBoundaryConditions.cpp
│   │   └── TestConfiguration.cpp
│   │
│   ├── integration/             # GTest end-to-end suite over the C API
│   │   ├── CMakeLists.txt
│   │   ├── Main.cpp
│   │   ├── Helpers.h
│   │   ├── TestByteAssociation.cpp
│   │   ├── TestOcclusionRecovery.cpp
│   │   ├── TestMultiObject.cpp
│   │   ├── TestConfigVariations.cpp
│   │   ├── TestAdvancedPath.cpp
│   │   ├── TestFullStack.cpp
│   │   ├── TestKinematicState.cpp
│   │   ├── TestObservationMomentum.cpp
│   │   ├── TestDuplicateSuppression.cpp
│   │   └── TestTimeAware.cpp
│   │
│   └── regression/
│       ├── CMakeLists.txt        # regression_test executable + tracker_bridge shared lib
│       ├── RegressionTest.cpp    # GTest entry; parameterised over (scenario × tracker)
│       ├── ScenarioList.h        # MOT17, MOT20, DanceTrack scenarios
│       ├── ITrackerAdapter.h     # Unified adapter interface
│       ├── SushiTrackAdapter.h    # SushiTrack adapter
│       ├── ByteTrackAdapter.h    # ByteTrack adapter (reference)
│       ├── OCSORTAdapter.h       # OC-SORT adapter (reference)
│       ├── TrackerRegistry.{h,cpp} # C-API for the Python tracker_bridge
│       ├── MOTRunner.h           # det.txt reader + prediction writer
│       ├── TestInfra.h           # Per-frame timing harness; emits "[ PERF ] " lines
│       │
│       ├── evaluator/            # Python evaluation entry points
│       │   ├── __init__.py
│       │   ├── trackeval_runner.py
│       │   ├── report_generator.py
│       │   ├── run_eval.py
│       │   └── data/             # MOT17 / MOT20 / DanceTrack sequences (det.txt + gt.txt)
│       │
│       └── inference/            # Python video inference pipeline
│           ├── demo.py           # CLI front-end
│           ├── pipeline.py       # IDetector, YOLOXDetector, VideoPipeline
│           └── tracker_bridge.py # SushiTrack -> public binding; ByteTrack/OCSORT -> shim
│
├── docs/
│   ├── README.md                 # This document
│   ├── CONTRIBUTING.md           # Engineering standards
│   ├── ARCHITECTURE.md           # Feature and internal-structure reference
│   ├── CHANGELOG.md
│   └── benchmarks/PRIVATE.md     # Internal benchmark log
│
└── cli/                          # `ds` CLI — stdlib only, no third-party deps
    ├── ds.py                     # Launcher (python3 cli/ds.py ...)
    ├── ds.sh                     # Wrapper (sh)
    ├── ds.bat                    # Wrapper (cmd/PowerShell)
    ├── install.sh                # Session installer (bash/zsh)
    ├── install.ps1               # Session installer (PowerShell)
    ├── config.json               # Tracked CLI defaults (paths, build, docker)
    ├── config.local.json         # Per-machine overrides (not tracked)
    ├── Dockerfile.cli            # Bare python3 image proving the zero-dep claim
    ├── sushitrack_cli/
    │   ├── cli.py                # Command declarations (one thin layer)
    │   ├── app.py                # Declarative argparse layer
    │   ├── config.py             # Layered JSON config + path resolution
    │   ├── env.py                # MSVC (vcvars) environment snapshotting
    │   ├── proc.py               # Subprocess helpers
    │   ├── console.py            # Semantic output facade
    │   ├── theme.py              # Colors
    │   ├── icons.py              # Glyphs
    │   ├── renderer.py           # ANSI backend (tables, rules, panels)
    │   └── services/             # build, tests, evaluate, deploy, docker,
    │                             #   diag, discovery, runner
    └── misc/
        ├── clean_preds.sh        # Wipes pred/ directories under evaluator/data
        ├── clean_preds.bat       # Wipes pred/ directories under evaluator/data (Windows)
        └── construct_trackeval_data.ps1 # Links GT + copies preds into TrackEval layout
```

---

## Installation

### CLI installer (PowerShell / Bash)

The CLI requires **nothing but `python3`** — no Typer, no Rich, no loguru, and no
pip install. It can always be run directly:

```bash
python3 cli/ds.py --help
```

The installer registers `ds` for the current session and prepends `cli/` to `PATH`.

PowerShell:

```powershell
Unblock-File ./cli/install.ps1
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
. ./cli/install.ps1
```

Bash/Zsh:

```bash
source cli/install.sh
```

To make permanent, append the printed lines to `$PROFILE` (PowerShell) or `~/.bashrc` / `~/.zshrc` (Unix).

### Conda environment

```bash
conda env create -f environment.yml
conda activate sushitrack
```

`environment.yml` installs Python 3.11, CMake, Ninja, GoogleTest, PyTorch from the conda `pytorch` channel (`pytorch-cuda=12.1`), and the remaining pip dependency set (numpy/pandas/scipy/opencv/loguru/rich/pytest/etc.).

### Docker

```bash
ds docker build      # builds image from ./Dockerfile (CUDA 12.1, Ubuntu 22.04)
ds docker run        # docker run -it --rm -v $(pwd):/workspace/sushitrack sushitrack bash
```

The Dockerfile pins the same wheel versions as `environment.yml`. The project root is bind-mounted at `/workspace/sushitrack`; output files written inside the container are immediately visible on the host. `ds` is symlinked into `/usr/local/bin`, so it works interactively and as `docker run ... ds build` alike.

`cli/Dockerfile.cli` builds the CLI alone on a bare `python:3-slim` — no compiler, no pip packages — which is how the zero-dependency claim is verified:

```bash
docker build -f cli/Dockerfile.cli -t sushitrack-cli .
docker run --rm -v "$PWD:/workspace/sushitrack" sushitrack-cli doctor
```

---

## Build

The build uses CMake with three options:

| Option | Default | Effect |
|---|---|---|
| `BUILD_UNIT_TEST` | OFF | Compiles `unit_test` GTest executable |
| `BUILD_REGRESSION_TEST` | OFF | Compiles `regression_test` executable and `tracker_bridge` shared library |
| `BUILD_INTEGRATION_TEST` | OFF | Compiles `integration_test` GTest executable |

Direct CMake invocation:

```bash
cmake -B build -S . -G Ninja -DBUILD_UNIT_TEST=ON -DBUILD_REGRESSION_TEST=ON -DBUILD_INTEGRATION_TEST=ON
cmake --build build --config Release
```

Each test target registers with CTest under a single label — `unit`,
`integration`, `regression` — set on its `gtest_discover_tests` call, and
`enable_testing()` at the top level makes the root `CTestTestfile.cmake` recurse
into the test directories. So `ctest` works from the build root:

```bash
ctest --test-dir build -L '^unit$' --output-on-failure     # what `ds test --suite unit` runs
ctest --test-dir build --print-labels
```

Through the CLI (preferred). Test targets are built **by default**; there is no
per-suite build flag — what to *run* is selected at test time by CTest label:

```bash
ds build                      # library + every test target (default)
ds build --no-test            # library only
ds build --type debug         # release (default) | debug | relwithdebinfo | minsizerel
ds build --clean              # wipe build/ first; combinable with the above
ds build -j 8                 # parallel jobs (default: CPU count)
ds build -D CMAKE_UNITY_BUILD=ON   # extra -D passed straight to cmake
ds build --deploy cpp         # build, then assemble package/ for C/C++ consumers
ds build --deploy python      # build, then assemble package/ for Python consumers
```

cmake configure runs only when the requested configuration (build type,
tests on/off, generator, extra defines) differs from what the tree was configured
with — recorded in `build/.sushitrack_config`. A configuration change reconfigures
from scratch, because a single-config generator bakes the build type into its
cache and would otherwise silently hand back the previous one.

Artefacts: `build/bin/{Release,Debug}/sushitrack.{dll,so}`, `build/bin/.../unit_test{,.exe}`, `build/bin/.../regression_test{,.exe}`, `build/bin/.../integration_test{,.exe}`, `build/bin/.../tracker_bridge.{dll,so}`.

`--deploy` additionally assembles a self-contained, redistributable `package/`
directory at the repository root — see [Deploy Packages](#deploy-packages).

---

## CLI Reference

Commands are flat: `ds <command> [flags]`, with `ds docker <action>` the only
subgroup. Arguments after a bare `--` are forwarded to the underlying binary.

| Command | Purpose |
|---|---|
| `ds build` | Configure + build; test targets built by default (`--no-test` to skip) |
| `ds test` | Run a CTest label group: `--suite unit / integration / regression / functional / all` |
| `ds eval` | MOT label benchmark + TrackEval metrics (not a CTest suite) |
| `ds infer` | YOLOX video inference pipeline through `tracker_bridge` |
| `ds run` | Run a built executable, by name or from a picker |
| `ds clean` | Remove `build/` and `package/` |
| `ds deploy {cpp,python}` | Assemble `package/` from the current build |
| `ds status` | What is built, what is missing, which data sets are present |
| `ds doctor` | Verify the toolchain; non-zero exit when a requirement is missing |
| `ds config` | Resolved configuration and the origin of every value |
| `ds paths` | Resolved project directories |
| `ds env` | Environment cmake and the test binaries execute under |
| `ds docker build` / `ds docker run` | Image build / bind-mounted container |

Configuration is layered: built-in defaults → `cli/config.json` → `cli/config.local.json`
→ `DS_*` environment variables (`DS_BUILD_DIR`, `DS_BUILD_TYPE`, `DS_GENERATOR`,
`DS_CMAKE`, `DS_DOCKER_IMAGE`, `DS_TRACKER`, `DS_PROJECT_ROOT`, …). `ds config`
prints which layer each value came from.

### `ds build`

| Flag | Effect |
|---|---|
| `--type, -t <type>` | `release` (default) / `debug` / `relwithdebinfo` / `minsizerel` |
| `--no-test` | Build the library only; test targets are built by default |
| `--clean` | Remove the build directory before configuring |
| `--jobs, -j <n>` | Parallel build jobs (default: CPU count) |
| `-D VAR=VALUE` | Extra `-D` option forwarded to cmake (repeatable) |
| `--deploy {cpp,python}` | After a successful build, assemble a redistributable `package/` for the chosen consumer ([details](#deploy-packages)) |

### `ds test`

Runs tests through CTest labels. Each test target carries one label, assigned in
`tests/*/CMakeLists.txt` on its `gtest_discover_tests` call, so a new test file —
or a whole target — is picked up with no CLI change.

| Suite | Label selection | Contents |
|---|---|---|
| `unit` | `-L '^unit$'` | `unit_test` — deterministic component tests, no external data |
| `integration` | `-L '^integration$'` | `integration_test` — end-to-end coverage through the C API |
| `regression` | `-L '^regression$'` | `regression_test` — MOT scenarios, pass/fail only (metrics: `ds eval`) |
| `functional` | `-L '^(unit\|integration\|regression)$'` | all three; **the default** |
| `all` | no `-L` | every registered test |

| Flag | Effect |
|---|---|
| `--suite, -s <name>` | Label group to run (default `functional`) |
| `--filter, -f <regex>` | `ctest -R` over `Suite.Case` test names |
| `--repeat, -r <n>` | `ctest --repeat until-fail:<n>` — re-run each test until it fails; flaky-test hunting |
| `--jobs, -j <n>` | Run up to N tests concurrently (`ctest --parallel`) |

```bash
ds test                              # functional: unit + integration + regression
ds test --suite unit
ds test --suite unit -f 'RectTest'
ds test --suite integration --repeat 3
ds test --suite all -j 8
```

For GTest-level knobs (shuffle, break-on-failure) run the binary directly instead:

```bash
ds run unit_test -- --gtest_shuffle --gtest_break_on_failure
```

### `ds eval`

The MOT label benchmark. Not a CTest suite: it runs `regression_test` over
det.txt-backed scenarios, parses the per-sequence `[ PERF ]` timings out of its
output, then runs TrackEval and prints a metrics table (HOTA / DetA / AssA /
MOTA / IDF1 / IDSW / FP / FN / Lat). `ds test --suite regression` answers only
pass/fail for the same binary.

| Flag | Effect |
|---|---|
| `--scenario <name>` | Sequence filter (exact match or case-insensitive substring) |
| `--tracker <name>` | `sushitrack` (default), `bytetrack`, or `ocsort` |
| `--compare` | Run all three trackers side-by-side |
| `--eval-only` | Skip the binary; re-score existing `pred/*.txt` files |
| `--trackeval` | Use the official `third_party/TrackEval/scripts/run_mot_challenge.py` instead of the internal runner |
| `--export <dir>` | Export TrackEval JSON/CSV to `<dir>` |
| `--private` | Only `*-YOLOX` sequences (MOT only) |
| `--public` | All sequences except `*-YOLOX` (MOT only) |

```bash
ds eval
ds eval --compare
ds eval --scenario MOT17-04-DPM --compare
ds eval --eval-only --export ./results
ds eval --trackeval --private
```

The internal runner needs the evaluator's own dependencies (numpy, scipy, loguru,
TrackEval) from `environment.yml`; `--trackeval` bypasses it. Scenarios registered
in `ScenarioList.h`: full MOT17 train set (21 sequences across DPM/FRCNN/SDP
detectors plus `-YOLOX` variants for each), MOT20 (`-01/-02/-03/-05` with `-YOLOX`
variants), and DanceTrack sequences (`dancetrack0001`, `0002`, `0006`, `0008`,
`0012`, `0015`, …).

### `ds infer`

Launches the YOLOX-driven video inference pipeline
(`tests/regression/inference/demo.py`), which loads `tracker_bridge.{dll,so}` via
ctypes. Arguments after `--` are forwarded to `demo.py` untouched.

| Flag | Default | Effect |
|---|---|---|
| `--source <path>` | required | Video file path or camera index |
| `--tracker <name>` | `sushitrack` | Tracker selector (`sushitrack` / `bytetrack` / `ocsort`) |
| `-n, --name <name>` | `yolox-s` | YOLOX architecture |
| `-c, --ckpt <path>` | – | Pre-trained YOLOX weights |
| `--device {cpu,gpu}` | `gpu` | Inference device |
| `--conf <float>` | `0.1` | Detection confidence floor |
| `--nms <float>` | `0.45` | NMS IoU threshold |
| `--tsize <H W \| S>` | – | Inference resolution; single value treated as square |
| `--num-classes <int>` | `80` | Class count (set `1` for MOT) |
| `--legacy` | off | Legacy YOLOX normalisation (BGR→RGB, 0–1 scale, ImageNet mean/std) |
| `--stress` | off | Headless max-throughput mode |
| `--no-save` | off | Disable output-video writer |
| `--save_name <name>` | – | Output filename override |
| `--mot-label` | off | Append detections to `output/det.txt` in MOT format |
| `--no-screen` | off | Disable UI display |
| `--reid` | off | Enable OSNet ReID embedding extraction (fills `feat` per detection) |
| `--reid-model <path>` | `osnet_x1_0.onnx` | Path to the ReID ONNX model |

```bash
ds infer -n yolox-x -c third_party/weight/mot17x.pth.tar --source third_party/video/MOT17-04.mp4 --num-classes 1 --legacy

ds infer -n yolox-x -c third_party/weight/mot17x.pth.tar --source tests/regression/evaluator/data/MOT20-01/img1 --num-classes 1 --legacy

ds infer --source 0 --tracker sushitrack --no-save --no-screen --stress
```

`--source` also accepts an image-sequence directory (`<path>/img1`), not just a video
file or camera index. `tests/regression/evaluator/data/` already holds 76 real
tracker benchmark sequences (MOT17, MOT20, DanceTrack) used by the regression
suite — any of their names works, e.g. `tests/regression/evaluator/data/MOT20-01/img1`
or `tests/regression/evaluator/data/dancetrack0001/img1`.

If the YOLOX checkpoint filename contains `mot17` and `--tsize` is omitted/640,
the pipeline overrides `tsize` to `(800, 1440)` to match the MOT17 training
resolution.

### `ds clean`

Removes the build directory and the assembled package directory (`build/` and
`package/` by default; both overridable in the `paths` section of `cli/config.json`).

```bash
ds clean
```

### `ds docker build` / `ds docker run`

`docker build` / `docker run` are issued directly — there are no shell wrappers left. `run` mounts the repo at the configured `docker.workdir` and passes `--gpus all` unless `--no-gpu` is given:
`docker run -it --rm --gpus all -v "<root>:/workspace/sushitrack" -w /workspace/sushitrack sushitrack bash`.

```bash
ds docker build
ds docker run
```

---

## C API

Header: `include/sushitrack_c.h`. Link against the compiled `sushitrack` shared library.

Minimal usage:

```c
#include "sushitrack_c.h"

sushitrack_params_t params;
sushitrack_get_default_params(&params);
params.kalman_type      = 2;
params.enable_tentative  = 1;
params.frame_rate        = 25;

sushitrack_tracker_t hdl;
sushitrack_status_t s = sushitrack_create_ex(&params, &hdl);
if (s != SUSHITRACK_SUCCESS) { /* handle */ }

sushitrack_object_t  dets[MAX_DETS];   /* fill in per frame */
sushitrack_track_t   tracks[MAX_TRACKS];
int n = sushitrack_update(hdl, dets, n_dets, tracks, MAX_TRACKS, dt);

sushitrack_destroy(hdl);
```

`sushitrack_create` returns `NULL` on failure; `sushitrack_create_ex` returns a `sushitrack_status_t` code and writes the handle through `out_hdl`. `sushitrack_update` returns the number of tracks written. Output tracks are sorted by lifecycle priority (`Tracked` < `Tentative` < `Lost` < `Removed`); the buffer is truncated if too small, with a warning logged.

The track-output `feat` pointer references memory owned by the tracker and is only valid until the next `sushitrack_update` or `sushitrack_destroy`.

### Status codes (`sushitrack_status_t`)

| Code | Value |
|---|---|
| `SUSHITRACK_SUCCESS` | 0 |
| `SUSHITRACK_INVALID_ARGUMENT` | -1 |
| `SUSHITRACK_INITIALIZATION_FAILURE` | -2 |
| `SUSHITRACK_OUT_OF_MEMORY` | -3 |
| `SUSHITRACK_RUNTIME_ERROR` | -4 |
| `SUSHITRACK_BUFFER_OVERFLOW` | -5 |

### Logging

```c
void my_logger(sushitrack_LogLevel level, const char* msg) { /* … */ }
sushitrack_set_log_callback(my_logger);  /* pass NULL to unregister */
```

`sushitrack_LogLevel` is `DEBUG=0`, `INFO=1`, `WARN=2`, `ERROR=3`. The callback is stored atomically and safe to set/clear from any thread.

### Track output fields

`sushitrack_track_t` includes `track_id`, `age` (tracklet length in frames), `last_seen_frame`, `feat`/`feat_dim`, `x/y/width/height`, `score`, `lost_time` (seconds or frames depending on `time_aware`), `label`, `state` (TrackState enum integer), `kinematic_state` (`0=Stationary, 1=Cruising, 2=Maneuvering, 3=Coasting`), `is_activated`, `is_confirmed`, `hits`, and `frames_since_last_update`.

---

## Python Binding

`bindings/python/sushitrack.py` is a dependency-free `ctypes` binding to the
public C API. It mirrors `sushitrack_object_t`, `sushitrack_track_t`, and
`sushitrack_params_t` exactly and wraps the entry points in a `SushiTrack` class, so
callers never write `ctypes` themselves. The shared library is located
automatically under `build/` (override with the `SUSHITRACK_LIB` environment
variable or the `lib_path` argument).

```python
from sushitrack import SushiTrack, TrackState

# Any sushitrack_params_t field is a keyword override; the rest keep their defaults.
tracker = SushiTrack(frame_rate=30, enable_tentative=1, iou_type=1)

for frame in stream:
    # detections: (x, y, w, h, score, label) with top-left x, y
    # (optionally a 7th element: a 1-D appearance embedding for ReID)
    dets   = [(x, y, w, h, score, label), ...]
    tracks = tracker.update(dets, dt=1.0 / 30.0)
    for t in tracks:
        if t.state is TrackState.TRACKED:
            print(t.track_id, t.x, t.y, t.width, t.height)

tracker.close()                       # or use `with SushiTrack(...) as tracker:`
```

`update` returns a list of `Track` named tuples (`track_id`, box, `score`,
`label`, `state`, `kinematic_state`, `age`, `lost_time`, `hits`, …) sorted by
lifecycle priority. `Status`, `TrackState`, and `KinematicState` are exposed as
`IntEnum`s; `set_log_callback(fn)` registers a `fn(level, message)` logger.

---

## Deploy Packages

`ds build --deploy {cpp,python}` builds the library and then
assembles a **self-contained, copy-and-run package** under `package/` at the
repository root. The package bundles everything a downstream project needs — the
compiled library, the headers or binding, a default config, and a working
example — so it can be lifted out of this repository and dropped into another
project without a build tree or environment variables. The directory is
regenerated from scratch on every deploy and is git-ignored.

The deploy is **platform-specific**: it copies the artefacts for the OS it runs
on. Run it on Windows to get a Windows package (`.dll` + `.lib`), and on Linux to
get a Linux package (`.so`). There is no cross-compilation — to ship both
platforms, run the deploy once on each.

```bash
ds build --deploy cpp        # C/C++ package
ds build --deploy python     # Python package
ds build --clean --deploy cpp   # clean rebuild, then deploy
```

Deploy reuses the same artefacts a plain `project build` produces; it never needs
a test target. If the shared library (or, on Windows, its import library) is
missing, the deploy fails with an explicit message telling you to build first.

### `--deploy cpp` layout

Targets a C or C++ consumer through the **C ABI** (`sushitrack_c.h`). The C ABI is
deliberately dependency-free — it does **not** pull in Eigen or any C++ header —
so only two headers are shipped.

```
package/
├── include/
│   ├── sushitrack_c.h                 # public C API
│   └── SushiTrack/types.hpp   # status codes, log levels (only C dependency of the API)
├── lib/
│   └── sushitrack.lib                # Windows import library (Linux: libsushitrack.so)
├── bin/
│   └── sushitrack.dll                # Windows runtime library (Linux: omitted — the .so in lib/ is both)
├── sushitrack.json                   # default runtime config; edit to retune without recompiling
├── example/
│   ├── example.c                    # minimal create → update loop → destroy
│   └── CMakeLists.txt               # links lib/, copies the DLL beside the exe (Windows), sets RPATH (Linux)
└── README.md
```

Build and run the bundled example:

```bash
cd package/example
cmake -B build
cmake --build build
./build/sushitrack_example            # Windows: the DLL is auto-copied next to the exe by CMake
```

To integrate into your own project: add `package/include` to the include path,
include `sushitrack_c.h`, link against `package/lib/sushitrack.lib` (Windows) or
`package/lib/libsushitrack.so` (Linux), and ensure the runtime library is
resolvable at load time (DLL on `PATH`/beside the exe on Windows; RPATH or
`LD_LIBRARY_PATH` on Linux). Create the tracker with
`sushitrack_createFromConfig("sushitrack.json")` to pick up the bundled config, and
edit that JSON to retune the algorithm without recompiling.

### `--deploy python` layout

Targets a Python consumer. The native library is bundled **inside** the package
directory, beside the binding module, so the package is portable with no build
tree and no `SUSHITRACK_LIB` variable.

```
package/
├── sushitrack/
│   ├── __init__.py                  # re-exports the public API (from .sushitrack import *)
│   ├── sushitrack.py                 # the ctypes binding (copied from bindings/python/)
│   └── sushitrack.dll                # bundled native library (Linux: libsushitrack.so)
├── sushitrack.json                   # default runtime config
├── example.py                       # minimal update loop using the bundled package
└── README.md
```

Run the bundled example:

```bash
cd package
python example.py
```

To integrate into your own project, copy the `sushitrack/` directory into your
source tree (or add `package/` to `PYTHONPATH`) and use it as a normal package:

```python
from sushitrack import SushiTrack, TrackState
tracker = SushiTrack(frame_rate=30, kalman_type=2)
tracks  = tracker.update([(x, y, w, h, score, label)], dt=1/30)
```

### How discovery works

The Python binding's `find_library` resolves the shared library in this order:
an explicit `lib_path` argument, the `SUSHITRACK_LIB` environment variable, **the
directory next to the binding module** (this is what makes the deployed package
self-contained), then the standard `build/` output locations. A binding used
in-place from this repository finds the library under `build/`; the same binding
copied into a deploy package finds the bundled library sitting beside it. No
configuration is required in either case.

---

## Data Layout

The regression harness reads from `tests/regression/evaluator/data/`:

```
tests/regression/evaluator/data/
├── MOT17-02-DPM/
│   ├── det/det.txt                   # MOT-format detections
│   ├── gt/gt.txt                     # MOT-format ground truth
│   └── seqinfo.ini                   # at minimum: seqLength, frameRate
├── MOT17-02-FRCNN/  …
├── MOT20-01/         …
├── MOT20-01-YOLOX/   …
├── dancetrack0001/img1/00000001.jpg  …
└── …
```

MOT CSV format:

```
<frame>, <id>, <x>, <y>, <w>, <h>, <score>, <class>, …
```

`seqinfo.ini` is parsed for `frameRate` (used for `dt = 1/frameRate` when `time_aware=1`). If absent, 30 fps is assumed.

Prediction files are written to `<sequence>/pred/<tracker>/<sequence>.txt` after each regression run. `cli/misc/clean_preds.sh` wipes every `pred/` subdirectory.

### Tracker config files

Each tracker owns a dedicated JSON file:

| File | Tracker | Loaded by |
|---|---|---|
| `sushitrack.json` (repository root) | SushiTrack | the **library itself** |
| `tests/bytetrack.json` | ByteTrack | regression harness |
| `tests/ocsort.json` | OCSORT | regression harness |

`sushitrack.json` is bound to the library, not the test harness: the C API parses it directly (`source/ConfigLoader.cpp`), so a compiled `sushitrack.dll`/`.so` is retuned by editing the file with **no recompilation**. Resolution order: the `$SUSHITRACK_CONFIG` environment variable, then an upward search for `sushitrack.json` from the working directory (so a binary launched from `build/bin` still finds the file at the project root). Any missing key falls back to the compiled default from `sushitrack_get_default_params`.

The schema is layered `basic` / `advanced`. The outer `"sushitrack"` wrapper is optional — a file whose root is already the `basic`/`advanced` object is also accepted:

```json
{
  "sushitrack": {
    "basic": {
      "track_thresh": 0.2, "high_thresh": 0.5, "match_thresh": 0.7,
      "frame_rate": 25, "track_buffer": 30,
      "enable_tentative": 1, "enable_reid": 0,
      "enable_mahalanobis": 0
    },
    "advanced": {
      "association":  { "primary_assoc_mode": 1, "fuse_score": 1, "iou_match_thresh": 0.5 },
      "coordinate_system": { "iou_type": 1, "enable_xyah": 0, "iou_plus_one": 1, "kalman_type": 0 },
    }
  }
}
```

C API entry points (`include/sushitrack_c.h`):

- `sushitrack_LoadParamsFromJson(path, &params)` — overlay a file onto a seeded `sushitrack_params_t` (`path = NULL` uses the resolution order above). A missing file is non-fatal.
- `sushitrack_createFromConfig(path)` — seed defaults, overlay the file, and create in one call.
- `sushitrack_create(NULL)` — shorthand for `sushitrack_createFromConfig(NULL)`. Passing an explicit `sushitrack_params_t*` bypasses file loading entirely (programmatic control).

The Python binding mirrors this: `SushiTrack(config="auto")` resolves `sushitrack.json` through the library, `config="<path>"` loads an explicit file, and keyword overrides still win over the file.

---

## Configuration Parameters

All knobs are exposed both on `sushitrack_params_t` (C API, `include/sushitrack_c.h`) and `SushiTrack::TrackerConfig` (C++, `include/SushiTrack/TrackerConfig.h`). Defaults below are from `sushitrack_get_default_params`.

### Detection thresholds

| Key | Default | Description |
|---|---|---|
| `track_thresh` | 0.2 | Splits detections into high (`score ≥ track_thresh`) and low pools |
| `high_thresh` | 0.5 | Minimum score to initialise a new track |

### Association cost limits

| Key | Default | Description |
|---|---|---|
| `match_thresh` | 0.7 | Maximum cost for the primary stage |
| `iou_match_thresh` | 0.5 | Maximum cost for the secondary stage |
| `unconfirmed_iou_thresh` | 0.7 | Maximum cost for unconfirmed tracks (`enable_tentative=0`) |
| `duplicate_iou_thresh` | 0.15 | Cost cutoff for duplicate suppression (IoU > 0.85) |

### Track lifecycle

| Key | Default | Description |
|---|---|---|
| `frame_rate` | 25 | Target frequency; drives noise scaling and wall-clock conversion |
| `track_buffer` | 30 | Frames a lost track is kept before deletion |
| `enable_tentative` | 0 | Enables tentative state |
| `tentative_match_thresh` | 0.3 | Cost cutoff for tentative association |
| `tentative_confirm_hits` | 3 | Consecutive matches required to confirm |
| `tentative_max_age_seconds` | 0.08 | Wall-clock tentative expiry budget, used only when `time_aware=1` (≈ 2 frames at 25 fps) |
| `tentative_max_miss_frames` | 2 | Frame-counted tentative expiry budget, used when `time_aware=0` |
| `tentative_init_suppress_iou` | 0.7 | IoU above which a new-track detection overlapping a confirmed track is suppressed |
| `tentative_fast_thresh` | 0.85 | Above this score, `tentative_fast_hits` hits are enough |
| `tentative_fast_hits` | 1 | Hit count for fast-confirmation regime |
| `tentative_slow_thresh` | 0.40 | Below this score, extra hits are required |
| `tentative_slow_hits_offset` | 1 | Extra hits added in `[slow_thresh, high_thresh)` |
| `tentative_vslow_hits_offset` | 2 | Extra hits added below `slow_thresh` |

### Motion model

| Key | Default | Description |
|---|---|---|
| `enable_xyah` | 0 | 0 = TLWH state, 1 = centroid+aspect+height state |
| `iou_plus_one` | 1 | Inclusive bounding-box dimensions |

`kalman_type` is documented under [Motion estimator](#motion-estimator) below.

`KalmanFilter` baseline constants (compile-time, see `include/SushiTrack/KalmanFilter.h`): `kStdWeightPosition=1/20`, `kStdWeightVelocity=1/160`, `kInitPositionMult=2.0`, `kInitVelocityMult=10.0`. These replace earlier configurable hyper-parameters and match the ByteTrack reference tuning.

### Association strategy

| Key | Default | Description |
|---|---|---|
| `primary_assoc_mode` | 1 | 0 = age-cascade legacy, 1 = global LAPJV |
| `fuse_score` | 1 | Score-fusion decorator on the primary cost matrix |
| `iou_type` | 1 | 0=IoU, 1=DIoU, 2=GIoU, 3=SIoU |
| `enable_mahalanobis` | 0 | Apply Mahalanobis gating |
| `gating_thresh` | 9.4877 | Chi-squared threshold (95 % CI, 4 DoF) |
| `motion_cost_weight` | 0.15 | Weight of squared Mahalanobis distance in gated IoU |
| `max_area_ratio` | 50.0 | Suppress pairs with extreme area disparity |

### Observation-centric momentum (OCM / ORU)

| Key | Default | Description |
|---|---|---|
| `enable_ocm` | 0 | Enable the observation-centric momentum re-ranking term on the primary stage |
| `enable_oru` | 0 | Enable observation-centric re-update (rollback and replay) on lost-track recovery |
| `ocm_vdc_weight` | 0.1 | Bound on the momentum term subtracted from each in-gate primary cost cell |
| `ocm_delta_t` | 3 | Observed-frame window over which the direction of travel is measured |
| `ocm_min_velocity` | 1.0 | Minimum observed speed (px per observed frame) below which momentum is suppressed |
| `ocm_max_age` | 1 | Maximum frames since the last observation for which momentum is applied |
| `ocm_maneuver_weight_scale` | 0.15 | Fraction of `ocm_vdc_weight` applied to Maneuvering tracks (0 restricts momentum to Cruising) |

### ReID

| Key | Default | Description |
|---|---|---|
| `enable_reid` | 0 | Enable appearance matching |
| `feature_momentum` | 0.9 | EMA weight for the smoothed feature |
| `max_feature_history` | 100 | Per-tracklet feature-vector gallery size |
| `reid_high_thresh` | 0.3 | Direct-association similarity threshold (high stage) |
| `reid_high_iou_weight` | 0.3 | IoU fallback weight (high stage) |
| `reid_low_thresh` | 0.5 | Direct-association similarity threshold (low stage) |
| `reid_low_iou_weight` | 0.2 | IoU fallback weight (low stage) |
| `reid_recovery_thresh` | 0.4 | Similarity threshold for PureReID lost recovery |
| `reid_direct_assignment_weight` | 0.2 | Scaling for high-confidence direct visual matches |

### Motion estimator

| Key | Default | Description |
|---|---|---|
| `kalman_type` | 0 | Motion estimator: `0` linear Kalman, `1` NSA, `2` IMM multi-model. With `2` each track reports a `kinematic_state` and continuous `maneuver_probability`; the IMM noise/transition parameters are built-in (see [architecture/OVERVIEW.md](architecture/OVERVIEW.md)). |

### Timing

| Key | Default | Description |
|---|---|---|
| `time_aware` | 0 | 0 = treat dt as 1 step; 1 = treat dt as real seconds (capped at 5 KF-steps) |

---

## Test Suites

### Unit tests (`unit_test`)

GTest-based, deterministic, no external data. Targets compiled when `-DBUILD_UNIT_TEST=ON`. Cover:

- `TestKalmanFilter` — initiate/predict/update, Mahalanobis distance, gate flags, LLT cache
- `TestTracklet` — lifecycle transitions, feature EMA
- `TestRect` — TLWH/TLBR/XYWH conversions
- `TestReID` — cosine similarity, max-history lookup
- `TestCostMatrix` — resize semantics, sentinel preservation
- `TestTimingPolicies` — frame vs wall-clock conversion and expiry symmetry
- `TestDistanceStrategies` — each cost calculator (IoU/DIoU/GIoU/SIoU/Cosine/ReIDFirst/PureReID/Fused/GatedIoU/FuseScoreDecorator)
- `TestAssignmentSolvers` — LAPJV correctness on dense and sparse matrices
- `TestTrackerTentative` — adaptive confirmation logic
- `TestTrackerBuffers` — track_buffer expiry under both timing modes
- `TestLogger` — callback dispatch, va_list path
- `TestSushiTrackC` / `TestCAPIErrors` — C-API surface and error codes
- `TestTracker` / `TestIntegration` — end-to-end flow
- `TestBoundaryConditions` — empty inputs, oversized output buffer, null args
- `TestConfiguration` — config-driven parameter parsing

Direct invocation:

```bash
ds run unit_test
ds run unit_test -- --gtest_filter="TrackletTest.*"
ds test --suite unit                    # ctest --test-dir build -L '^unit$'
```

### Regression tests (`regression_test`)

Parameterised GTest binary that iterates over `Scenario × tracker_factory`. For each pair it:

1. Reads `det.txt` frames via `MOTRunner`.
2. Drives the tracker through `ITrackerAdapter` (`SushiTrackAdapter`, `ByteTrackAdapter`, `OCSORTAdapter`).
3. Writes predictions to `<sequence>/pred/<tracker>/<sequence>.txt`.
4. Emits a `[ PERF <tracker> ] <sequence>: <us/frame> us/frame` line read back by the CLI (`cli/sushitrack_cli/services/tests.py`).

Config resolution: SushiTrack loads `sushitrack.json` through the library (`$SUSHITRACK_CONFIG`, then an upward search); ByteTrack and OCSORT read `tests/bytetrack.json` and `tests/ocsort.json`.

Direct invocation:

```bash
./build/bin/Release/regression_test
./build/bin/Release/regression_test --gtest_filter="*MOT17_04_DPM_sushitrack*"
SUSHITRACK_CONFIG=./sushitrack.json ./build/bin/Release/regression_test
```

### Integration tests (`integration_test`)

GTest binary that drives the tracker end-to-end exclusively through the C API, as a downstream FFI consumer would. Targets compiled when `-DBUILD_INTEGRATION_TEST=ON`. Cover:

- `TestByteAssociation` — two-stage BYTE association: low-confidence sustain, new-track seeding rules
- `TestOcclusionRecovery` — lost-track buffering, within/beyond-buffer recovery, no id transfer
- `TestMultiObject` — identity separation, counter-moving pass, output capacity/priority contract
- `TestConfigVariations` — IoU variant sweep, XYAH coordinates, NSA Kalman
- `TestAdvancedPath` — ReID + Mahalanobis gating
- `TestFullStack` — full advanced stack enabled at once
- `TestKinematicState` — IMM-derived regime reported end-to-end (Stationary/Cruising/Maneuvering/Coasting)
- `TestObservationMomentum` — observation-centric momentum re-ranking and ORU recovery
- `TestDuplicateSuppression` — cross-pool duplicate removal by track lifetime
- `TestTimeAware` — wall-clock (`time_aware`) stepping and expiry

Direct invocation:

```bash
ds run integration_test
ds test --suite integration             # ctest --test-dir build -L '^integration$'
```

---

## Evaluation Pipeline

`tests/regression/evaluator/`:

- **`trackeval_runner.py`** — wraps the TrackEval Python module. Builds per-benchmark sequence maps, configures `MotChallenge2DBox` dataset, runs `HOTA`, `CLEAR`, `Identity` metrics.
- **`report_generator.py`** — `print_trackeval_table(results, compare)` formats the per-tracker × per-sequence table; `export_trackeval_results(results, dir)` writes JSON/CSV.
- **`run_eval.py`** — standalone CLI for `TrackEvalRunner` (independent of the compiled binary).

`ds eval` automatically chains regression_test → TrackEval; pass `--eval-only` to skip the C++ binary and re-evaluate cached predictions, or `--trackeval` to invoke the upstream `third_party/TrackEval/scripts/run_mot_challenge.py` script.

---

## Inference Pipeline

`tests/regression/inference/`:

- **`tracker_bridge.py`** — `TrackerBridge` dispatches per tracker. `SushiTrack` is driven through the public-API binding ([`bindings/python/sushitrack.py`](#python-binding)), so the C API is consumed in one place and the binding is exercised on every inference frame. The reference trackers (`ByteTrack`, `OCSORT`) have no public C API and stay on the `TrackerRegistry` shim (`create_tracker` / `tracker_process_c` / `delete_tracker` in `tracker_bridge.{dll,so}`, built from `tests/regression/TrackerRegistry.cpp` when `-DBUILD_REGRESSION_TEST=ON`). Both paths return the same `{track_id, bbox, score}` shape.
- **`pipeline.py`** — defines `IDetector`, `YOLOXDetector` (loads weights via `yolox.exp.get_exp`, preprocesses with optional legacy normalisation, runs inference, postprocesses with NMS), and `VideoPipeline` (frame loop, detection→tracker hand-off, drawing, optional MOT-format detection logging, stress/no-screen modes).
- **`demo.py`** — argparse front-end exposing the full YOLOX + tracker flag set. Invoked through `ds infer`.

Example end-to-end run:

```bash
ds build
ds infer \
    -n yolox-x -c third_party/weight/mot17x.pth.tar \
    --source third_party/video/MOT17-04.mp4 \
    --tracker SushiTrack --num-classes 1 --legacy
```

The pipeline auto-detects MOT17 checkpoints by filename and sets `tsize=(800,1440)` accordingly.

---

## Ultralytics Integration

SushiTrack can replace ByteTrack/BoT-SORT as the tracker behind an Ultralytics
YOLO model (`model.track(...)`) so that any Ultralytics detector — YOLOv8/v9/v10/11,
RT-DETR, or a custom-trained checkpoint — feeds its detections into SushiTrack
per frame. The integration is **non-invasive**: you do not fork or modify the
`ultralytics` package. You build the SushiTrack shared library, wrap its C API
with a thin `ctypes` adapter that satisfies the Ultralytics tracker contract,
and register that adapter through Ultralytics' public prediction callbacks.

> This section documents the porting recipe only. No adapter code is shipped in
> this repository — the snippets below are the reference implementation you copy
> into your Ultralytics project.

### How Ultralytics tracking works

`model.track()` installs two callbacks (`ultralytics/trackers/track.py`):

- **`on_predict_start`** — instantiates one tracker object per batch stream from
  `TRACKER_MAP[cfg.tracker_type]` and stores them on `predictor.trackers`.
- **`on_predict_postprocess_end`** — for each frame, takes the detection
  `Boxes` (`predictor.results[i].boxes`), calls `tracker.update(det, img)`, and
  expects back an `(N, 7+)` array whose **last column is the source detection
  index**. Ultralytics slices the results by that index and overwrites the boxes
  with columns `[:-1]`, laid out as `[x1, y1, x2, y2, track_id, conf, cls]`.

Any object exposing a compatible `update(results, img=None)` method can take the
place of `BYTETracker`. That is the single seam SushiTrack plugs into.

### Step 1 — Get the library and binding into your project

You have two options. For an Ultralytics project living **outside** this
repository, the deploy package is self-contained: it needs no build tree or
environment variables on the consumer side.

**Option A — deploy package (recommended for external projects).** Produce a
portable Python package and copy it next to your Ultralytics code:

```bash
ds build --deploy python      # builds + assembles package/
# then, in your Ultralytics project:
cp -r /path/to/sushitrack/package/sushitrack   ./sushitrack      # bundles the .py + native lib
```

The bundled `sushitrack/` directory carries the binding and the native library
together, so importing `from sushitrack import SushiTrack` works with no build tree
and no `SUSHITRACK_LIB` variable on the consumer side. See
[Deploy Packages](#deploy-packages).

**Option B — in-repo binding.** When you work inside this repository, build the
library and import the binding directly:

```bash
ds build           # produces build/bin/<cfg>/sushitrack.{dll,so}
# bindings/python/sushitrack.py then finds the library under build/ automatically
```

Either way the adapter uses the shipped Python binding
([`bindings/python/sushitrack.py`](#python-binding)), which wraps the **public C
API** (`include/sushitrack_c.h`) — not the regression `tracker_bridge`, whose
reduced struct omits the state/feature/kinematic fields.

### Step 2 — Implement the Ultralytics tracker contract

The adapter converts Ultralytics detections into SushiTrack detections, calls
`SushiTrack.update`, and emits the `(N, 7)` array Ultralytics expects, plus the
detection-index column.

Two conversions matter:

- **Coordinates.** Ultralytics `Boxes.xywh` is **centre-based** (`cx, cy, w, h`);
  SushiTrack consumes **top-left** `x, y, w, h`. Use `Boxes.xyxy` and derive
  `x = x1`, `y = y1`, `w = x2 - x1`, `h = y2 - y1` to avoid sign mistakes.
- **Detection index.** The C API returns a filtered/Kalman-smoothed box and does
  **not** echo which input detection produced each track. Ultralytics needs that
  index to slice masks/probs. Recover it by greedy IoU-matching each output
  track box against the input detections of the current frame.

```python
import numpy as np
from sushitrack import SushiTrack, TrackState     # bindings/python/sushitrack.py

class Tracker:
    def __init__(self, frame_rate=30, **overrides):
        # overrides are sushitrack_params_t fields, e.g. enable_tentative=1, iou_type=1
        self.ds = SushiTrack(frame_rate=frame_rate, **overrides)
        self.dt = 1.0 / float(frame_rate)

    def update(self, results, img=None):
        xyxy = results.xyxy.cpu().numpy() if hasattr(results.xyxy, "cpu") else np.asarray(results.xyxy)
        conf = np.asarray(results.conf)
        cls  = np.asarray(results.cls)

        dets = [(x1, y1, x2 - x1, y2 - y1, float(c), int(k))
                for (x1, y1, x2, y2), c, k in zip(xyxy, conf, cls)]
        tracks = self.ds.update(dets, dt=self.dt)

        rows = []
        for t in tracks:
            if t.state not in (TrackState.TRACKED, TrackState.TENTATIVE):
                continue
            box = np.array([t.x, t.y, t.x + t.width, t.y + t.height])
            j = self._best_iou_idx(box, xyxy)            # recover detection index
            if j < 0:
                continue
            rows.append([box[0], box[1], box[2], box[3], t.track_id, t.score, t.label, j])
        return np.asarray(rows, dtype=np.float32) if rows else np.empty((0, 8), np.float32)

    @staticmethod
    def _best_iou_idx(box, dets, thr=0.3):
        if len(dets) == 0:
            return -1
        xx1 = np.maximum(box[0], dets[:, 0]); yy1 = np.maximum(box[1], dets[:, 1])
        xx2 = np.minimum(box[2], dets[:, 2]); yy2 = np.minimum(box[3], dets[:, 3])
        w = np.clip(xx2 - xx1, 0, None); h = np.clip(yy2 - yy1, 0, None)
        inter = w * h
        area_b = (box[2]-box[0]) * (box[3]-box[1])
        area_d = (dets[:,2]-dets[:,0]) * (dets[:,3]-dets[:,1])
        iou = inter / (area_b + area_d - inter + 1e-9)
        j = int(iou.argmax())
        return j if iou[j] >= thr else -1
```

### Step 3 — Register the adapter with `model.track()`

No package fork required — install the adapter through the same callbacks
Ultralytics itself uses. The binding finds the shared library automatically:
beside the module when you used the deploy package (Option A), or under `build/`
when you import in-repo (Option B); `SUSHITRACK_LIB` overrides both:

```python
from ultralytics import YOLO
from sushitrack_adapter import Tracker

def on_predict_start(predictor, persist=False):
    bs = predictor.dataset.bs
    predictor.trackers = [
        Tracker(frame_rate=30, enable_tentative=1, iou_type=1)
        for _ in range(bs)
    ]

model = YOLO("yolo11n.pt")
model.add_callback("on_predict_start", on_predict_start)

for result in model.track(source="video.mp4", stream=True, persist=True):
    boxes = result.boxes
    if boxes.id is not None:
        ids   = boxes.id.int().tolist()
        xyxy  = boxes.xyxy.tolist()
        # ... your downstream logic ...
```

Because Ultralytics' built-in `on_predict_postprocess_end` already calls
`predictor.trackers[i].update(...)` and interprets the returned array, supplying
your own `on_predict_start` is enough to swap the tracker. (Alternatively, fork
the package and add `"sushitrack": Tracker` to `TRACKER_MAP` with a
matching `sushitrack.yaml`; the callback route above avoids touching vendored
code and is the recommended path.)

### Notes and caveats

- **Top-left vs. centre coordinates** are the most common integration bug —
  always derive SushiTrack boxes from `xyxy`.
- **Detection-index recovery** via IoU is robust when the Kalman correction is
  small; for fast motion raise `max_tracks` and lower the IoU floor rather than
  asserting a 1:1 mapping. Tracks with no input overlap (pure Kalman coasting
  during a missed frame) are intentionally dropped from the Ultralytics output
  for that frame, exactly as the built-in trackers do.
- **ReID is off in this recipe.** SushiTrack's appearance path expects an
  external embedding per detection; Ultralytics detection models do not produce
  one. To enable `enable_reid=1`, run an embedding extractor (e.g. the OSNet path
  used by the YOLOX inference pipeline) and append the 1-D embedding as a 7th
  element of each detection tuple passed to `update`. Without it the tracker uses
  the geometry fallback.
- **Class handling.** SushiTrack associates across all detections regardless of
  `label`; pass `classes=[...]` to `model.track()` if you need per-class
  filtering upstream, or partition detections before `update()`.
- **Stateful streaming.** Create the tracker once per stream and reuse the
  handle across frames (as above). Re-creating it per frame resets all IDs.

---

## Internal Tools

| File | Purpose |
|---|---|
| `cli/ds.py` | CLI launcher; sets up `sys.path` and calls `sushitrack_cli.cli.entry` |
| `cli/sushitrack_cli/cli.py` | Flat command declarations delegating to the service layer |
| `cli/sushitrack_cli/app.py` | Declarative argparse layer (command registry, grouped help, `--` passthrough) |
| `cli/sushitrack_cli/config.py` | Layered JSON config, `DS_*` overrides, project-root and path resolution |
| `cli/sushitrack_cli/env.py` | vcvars discovery and build-environment snapshotting on Windows |
| `cli/sushitrack_cli/services/` | build, tests (ctest), label (`ds eval`), infer, evaluate (TrackEval), deploy, docker, diag, discovery, runner |
| `cli/config.json` | Tracked CLI defaults (`cli`, `paths`, `build`, `docker`, `test` sections) |
| `cli/install.sh` | Bash/Zsh alias and PATH setup; prints permanent-install lines |
| `cli/install.ps1` | PowerShell session function for `ds` |
| `cli/ds.sh` / `cli/ds.bat` | Thin wrappers — `python3 ds.py "$@"` / `python ds.py %*` |
| `cli/Dockerfile.cli` | Bare `python:3-slim` image containing only `cli/`, used to verify the CLI needs no third-party packages |
| `cli/misc/clean_preds.sh` / `cli/misc/clean_preds.bat` | Recursively removes `pred/` directories under the evaluator data root |
| `cli/misc/construct_trackeval_data.ps1` | Creates the official TrackEval directory tree by symlinking GT (MOT17-SDP as representative for each sequence) and copying per-tracker predictions for `sushitrack`/`bytetrack`/`ocsort` |

`Dockerfile`: CUDA 12.1 devel base, installs `build-essential`, `cmake`, `git`, `python3`, `python3-pip`, `libgtest-dev`, `libgl1-mesa-glx`, `libglib2.0-0`, then pip-installs the pinned wheel set, copies the project, and symlinks `ds` into `/usr/local/bin`.

---

## Benchmark Results

Evaluated on the MOT17 and MOT20 train sets and DanceTrack, comparing SushiTrack
against the bundled ByteTrack and OC-SORT references at matched parameters.
Metrics are from TrackEval. The per-tracker aggregates below summarise the
internal run recorded in [reference/BENCHMARKS.md](reference/BENCHMARKS.md),
which also holds the per-sequence breakdown. SushiTrack ran on the default
linear-Kalman path (`kalman_type=0`) with the tentative lifecycle, OCM, and ORU
enabled and appearance ReID off; the full config is in that file. Latency is the
mean per-frame `update()` time, single-threaded, excluding I/O. BENCHMARKS.md also
records the `kalman_type=2` IMM ablation: 45.8 % overall HOTA against 45.7 % here,
at roughly four times the per-frame latency.

**MOT17**

| Tracker | HOTA | DetA | AssA | MOTA | IDF1 | IDSW | FP | FN | Lat (µs/frame) |
|---|---|---|---|---|---|---|---|---|---|
| SushiTrack | 47.2 % | 48.2 % | 47.5 % | 55.0 % | 55.5 % | 2849 | 7092 | 185931 | 37 |
| ByteTrack | 44.7 % | 48.5 % | 42.4 % | 55.1 % | 52.5 % | 4773 | 12806 | 177427 | 87 |
| OC-SORT | 44.1 % | 46.4 % | 43.2 % | 53.0 % | 52.2 % | 2836 | 9172 | 191275 | 167 |

**MOT20**

| Tracker | HOTA | DetA | AssA | MOTA | IDF1 | IDSW | FP | FN | Lat (µs/frame) |
|---|---|---|---|---|---|---|---|---|---|
| SushiTrack | 44.9 % | 43.9 % | 50.9 % | 50.6 % | 50.1 % | 2904 | 13740 | 1185170 | 318 |
| ByteTrack | 41.9 % | 42.7 % | 46.9 % | 49.9 % | 46.3 % | 3680 | 26843 | 1184486 | 506 |
| OC-SORT | 38.7 % | 39.0 % | 41.5 % | 45.1 % | 44.0 % | 2972 | 29224 | 1254585 | 1181 |

**DanceTrack**

| Tracker | HOTA | DetA | AssA | MOTA | IDF1 | IDSW | FP | FN | Lat (µs/frame) |
|---|---|---|---|---|---|---|---|---|---|
| SushiTrack | 44.9 % | 72.0 % | 29.3 % | 85.1 % | 46.5 % | 3568 | 7215 | 42428 | 18 |
| ByteTrack | 37.9 % | 65.2 % | 23.3 % | 82.0 % | 42.8 % | 3863 | 15913 | 43195 | 34 |
| OC-SORT | 45.6 % | 69.7 % | 31.1 % | 80.2 % | 48.5 % | 2949 | 15976 | 50485 | 71 |

**Overall**

| Tracker | HOTA | DetA | AssA | MOTA | IDF1 | IDSW | FP | FN | Lat (µs/frame) |
|---|---|---|---|---|---|---|---|---|---|
| SushiTrack | 45.7 % | 60.3 % | 38.3 % | 70.4 % | 50.2 % | 9321 | 28047 | 1413529 | 57 |
| ByteTrack | 40.8 % | 56.7 % | 32.8 % | 68.7 % | 46.8 % | 12316 | 55562 | 1405108 | 103 |
| OC-SORT | 44.3 % | 57.9 % | 36.7 % | 66.5 % | 49.4 % | 8757 | 54372 | 1496345 | 223 |

---

## License

Copyright (c) 2026-present Mustafa Garip & Sushi Systems.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
