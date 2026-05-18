# SushiTrack

A C++17 multi-object tracking library. SushiTrack extends the ByteTrack two-stage association pipeline with an interface-driven, dependency-injected architecture, four IoU variants, multiple cost calculators, visual ReID fusion, Mahalanobis gating, a Noise-Scale-Adaptive Kalman filter, a per-track kinematic finite state machine, a scene-level interaction graph for occlusion-aware lost-track recovery, real-time (`time_aware`) operation, tentative track validation, and a stable C API. A Python CLI (`sushitrack`/`ds`), an MOT-Challenge-style regression harness, a GTest unit suite, and a YOLOX-driven video inference pipeline are bundled.

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
  - [project test label](#project-test-label)
  - [project test inference](#project-test-inference)
  - [project clean](#project-clean)
  - [docker build / docker run](#docker-build--docker-run)
- [C API](#c-api)
- [Data Layout](#data-layout)
- [Configuration Parameters](#configuration-parameters)
- [Test Suites](#test-suites)
- [Evaluation Pipeline](#evaluation-pipeline)
- [Inference Pipeline](#inference-pipeline)
- [Internal Tools](#internal-tools)
- [Benchmark Results](#benchmark-results)
- [License](#license)

---

## Architecture

The runtime is organised around interfaces that the tracker depends on. Concrete implementations are wired up by the C-API entry point (`SushiTrack_CreateEx` in `src/SushiTrackC.cpp`) based on the `SushiTrack_Params` flags. The core orchestrator `SushiTrackTracker` (see `include/SushiTrack/SushiTrackTracker.h`) consumes only the abstractions.

| Interface | Purpose | Concrete implementations |
|---|---|---|
| `IStatePredictor` | State propagation/update | `KalmanFilter`, `NSAKalmanFilter` |
| `IGateValidator` | Mahalanobis gating | `KalmanFilter`, `NSAKalmanFilter`, `NullGateValidator` |
| `IKalmanFilter` | Unified predictor + validator | `KalmanFilter`, `NSAKalmanFilter` |
| `IKalmanFactory` | Per-tracklet predictor factory | `GenericKalmanFactory` (registry-driven) |
| `ICostCalculator` | Pairwise association cost | `IoUCostCalculator`, `DIoUCostCalculator`, `GIoUCostCalculator`, `SIoUCostCalculator`, `CosineCostCalculator`, `ReIDFirstCostCalculator`, `PureReIDCostCalculator`, `FusedCostCalculator`, `GatedIoUCostCalculator`, `FuseScoreDecorator` |
| `IAssignmentSolver` | Bipartite assignment | `LAPJVSolver` (Jonker–Volgenant with active-row/column reduction) |
| `ITimingPolicy` | dt normalisation, expiry | `FrameTimingPolicy`, `WallClockTimingPolicy` |
| `IInteractionGraph` | Pairwise occlusion graph | `BottomYInteractionGraph` (bottom-Y depth ordering + kinematic tiebreaker, DIoU activation) |

`KalmanFilterRegistry` is a singleton plug-in registry. New Kalman variants register themselves through `KalmanFilterRegistrar<T>` and become selectable by `kalman_type`.

Track lifecycle is governed by `TrackState` (`New`, `Tracked`, `Lost`, `Removed`, `Tentative`). When `enable_fsm` is set, each track also carries a `TrackFSM` that classifies its motion as `Stationary`, `Cruising`, `Maneuvering`, or `Coasting`, with hysteresis (`kStableFrames=2`) and a warmup of `kWarmupFrames=3` frames. The FSM rescales process-noise multipliers on the predictor.

---

## Features

**Two-stage ByteTrack association** — high-confidence detections (`score >= track_thresh`) match first; remaining tracks then attempt to absorb low-confidence detections.

**Tentative validation** — when `enable_tentative=1`, new tracks must accumulate `tentative_confirm_hits` consecutive matches before promotion. Confirmation hits are adaptive: `tentative_fast_thresh` (default 0.85) allows single-hit confirmation; `tentative_slow_thresh` (default 0.40) adds `tentative_slow_hits_offset` extra hits; below the slow threshold adds `tentative_vslow_hits_offset` hits. A separate `tentative_match_thresh` and `tentative_max_age_seconds` govern tentative-pass cost and expiry.

**IoU variants** — `iou_type` selects `IoUCostCalculator` (0), `DIoUCostCalculator` (1, default), `GIoUCostCalculator` (2), or `SIoUCostCalculator` (3) for the primary/secondary stages. Tentative and duplicate stages are pinned to plain IoU so their thresholds keep their calibration.

**Score fusion** — `FuseScoreDecorator` rescales the primary cost matrix by detection score: `fused = 1 − (1 − inner) * score`. Sentinel cells are preserved. Applied only to the primary stage. Toggled with `fuse_score`.

**Visual ReID** — `enable_reid=1` activates one of three paths depending on `enable_mahalanobis`:
- ReID-first (default): `ReIDFirstCostCalculator` for the primary/secondary passes with an IoU fallback, plus `PureReIDCostCalculator` for lost recovery.
- Fused: `FusedCostCalculator` combines IoU, cosine appearance and Mahalanobis gating (when `enable_mahalanobis=1`).
- Lost recovery: `PureReIDCostCalculator` (`reid_recovery_thresh`), active in all ReID paths.

Visual features are L2-normalised and exponentially smoothed (`feature_momentum`, default 0.9). A per-tracklet feature history holds up to `max_feature_history` (default 100) vectors and `getMaxFeatureSimilarity` returns the max cosine similarity against the history.

**Mahalanobis gating** — `enable_mahalanobis=1` filters association candidates via a chi-squared test on the projected state distribution. `gating_thresh` defaults to 9.4877 (95 % CI for 4 DoF). `GatedIoUCostCalculator` combines IoU with the gate; `motion_cost_weight` blends the squared distance into the cost.

**NSA Kalman filter** — `kalman_type=1` selects `NSAKalmanFilter`, a variant that scales measurement noise by detection confidence in both `initiate` and `project`.

**Kinematic FSM** — `enable_fsm=1`. Classifies motion using thresholds `fsm_stationary_vel_thresh` (default 40 units/s) and `fsm_maneuver_accel_thresh` (default 250 units/s²). `Coasting` is assigned during Lost intervals; `getLastSeenState()` returns the regime active immediately before Coasting so downstream policies can reason about the true motion profile.

**Interaction context** — `enable_interaction_context=1` (requires `enable_fsm`). `BottomYInteractionGraph` builds a per-frame occlusion graph using DIoU activation (`interaction_iou_thresh`, default 0.3) and bottom-Y depth ordering with a kinematic tiebreaker. When a track transitions to `Lost`, the front-most occluder identifier is snapshotted on the tracklet. The lost-recovery and primary stages add `interaction_occluder_penalty` (default 0.3) to candidate detections that geometrically coincide with the still-valid occluder. Snapshot validity (`interaction_occluder_validity_seconds`, default 2.0 s ≈ 50 frames at 25 fps) is scaled per occluder kinematic state (`Stationary`×1.5, `Cruising`×1.0, `Coasting`×0.7, `Maneuvering`×0.5). Penalty magnitude is further scaled by the occludee's kinematic state and by velocity alignment between occluder and occludee (cosine of heading angle mapped into [0.6, 1.4]).

**Coordinate representations** — `enable_xyah=0` (default) uses TLWH state (`[x, y, w, h, vx, vy, vw, vh]`); `enable_xyah=1` uses centroid + aspect ratio + height (`[cx, cy, a, h, …]`). `iou_plus_one=1` (default) uses inclusive box dimensions consistent with the ByteTrack reference.

**Primary association mode** — `primary_assoc_mode=1` (default) runs a single global LAPJV solve over the high-confidence pool. `primary_assoc_mode=0` retains a legacy age-cascade path.

**Duplicate suppression** — `duplicate_iou_thresh` (default 0.15, i.e. IoU > 0.85) removes redundant tracklets after each frame using a plain-IoU calculator.

**LAPJV solver** — `LAPJVSolver` runs the Jonker–Volgenant algorithm with active-row/column reduction. Workspace buffers are reused across frames to remove per-frame heap pressure.

**Timing policies** — `time_aware=0` uses `FrameTimingPolicy` (dt=1.0 each call, expiry by frame count). `time_aware=1` uses `WallClockTimingPolicy`: dt is treated as real seconds and multiplied by `frame_rate` to obtain a KF step, capped at 5.0. Track expiry compares accumulated wall time against `track_buffer / frame_rate` seconds (strict >, with a 1e-4 epsilon for symmetry with the frame-counted path).

**C API** — `SushiTrackC.h` exposes an opaque `SushiTrackHdl` plus `SushiTrack_GetDefaultParams`, `SushiTrack_Create`, `SushiTrack_CreateEx`, `SushiTrack_Update`, `SushiTrack_Destroy`, `SushiTrack_SetLogCallback`. Suitable for FFI integration from Python, Rust, or any C-FFI language.

**Logging** — `SUSHITRACK_LOG_DEBUG/INFO/WARN/ERROR` macros route to a single global callback registered via `SushiTrack_SetLogCallback`. The callback is stored atomically; setting/clearing it is thread-safe.

**Thread safety** — `Tracklet` state access is mutex-protected for `getMean`/`getCovariance`/`getMeasurement`. The `KalmanFilter` LLT projection cache is mutex-protected.

---

## Requirements

| Component | Version |
|---|---|
| C++ compiler | C++17 (MSVC 19.20+, GCC 9+, Clang 10+) |
| CMake | ≥ 3.14 |
| Eigen | vendored at `3rdparty/eigen` |
| GoogleTest | provided via conda or system package |
| Ninja | provided via conda |
| Python | 3.11 |
| Docker (optional) | Engine ≥ 20.10 with NVIDIA Container Toolkit for CUDA inference |

Python runtime dependencies (managed by `environment.yml` / `Dockerfile`): `numpy==2.4.3`, `pandas==3.0.2`, `scipy==1.17.1`, `pytest==9.0.3`, `absl-py==2.4.0`, `loguru==0.7.3`, `matplotlib==3.10.9`, `opencv-python==4.13.0.92`, `pillow==12.1.1`, `pycocotools==2.0.11`, `rich==15.0.0`, `tqdm==4.67.3`, `torch==2.5.1+cu121`, `torchvision==0.20.1+cu121`, `torchaudio==2.5.1+cu121`.

Compiler flags: MSVC compiles with `/W3 /O2 /Ot /fp:fast /arch:AVX2 /DNDEBUG`; GCC/Clang compile with `-Wall -O3 -ffast-math -funroll-loops -DNDEBUG`. Library is built `SHARED`; on Windows `CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS` is `ON`. Outputs are unified under `${CMAKE_BINARY_DIR}/bin` and `${CMAKE_BINARY_DIR}/lib`.

---

## Project Structure

```
sushitrack/
├── CMakeLists.txt                # Top-level build; defines library + 3rdparty subdirs + tests
├── Dockerfile                    # CUDA 12.1 base, system deps, Python wheels, alias setup
├── environment.yml               # Conda env: python 3.11, cmake, gtest, ninja, pip wheels
├── README.md
│
├── include/
│   ├── SushiTrackC.h              # C-compatible public API
│   └── SushiTrack/
│       ├── SushiTrackTracker.h    # Core orchestrator
│       ├── SushiTrackTypes.h      # Status codes, log levels, callback typedef
│       ├── TrackerConfig.h       # C++ config struct (mirror of SushiTrack_Params)
│       ├── Tracklet.h            # Per-track state, lifecycle, feature history
│       ├── TrackFSM.h            # Kinematic FSM
│       ├── InteractionState.h    # Free / Occluded enum
│       ├── InteractionGraph.h    # BottomYInteractionGraph
│       ├── IInteractionGraph.h   # Strategy interface
│       ├── KalmanFilter.h        # Linear Kalman filter
│       ├── NSAKalmanFilter.h     # Noise-Scale Adaptive variant
│       ├── KalmanFilterRegistry.h# Plug-in registry
│       ├── KalmanTypes.h         # Eigen matrix typedefs
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
├── src/                          # Library implementations (mirrors include/)
│
├── 3rdparty/
│   ├── eigen/                    # Header-only linear algebra
│   ├── bytetrack/                # Reference ByteTrack for regression comparison
│   ├── ocsort/                   # Reference OC-SORT for regression comparison
│   ├── TrackEval/                # Official MOT-Challenge evaluator (used optionally)
│   └── yolox/                    # YOLOX detector (used by the inference pipeline)
│
├── tests/
│   ├── CMakeLists.txt            # Options BUILD_UNIT_TEST / BUILD_REGRESSION_TEST
│   ├── unit/                     # GTest deterministic unit tests
│   │   ├── CMakeLists.txt
│   │   ├── UnitTest.cpp          # GTest main
│   │   ├── TestTrackFSM.cpp
│   │   ├── TestKalmanFilter.cpp
│   │   ├── TestTracklet.cpp
│   │   ├── TestRect.cpp
│   │   ├── TestReID.cpp
│   │   ├── TestCostMatrix.cpp
│   │   ├── TestTimingPolicies.cpp
│   │   ├── TestDistanceStrategies.cpp
│   │   ├── TestAssignmentSolvers.cpp
│   │   ├── TestInteractionGraph.cpp
│   │   ├── TestTrackerTentative.cpp
│   │   ├── TestTrackerBuffers.cpp
│   │   ├── TestLogger.cpp
│   │   ├── TestSushiTrackC.cpp
│   │   ├── TestSushiTrackTracker.cpp
│   │   ├── TestCAPIErrors.cpp
│   │   ├── TestIntegration.cpp
│   │   ├── TestBoundaryConditions.cpp
│   │   └── TestConfiguration.cpp
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
│           └── tracker_bridge.py # ctypes binding to libtracker_bridge.so / .dll
│
├── docs/
│   ├── CHANGELOG.md
│   └── benchmarks/PRIVATE.md     # Internal benchmark log
│
└── .tools/
    ├── sushitrack.py              # CLI entry point (argparse, subcommand dispatcher)
    ├── sushitrack.sh              # Direct shell wrapper
    ├── ds.sh                     # Short shell wrapper
    ├── install.sh                # Session installer (bash/zsh)
    ├── install.ps1               # Session installer (PowerShell)
    ├── paths_manager.py          # Loads .tools/paths.env overrides
    ├── docker/
    │   └── run.sh                # Bind-mounted container launcher
    ├── project/
    │   ├── run.sh                # Forwards to sushitrack.py project run
    │   └── clean.sh              # Removes build/
    └── misc/
        ├── clean_preds.sh        # Wipes pred/ directories under evaluator/data
        └── construct_trackeval_data.ps1 # Links GT + copies preds into TrackEval layout
```

---

## Installation

### CLI installer (PowerShell / Bash)

The installer registers `sushitrack` and `ds` as aliases that invoke `.tools/sushitrack.py` and prepends `.tools/` to `PATH` for the current session.

PowerShell:

```powershell
Unblock-File ./.tools/install.ps1
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
. ./.tools/install.ps1
```

Bash/Zsh:

```bash
source .tools/install.sh
```

To make permanent, append the printed lines to `$PROFILE` (PowerShell) or `~/.bashrc` / `~/.zshrc` (Unix).

### Conda environment

```bash
conda env create -f environment.yml
conda activate sushitrack
```

`environment.yml` installs Python 3.11, CMake, Ninja, GoogleTest, and the full pip dependency set (numpy/pandas/scipy/torch+cu121/opencv/loguru/rich/pytest/etc.).

### Docker

```bash
sushitrack docker build      # builds image from ./Dockerfile (CUDA 12.1, Ubuntu 22.04)
sushitrack docker run        # docker run -it --rm -v $(pwd):/workspace/sushitrack sushitrack bash
```

The Dockerfile pins the same wheel versions as `environment.yml`. The project root is bind-mounted at `/workspace/sushitrack`; output files written inside the container are immediately visible on the host. `ds` and `sushitrack` aliases are pre-registered in the container shell.

---

## Build

The build uses CMake with three options:

| Option | Default | Effect |
|---|---|---|
| `BUILD_UNIT_TEST` | OFF | Compiles `unit_test` GTest executable |
| `BUILD_REGRESSION_TEST` | OFF | Compiles `regression_test` executable and `tracker_bridge` shared library |
| `BUILD_MOT_TEST` | OFF | Deprecated alias for `BUILD_REGRESSION_TEST` |

Direct CMake invocation:

```bash
cmake -B build -S . -G Ninja -DBUILD_UNIT_TEST=ON -DBUILD_REGRESSION_TEST=ON
cmake --build build --config Release
```

Through the CLI (preferred):

```bash
sushitrack project build                      # library only
sushitrack project build --unit               # library + unit_test
sushitrack project build --regression         # library + regression_test + tracker_bridge
sushitrack project build --test               # alias for --unit --regression (deprecated)
sushitrack project build --clean              # wipe build/ first; combinable with the above
```

Artefacts: `build/bin/{Release,Debug}/sushitrack.{dll,so}`, `build/bin/.../unit_test{,.exe}`, `build/bin/.../regression_test{,.exe}`, `build/bin/.../tracker_bridge.{dll,so}`.

---

## CLI Reference

All commands take the form `sushitrack <command> <action> [flags]`. `ds` is a shorthand alias.

### `project build`

| Flag | Effect |
|---|---|
| `--clean` | Remove the build directory before configuring |
| `--unit` | Add `unit_test` target |
| `--regression` | Add `regression_test` + `tracker_bridge` |
| `--test` | Deprecated alias for `--unit --regression` |

### `project test all`

Default when `test_type` is omitted. Runs the unit suite first; if it passes, runs the label regression suite. Fail-fast: label is skipped if unit fails.

```bash
sushitrack project test                # equivalent to 'project test all'
```

### `project test unit`

Runs the compiled `unit_test` GTest binary.

| Flag | Effect |
|---|---|
| `--filter <expr>` | Passes through as `--gtest_filter=<expr>` |
| `--verbose` | Adds `--gtest_print_time=1` |

```bash
sushitrack project test unit
sushitrack project test unit --filter "TrackFSMTest.*" --verbose
```

### `project test label`

Runs `regression_test` on det.txt-backed MOT/DanceTrack scenarios, parses per-sequence `[ PERF ]` lines from the binary output, then runs TrackEval and prints a metrics table (HOTA / DetA / AssA / MOTA / IDF1 / IDSW / FP / FN / Lat).

| Flag | Effect |
|---|---|
| `--scenario <name>` | Sequence filter (exact match or case-insensitive substring) |
| `--tracker <name>` | `SushiTrack` (default), `ByteTrack`, or `OCSORT` |
| `--compare` | Run all three trackers side-by-side |
| `--eval-only` | Skip the binary; re-evaluate existing `pred/*.txt` files |
| `--trackeval` | Use the official `3rdparty/TrackEval/scripts/run_mot_challenge.py` instead of the internal runner |
| `--export <dir>` | Export TrackEval JSON/CSV to `<dir>` |
| `--private` | Run only `*-YOLOX` sequences (MOT only) |
| `--public` | Run all sequences except `*-YOLOX` (MOT only) |

```bash
sushitrack project test label
sushitrack project test label --compare
sushitrack project test label --scenario MOT17-04-DPM --compare
sushitrack project test label --eval-only --export ./results
sushitrack project test label --trackeval --private
```

Scenarios registered in `ScenarioList.h`: full MOT17 train set (21 sequences across DPM/FRCNN/SDP detectors plus `-YOLOX` variants for each), MOT20 (`-01/-02/-03/-05` with `-YOLOX` variants), and DanceTrack sequences (`dancetrack0001`, `0002`, `0006`, `0008`, `0012`, `0015`, …).

### `project test inference`

Launches the YOLOX-driven video inference pipeline (`tests/regression/inference/demo.py`) which loads `tracker_bridge.{dll,so}` via ctypes. Requires `--regression` build.

| Flag | Default | Effect |
|---|---|---|
| `--source <path>` | required | Video file path or camera index |
| `--tracker <name>` | `SushiTrack` | Tracker selector (`SushiTrack` / `ByteTrack` / `OCSORT`) |
| `-n, --name <name>` | `yolox-s` | YOLOX architecture |
| `-c, --ckpt <path>` | – | Pre-trained YOLOX weights |
| `--device {cpu,gpu}` | `gpu` | Inference device |
| `--conf <float>` | `0.1` | Detection confidence floor |
| `--nms <float>` | `0.45` | NMS IoU threshold |
| `--tsize <H W \| S>` | – | Inference resolution; single value treated as square |
| `--num-classes <int>` | `80` | Class count (set `1` for MOT) |
| `--legacy` | off | Legacy YOLOX normalisation (BGR→RGB, 0–1 scale, ImageNet mean/std) |
| `--stress` | off | Headless max-throughput mode |
| `--no_save` | off | Disable output-video writer |
| `--save_name <name>` | – | Output filename override |
| `--mot-label` | off | Append detections to `output/det.txt` in MOT format |
| `--no-screen` | off | Disable UI display |

Examples:

```bash
sushitrack project test inference \
    -n yolox-x -c 3rdparty/weight/mot17x.pth.tar \
    --source 3rdparty/video/MOT17-04.mp4 \
    --num-classes 1 --legacy

sushitrack project test inference \
    --source 0 --tracker SushiTrack --no_save --no-screen --stress
```

If the YOLOX checkpoint filename contains `mot17` and `--tsize` is omitted/640, the pipeline overrides `tsize` to `(800, 1440)` to match the MOT17 training resolution.

### `project clean`

Removes the build directory (`build/` by default; overridable via `.tools/paths.env`).

```bash
sushitrack project clean
```

### `docker build` / `docker run`

Wrappers around `.tools/docker/build.sh` and `.tools/docker/run.sh`. The run script issues `docker run -it --rm -v "$(pwd):/workspace/sushitrack" sushitrack bash`.

```bash
sushitrack docker build
sushitrack docker run
```

---

## C API

Header: `include/SushiTrackC.h`. Link against the compiled `sushitrack` shared library.

Minimal usage:

```c
#include "SushiTrackC.h"

SushiTrack_Params params;
SushiTrack_GetDefaultParams(&params);
params.enable_fsm        = 1;
params.enable_tentative  = 1;
params.frame_rate        = 25;

SushiTrackHdl hdl;
SushiTrack_Status s = SushiTrack_CreateEx(&params, &hdl);
if (s != SUSHITRACK_SUCCESS) { /* handle */ }

SushiTrack_Object  dets[MAX_DETS];   /* fill in per frame */
SushiTrack_Track   tracks[MAX_TRACKS];
int n = SushiTrack_Update(hdl, dets, n_dets, tracks, MAX_TRACKS, dt);

SushiTrack_Destroy(hdl);
```

`SushiTrack_Create` returns `NULL` on failure; `SushiTrack_CreateEx` returns a `SushiTrack_Status` code and writes the handle through `out_hdl`. `SushiTrack_Update` returns the number of tracks written. Output tracks are sorted by lifecycle priority (`Tracked` < `Tentative` < `Lost` < `Removed`); the buffer is truncated if too small, with a warning logged.

The track-output `feat` pointer references memory owned by the tracker and is only valid until the next `SushiTrack_Update` or `SushiTrack_Destroy`.

### Status codes (`SushiTrack_Status`)

| Code | Value |
|---|---|
| `SUSHITRACK_SUCCESS` | 0 |
| `SUSHITRACK_INVALID_ARGUMENT` | −1 |
| `SUSHITRACK_INITIALIZATION_FAILURE` | −2 |
| `SUSHITRACK_OUT_OF_MEMORY` | −3 |
| `SUSHITRACK_RUNTIME_ERROR` | −4 |
| `SUSHITRACK_BUFFER_OVERFLOW` | −5 |

### Logging

```c
void my_logger(SushiTrack_LogLevel level, const char* msg) { /* … */ }
SushiTrack_SetLogCallback(my_logger);  /* pass NULL to unregister */
```

`SushiTrack_LogLevel` is `DEBUG=0`, `INFO=1`, `WARN=2`, `ERROR=3`. The callback is stored atomically and safe to set/clear from any thread.

### Track output fields

`SushiTrack_Track` includes `track_id`, `age` (tracklet length in frames), `last_seen_frame`, `feat`/`feat_dim`, `x/y/width/height`, `score`, `lost_time` (seconds or frames depending on `time_aware`), `label`, `state` (TrackState enum integer), `kinematic_state` (`0=Stationary, 1=Cruising, 2=Maneuvering, 3=Coasting`), `is_activated`, `is_confirmed`, `hits`, and `frames_since_last_update`.

---

## Data Layout

The regression harness reads from `tests/regression/evaluator/data/`:

```
tests/regression/evaluator/data/
├── config.json                       # optional override file
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

Prediction files are written to `<sequence>/pred/<tracker>/<sequence>.txt` after each regression run. `.tools/misc/clean_preds.sh` wipes every `pred/` subdirectory.

### Tracker config file

Override compiled defaults at runtime through a JSON file. Resolution order: `$SUSHITRACK_CONFIG` env var → `tests/config.json` → `tests/regression/evaluator/data/config.json` → `data/config.json`. Any missing key falls back to the compiled default.

```json
{
  "sushitrack": {
    "track_thresh": 0.2, "high_thresh": 0.5, "match_thresh": 0.7,
    "frame_rate": 25, "track_buffer": 30,
    "enable_reid": 0, "enable_fsm": 0, "enable_tentative": 0,
    "enable_mahalanobis": 0, "enable_interaction_context": 0,
    "iou_type": 1, "enable_xyah": 0, "iou_plus_one": 1,
    "time_aware": 0, "kalman_type": 0,
    "primary_assoc_mode": 1, "fuse_score": 1,
    "fsm_stationary_vel_thresh": 40.0, "fsm_maneuver_accel_thresh": 250.0,
    "reid_high_thresh": 0.3, "reid_high_iou_weight": 0.3,
    "reid_low_thresh": 0.5,  "reid_low_iou_weight": 0.2,
    "reid_recovery_thresh": 0.4
  },
  "bytetrack": { "track_thresh": 0.5, "high_thresh": 0.6, "match_thresh": 0.8,
                 "frame_rate": 30, "track_buffer": 30 },
  "ocsort":    { "...": "..." }
}
```

---

## Configuration Parameters

All knobs are exposed both on `SushiTrack_Params` (C API, `include/SushiTrackC.h`) and `SushiTrack::TrackerConfig` (C++, `include/SushiTrack/TrackerConfig.h`). Defaults below are from `SushiTrack_GetDefaultParams`.

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
| `tentative_max_age_seconds` | 0.08 | Wall-clock or frame-count budget for tentative expiry (≈ 2 frames at 25 fps) |
| `tentative_fast_thresh` | 0.85 | Above this score, `tentative_fast_hits` hits are enough |
| `tentative_fast_hits` | 1 | Hit count for fast-confirmation regime |
| `tentative_slow_thresh` | 0.40 | Below this score, extra hits are required |
| `tentative_slow_hits_offset` | 1 | Extra hits added in `[slow_thresh, high_thresh)` |
| `tentative_vslow_hits_offset` | 2 | Extra hits added below `slow_thresh` |

### Motion model

| Key | Default | Description |
|---|---|---|
| `kalman_type` | 0 | 0 = Linear KF, 1 = NSA KF |
| `enable_xyah` | 0 | 0 = TLWH state, 1 = centroid+aspect+height state |
| `iou_plus_one` | 1 | Inclusive bounding-box dimensions |

`KalmanFilter` baseline constants (compile-time, see `include/SushiTrack/KalmanFilter.h`): `kStdWeightPosition=1/20`, `kStdWeightVelocity=1/160`, `kInitPositionMult=2.0`, `kInitVelocityMult=10.0`. These replace earlier configurable hyper-parameters and match the ByteTrack reference tuning.

### Association strategy

| Key | Default | Description |
|---|---|---|
| `primary_assoc_mode` | 1 | 0 = age-cascade legacy, 1 = global LAPJV |
| `fuse_score` | 1 | Score-fusion decorator on the primary cost matrix |
| `iou_type` | 1 | 0=IoU, 1=DIoU, 2=GIoU, 3=SIoU |
| `enable_mahalanobis` | 0 | Apply Mahalanobis gating |
| `gating_thresh` | 9.4877 | Chi-squared threshold (95 % CI, 4 DoF) |
| `proximity_thresh` | 0.7 | Broad spatial filter before gating |
| `motion_cost_weight` | 0.15 | Weight of squared Mahalanobis distance in gated IoU |
| `max_area_ratio` | 50.0 | Suppress pairs with extreme area disparity |

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

### Kinematic FSM

| Key | Default | Description |
|---|---|---|
| `enable_fsm` | 0 | Per-track kinematic state machine |
| `fsm_stationary_vel_thresh` | 40.0 | Velocity (units/s) below which → Stationary |
| `fsm_maneuver_accel_thresh` | 250.0 | Acceleration (units/s²) above which → Maneuvering |

### Interaction context

| Key | Default | Description |
|---|---|---|
| `enable_interaction_context` | 0 | Per-frame interaction graph + occluder-aware penalty (requires `enable_fsm`) |
| `interaction_iou_thresh` | 0.3 | Minimum DIoU to register an interaction pair |
| `interaction_occluder_penalty` | 0.3 | Additive cost penalty against still-valid occluders |
| `interaction_occluder_validity_seconds` | 2.0 | Snapshot lifetime (scaled by occluder kinematic state) |

### Timing

| Key | Default | Description |
|---|---|---|
| `time_aware` | 0 | 0 = treat dt as 1 step; 1 = treat dt as real seconds (capped at 5 KF-steps) |

---

## Test Suites

### Unit tests (`unit_test`)

GTest-based, deterministic, no external data. Targets compiled when `-DBUILD_UNIT_TEST=ON`. Cover:

- `TestTrackFSM` — kinematic transitions, hysteresis, warmup, last-seen state
- `TestKalmanFilter` — initiate/predict/update, Mahalanobis distance, gate flags, LLT cache
- `TestTracklet` — lifecycle transitions, feature EMA, occluder snapshot
- `TestRect` — TLWH/TLBR/XYWH conversions
- `TestReID` — cosine similarity, max-history lookup
- `TestCostMatrix` — resize semantics, sentinel preservation
- `TestTimingPolicies` — frame vs wall-clock conversion and expiry symmetry
- `TestDistanceStrategies` — each cost calculator (IoU/DIoU/GIoU/SIoU/Cosine/ReIDFirst/PureReID/Fused/GatedIoU/FuseScoreDecorator)
- `TestAssignmentSolvers` — LAPJV correctness on dense and sparse matrices
- `TestInteractionGraph` — build, occluder selection, kinematic tiebreaker, overlap query
- `TestTrackerTentative` — adaptive confirmation logic
- `TestTrackerBuffers` — track_buffer expiry under both timing modes
- `TestLogger` — callback dispatch, va_list path
- `TestSushiTrackC` / `TestCAPIErrors` — C-API surface and error codes
- `TestSushiTrackTracker` / `TestIntegration` — end-to-end flow
- `TestBoundaryConditions` — empty inputs, oversized output buffer, null args
- `TestConfiguration` — config-driven parameter parsing

Direct invocation:

```bash
./build/bin/Release/unit_test
./build/bin/Release/unit_test --gtest_filter="TrackFSMTest.*"
ctest -R unit --output-on-failure
```

### Regression tests (`regression_test`)

Parameterised GTest binary that iterates over `Scenario × tracker_factory`. For each pair it:

1. Reads `det.txt` frames via `MOTRunner`.
2. Drives the tracker through `ITrackerAdapter` (`SushiTrackAdapter`, `ByteTrackAdapter`, `OCSORTAdapter`).
3. Writes predictions to `<sequence>/pred/<tracker>/<sequence>.txt`.
4. Emits a `[ PERF <tracker> ] <sequence>: <us/frame> us/frame` line read back by `sushitrack.py`.

Config resolution: `$SUSHITRACK_CONFIG` → `tests/config.json` → `tests/regression/evaluator/data/config.json` → `data/config.json`.

Direct invocation:

```bash
./build/bin/Release/regression_test
./build/bin/Release/regression_test --gtest_filter="*MOT17_04_DPM_sushitrack*"
SUSHITRACK_CONFIG=tests/config.json ./build/bin/Release/regression_test
```

---

## Evaluation Pipeline

`tests/regression/evaluator/`:

- **`trackeval_runner.py`** — wraps the TrackEval Python module. Builds per-benchmark sequence maps, configures `MotChallenge2DBox` dataset, runs `HOTA`, `CLEAR`, `Identity` metrics.
- **`report_generator.py`** — `print_trackeval_table(results, compare)` formats the per-tracker × per-sequence table; `export_trackeval_results(results, dir)` writes JSON/CSV.
- **`run_eval.py`** — standalone CLI for `TrackEvalRunner` (independent of the compiled binary).

`sushitrack project test label` automatically chains regression_test → TrackEval; pass `--eval-only` to skip the C++ binary and re-evaluate cached predictions, or `--trackeval` to invoke the upstream `3rdparty/TrackEval/scripts/run_mot_challenge.py` script.

---

## Inference Pipeline

`tests/regression/inference/`:

- **`tracker_bridge.py`** — `TrackerBridge` ctypes wrapper that loads `tracker_bridge.{dll,so}`. Exposes `create_tracker(name)`, `tracker_process_c(...)`, `delete_tracker(...)`. The shared library is built from `tests/regression/TrackerRegistry.cpp` when `-DBUILD_REGRESSION_TEST=ON`.
- **`pipeline.py`** — defines `IDetector`, `YOLOXDetector` (loads weights via `yolox.exp.get_exp`, preprocesses with optional legacy normalisation, runs inference, postprocesses with NMS), and `VideoPipeline` (frame loop, detection→tracker hand-off, drawing, optional MOT-format detection logging, stress/no-screen modes).
- **`demo.py`** — argparse front-end exposing the full YOLOX + tracker flag set. Invoked through `sushitrack project test inference`.

Example end-to-end run:

```bash
sushitrack project build --regression
sushitrack project test inference \
    -n yolox-x -c 3rdparty/weight/mot17x.pth.tar \
    --source 3rdparty/video/MOT17-04.mp4 \
    --tracker SushiTrack --num-classes 1 --legacy
```

The pipeline auto-detects MOT17 checkpoints by filename and sets `tsize=(800,1440)` accordingly.

---

## Internal Tools

| File | Purpose |
|---|---|
| `.tools/sushitrack.py` | argparse-based CLI; subcommand dispatcher; binary discovery; TrackEval orchestration |
| `.tools/paths_manager.py` | Loads `paths.env` overrides for `DIR_TOOLS`, `DIR_BUILD`, `DIR_DATA`, `DIR_3RDPARTY`, `DIR_TESTS`, `DIR_REGRESSION`, `DIR_EVALUATOR`, `DIR_INFERENCE` |
| `.tools/install.sh` | Bash/Zsh aliases and PATH setup; prints permanent-install lines |
| `.tools/install.ps1` | PowerShell session functions for `ds` / `sushitrack` |
| `.tools/sushitrack.sh` / `.tools/ds.sh` | Shell wrappers — `python3 sushitrack.py "$@"` |
| `.tools/project/run.sh` | Forwards to `sushitrack.py project run` |
| `.tools/project/clean.sh` | Removes the build directory honouring `DIR_BUILD` from `paths.env` |
| `.tools/docker/run.sh` | `docker run -it --rm -v $(pwd):/workspace/sushitrack sushitrack bash` |
| `.tools/misc/clean_preds.sh` | Recursively removes `pred/` directories under the evaluator data root |
| `.tools/misc/construct_trackeval_data.ps1` | Creates the official TrackEval directory tree by symlinking GT (MOT17-SDP as representative for each sequence) and copying per-tracker predictions for `sushitrack`/`bytetrack`/`ocsort` |

`Dockerfile`: CUDA 12.1 devel base, installs `build-essential`, `cmake`, `git`, `python3`, `python3-pip`, `libgtest-dev`, `libgl1-mesa-glx`, `libglib2.0-0`, then pip-installs the pinned wheel set, copies the project, and registers `ds`/`sushitrack` aliases in `.bashrc`.

---

## Benchmark Results

Evaluated on MOT17 / MOT20 train sets with public detectors. SushiTrack compared against the bundled ByteTrack reference at matched parameters. Metrics from TrackEval.

**MOT17 train**

| Tracker | HOTA | DetA | AssA | MOTA | IDF1 | IDSW | FP | FN | Lat (µs/frame) |
|---|---|---|---|---|---|---|---|---|---|
| SushiTrack | 41.5 % | 40.8 % | 43.4 % | 46.7 % | 50.2 % | 1734 | 6448 | 168570 | 46 |
| ByteTrack | 39.1 % | 40.9 % | 38.7 % | 46.5 % | 47.1 % | 2763 | 8827 | 165486 | 71 |

**MOT20 train**

| Tracker | HOTA | DetA | AssA | MOTA | IDF1 | IDSW | FP | FN | Lat (µs/frame) |
|---|---|---|---|---|---|---|---|---|---|
| SushiTrack | 24.9 % | 16.9 % | 42.2 % | 17.7 % | 24.2 % | 189 | 235 | 1037435 | 67 |
| ByteTrack | 23.0 % | 15.6 % | 40.5 % | 16.6 % | 21.6 % | 200 | 243 | 1045728 | 124 |

**Overall**

| Tracker | HOTA | DetA | AssA | MOTA | IDF1 | IDSW | FP | FN | Lat (µs/frame) |
|---|---|---|---|---|---|---|---|---|---|
| SushiTrack | 38.8 % | 37.0 % | 43.2 % | 42.0 % | 46.0 % | 1923 | 6683 | 1206005 | 50 |
| ByteTrack | 36.5 % | 36.8 % | 39.0 % | 41.7 % | 43.0 % | 2963 | 9070 | 1211214 | 80 |

Latency is mean per-frame `update()` time, single-threaded, excluding I/O.

Internal private-detection benchmark (see `docs/benchmarks/PRIVATE.md`):

| Tracker | HOTA | DetA | AssA | MOTA | IDF1 | IDSW | FP | FN | Lat (µs) |
|---|---|---|---|---|---|---|---|---|---|
| SushiTrack | 64.8 % | 71.8 % | 59.0 % | 83.0 % | 74.5 % | 3215 | 15287 | 161629 | 455 |
| ByteTrack | 60.6 % | 71.0 % | 52.5 % | 82.2 % | 68.7 % | 5133 | 30837 | 150356 | 606 |
| OC-SORT | 62.2 % | 70.3 % | 55.6 % | 80.6 % | 71.1 % | 3665 | 33487 | 169143 | 1729 |

---

## License

Copyright © 2026 Mustafa Garip & Sushi Systems All rights reserved.

This software and all associated source files are proprietary. No part of this codebase may be copied, modified, distributed, sublicensed, or used in any form without the prior written consent of Mustafa Garip & Sushi Systems
