# SushiTrack

A multi-object tracking library written in C++17. SushiTrack extends the core ByteTrack pipeline with a modular, interface-driven architecture and a set of optional features — a kinematic finite state machine, tentative track validation, visual ReID fusion, multiple cost calculators, and real-time timing — all accessible through a single C API and a clean CLI.

---

Unblock-File ./.tools/install.ps1
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser

## Contents

- [Features](#features)
- [Requirements](#requirements)
- [Project Structure](#project-structure)
- [Getting Started](#getting-started)
  - [Option A — Docker](#option-a--docker)
  - [Option B — Conda](#option-b--conda)
- [CLI Reference](#cli-reference)
- [C API](#c-api)
- [Data Layout](#data-layout)
- [Configuration](#configuration)
- [Benchmark Results](#benchmark-results)
- [License](#license)

---

## Features

**Core tracking pipeline** is compatible with ByteTrack's two-stage association model: high-confidence detections matched first, low-confidence detections matched against remaining tracks.

**Tentative track validation** — new tracks enter a tentative state and must accumulate a configurable number of consecutive matches before receiving a confirmed identity. Reduces ID proliferation from spurious detections.

**Kinematic FSM** — classifies each track as stationary, cruising, maneuvering, or coasting based on estimated velocity and acceleration. Kalman process noise is adjusted per-state to improve prediction accuracy under variable motion.

**Visual ReID** — optional feature vector path supports cosine distance, a ReID-first strategy (appearance with IoU fallback), pure appearance matching, and a fused calculator combining IoU, cosine distance, and Mahalanobis gating.

**Distance-IoU (DIoU)** — penalizes center-point distance in addition to overlap, configurable as a drop-in replacement for standard IoU.

**Mahalanobis gating** — filters association candidates using chi-squared thresholds on predicted state distributions before computing full cost matrices.

**Real-time timing** — a `time_aware` mode normalizes wall-clock intervals to frame-equivalent steps, keeping the Kalman filter and track expiration logic calibrated for variable-rate inputs.

**C API** — a stable C-compatible interface (`SushiTrackC.h`) exposes the full tracker as an opaque handle. Suitable for FFI integration from Python, Rust, or any language with a C FFI.

**LAPJV solver** — Jonker-Volgenant linear assignment with active-row/column reduction and pre-allocated workspace buffers to minimize per-frame heap pressure.

---

## Requirements

**Build (native)**

| Requirement | Version |
|---|---|
| C++ compiler | C++17 or later |
| CMake | provided via conda environment |
| Eigen | 3.x |
| GoogleTest | provided via conda environment |
| Ninja | provided via conda environment |

**Python environment** — Python 3.11 and all evaluation dependencies are managed through the provided conda environment file. This covers the build tooling (cmake, ninja, gtest), the evaluation stack (trackeval, motmetrics, numpy, pandas, scipy), and the test runner (pytest).

**Docker** — Docker Engine 20.10 or later. Everything else is handled inside the image.

---

## Project Structure

```
sushitrack/
├── .tools/
│   ├── sushitrack.py          # CLI entry point
│   ├── install.sh            # Session installer (Linux / macOS)
│   ├── install.ps1           # Session installer (PowerShell)
│   ├── logger.py
│   ├── project/
│   │   ├── build.sh
│   │   └── clean.sh
│   └── docker/
│       ├── build.sh
│       └── run.sh
├── src/                      # C++ library source
├── test/
│   ├── cpp/                  # C++ benchmark harness
│   │   ├── MainTest.cpp
│   │   ├── MOTRunner.h
│   │   ├── ScenarioList.h
│   │   ├── TestInfra.h
│   │   ├── SushiTrackAdapter.h
│   │   └── ByteTrackAdapter.h
│   └── python/               # Evaluation pipeline
│       ├── trackeval_runner.py
│       ├── report_generator.py
│       ├── data_loader.py
│       └── mot_evaluator.py
├── data/                     # MOT sequences (user-provided)
│   ├── config.json
│   └── <SequenceName>/
│       ├── det/det.txt
│       ├── gt/gt.txt
│       └── seqinfo.ini
└── Dockerfile
```

---

## Getting Started

### Option A — Docker

Build the image once, then drop into a shell with the full environment ready.

```bash
# 1. Install the CLI for the current session
source .tools/install.sh

# 2. Build the Docker image
sushitrack docker build

# 3. Start a container (project root is mounted at /workspace/sushitrack)
sushitrack docker run
```

Inside the container all `sushitrack` commands work the same way. The project directory is bind-mounted so any files written (prediction outputs, exports) are visible on the host immediately.

---

### Option B — Conda

**Create the environment**

```bash
conda env create -f environment.yml
conda activate sushitrack
```

The environment file installs cmake, ninja, gtest, Python 3.11, and the full evaluation stack (trackeval, motmetrics, numpy, pandas, scipy, pytest) in one step. Eigen is not in the environment file — install it separately if your system does not already provide it (e.g. `conda install -c conda-forge eigen`).

**Install the CLI**

```bash
# Linux / macOS (bash or zsh)
source .tools/install.sh

# PowerShell
. .tools/install.ps1
```

This registers `sushitrack` and the shorthand `ds` as aliases for the current session. To make them permanent, add the printed lines to `~/.bashrc`, `~/.zshrc`, or `$PROFILE`.

**Build the library**

```bash
sushitrack project build
```

**Build including the test binary**

```bash
sushitrack project build --test
```

**Clean build**

```bash
sushitrack project build --clean
sushitrack project build --clean --test
```

---

## CLI Reference

All commands follow the pattern:

```
sushitrack <command> <action> [flags]
```

### `project build`

Compiles the C++ library and optionally the benchmark test binary.

| Flag | Effect |
|---|---|
| *(none)* | Build the library in release mode |
| `--test` | Include the `mot_test` benchmark binary |
| `--clean` | Remove previous build artifacts before building |

```bash
sushitrack project build
sushitrack project build --test
sushitrack project build --clean --test
```

---

### `project test`

Runs the compiled benchmark binary against MOT sequences, then pipes the prediction files through TrackEval and prints a results table.

| Flag | Argument | Effect |
|---|---|---|
| *(none)* | — | Run SushiTrack on all sequences, evaluate |
| `--compare` | — | Run both SushiTrack and ByteTrack, show side-by-side results |
| `--scenario` | `<name>` | Restrict to a single sequence, e.g. `MOT17-04-DPM` |
| `--eval-only` | — | Skip the binary, re-evaluate existing prediction files |
| `--export` | `<dir>` | Write evaluation results as JSON/CSV to the given directory |

```bash
# Run SushiTrack on all sequences
sushitrack project test

# Compare SushiTrack against ByteTrack on all sequences
sushitrack project test --compare

# Single sequence
sushitrack project test --scenario MOT17-04-DPM

# Compare on a single sequence
sushitrack project test --scenario MOT17-04-DPM --compare

# Re-evaluate previously generated predictions
sushitrack project test --eval-only

# Export results
sushitrack project test --compare --export ./results
```

The binary must exist before running `project test`. Build it first with `sushitrack project build --test`. The `--eval-only` flag skips the binary execution and works on whatever `pred/*.txt` files are already on disk.

---

### `project clean`

Removes `build/` and `test/build/` directories.

```bash
sushitrack project clean
```

---

### `docker build`

Builds the `sushitrack` Docker image from the Dockerfile at the project root.

```bash
sushitrack docker build
```

---

### `docker run`

Starts a container from the `sushitrack` image with the project root bind-mounted.

```bash
sushitrack docker run
```

---

## C API

Include `SushiTrackC.h`. Link against the compiled shared or static library.

```c
#include "SushiTrackC.h"

// 1. Get defaults and optionally override them
SushiTrack_Params params;
SushiTrack_GetDefaultParams(&params);
params.enable_fsm       = 1;
params.enable_tentative = 1;
params.frame_rate       = 25;

// 2. Create tracker
SushiTrackHdl hdl = SushiTrack_Create(&params);

// Per-frame loop
SushiTrack_Object dets[MAX_DETS];
SushiTrack_Track  tracks[MAX_TRACKS];

// ... populate dets[] ...

int n = SushiTrack_Update(hdl, dets, n_dets, tracks, MAX_TRACKS, dt);

// 3. Destroy when done
SushiTrack_Destroy(hdl);
```

`SushiTrack_CreateEx` returns a `SushiTrack_Status` code instead of checking for NULL, which is preferable when you want to distinguish between failure reasons.

```c
SushiTrackHdl hdl;
SushiTrack_Status s = SushiTrack_CreateEx(&params, &hdl);
if (s != SUSHITRACK_SUCCESS) { /* handle error */ }
```

**Status codes**

| Code | Value | Meaning |
|---|---|---|
| `SUSHITRACK_SUCCESS` | 0 | Operation completed |
| `SUSHITRACK_INVALID_ARGUMENT` | -1 | Parameter validation failed |
| `SUSHITRACK_INITIALIZATION_FAILURE` | -2 | Internal setup failed |
| `SUSHITRACK_OUT_OF_MEMORY` | -3 | Allocation failed |
| `SUSHITRACK_RUNTIME_ERROR` | -4 | Unhandled exception |
| `SUSHITRACK_BUFFER_OVERFLOW` | -5 | Output buffer too small |

**Optional log callback**

```c
void my_logger(SushiTrack_LogLevel level, const char* msg) {
    printf("[%d] %s\n", level, msg);
}

SushiTrack_SetLogCallback(my_logger);
```

Pass `NULL` to unregister. The callback is stored atomically so it is safe to set or clear from any thread at any time.

---

## Data Layout

Place MOT sequences under `data/` at the project root. Each sequence directory must contain at minimum a detection file and a ground-truth file for evaluation.

```
data/
├── config.json
├── MOT17-02-DPM/
│   ├── det/
│   │   └── det.txt
│   ├── gt/
│   │   └── gt.txt
│   └── seqinfo.ini
├── MOT17-04-DPM/
│   └── ...
└── MOT20-01/
    └── ...
```

`det.txt` and `gt.txt` follow the standard MOT Challenge CSV format:

```
<frame>, <id>, <x>, <y>, <w>, <h>, <score>, <class>, ...
```

`seqinfo.ini` is the standard MOT sequence metadata file. The runner reads `frameRate` from it to calibrate the tracker and pass the correct `dt` per frame. If the file is absent, the runner defaults to 30 fps.

Prediction files are written automatically to `<sequence>/pred/<tracker>.txt` during a test run.

The sequences currently registered in the benchmark harness are the full MOT17 train set (21 sequences across DPM, FRCNN, and SDP detectors) and MOT20-01, 02, 03, 05.

---

## Configuration

Tracker parameters can be overridden at runtime through `data/config.json`. The file contains one section per tracker. Any key absent from the file falls back to the compiled default.

```json
{
  "sushitrack": {
    "track_thresh": 0.5,
    "high_thresh": 0.6,
    "match_thresh": 0.8,
    "frame_rate": 30,
    "track_buffer": 30,
    "enable_reid": 0,
    "enable_fsm": 0,
    "enable_tentative": 0,
    "enable_mahalanobis": 0,
    "enable_diou": 1,
    "enable_xyah": 0,
    "iou_plus_one": 1,
    "time_aware": 0,

    "reid_high_thresh": 0.3,
    "reid_high_iou_weight": 0.3,
    "reid_low_thresh": 0.5,
    "reid_low_iou_weight": 0.2,
    "reid_recovery_thresh": 0.4
  },
  "bytetrack": {
    "track_thresh": 0.5,
    "high_thresh": 0.6,
    "match_thresh": 0.8,
    "frame_rate": 30,
    "track_buffer": 30
  }
}
```

**Detection thresholds**

| Key | Default | Description |
|---|---|---|
| `track_thresh` | 0.5 | Splits detections into high-confidence (`score >= track_thresh`) and low-confidence (`score < track_thresh`); high-confidence detections participate in the primary association pass |
| `high_thresh` | 0.6 | Minimum score to initialize a new track; detections in `[track_thresh, high_thresh)` can match existing tracks but cannot seed new ones |

**Association cost limits**

| Key | Default | Description |
|---|---|---|
| `match_thresh` | 0.8 | Maximum cost for the primary association pass (high-confidence detections against confirmed tracks) |
| `iou_match_thresh` | 0.5 | Maximum cost for the secondary association pass (low-confidence detections against remaining tracks) |
| `unconfirmed_iou_thresh` | 0.7 | Maximum cost for matching unconfirmed tracks; applies when `enable_tentative = 0` |
| `duplicate_iou_thresh` | 0.15 | Cost threshold for duplicate suppression; two tracks are considered duplicates when their association cost falls below this value (cost = 1 − IoU, so 0.15 corresponds to IoU > 0.85) |

**Track lifecycle**

| Key | Default | Description |
|---|---|---|
| `frame_rate` | 30 | Target processing frequency; used to scale noise and buffer duration |
| `track_buffer` | 30 | Frames a lost track is kept before removal |
| `enable_tentative` | 0 | Enables intermediate tentative state before track confirmation |
| `tentative_confirm_hits` | 1 | Consecutive matches required to confirm a tentative track |
| `tentative_max_age` | 2 | Frames a tentative track can go unmatched before removal |


**Motion model**

| Key | Default | Description |
|---|---|---|
| `std_weight_position` | 0.05 | Process noise baseline for position |
| `std_weight_velocity` | 0.00625 | Process noise baseline for velocity |
| `kf_pos_weight` | 2.0 | Scales initial position uncertainty |
| `kf_size_weight` | 2.0 | Scales initial size uncertainty |
| `kf_vel_weight` | 10.0 | Scales initial velocity uncertainty |
| `enable_xyah` | 0 | Use centroid + aspect ratio + height state representation instead of corner + width + height |

**Association strategy**

| Key | Default | Description |
|---|---|---|
| `enable_mahalanobis` | 0 | Apply Mahalanobis distance gating before building cost matrices |
| `gating_thresh` | 10.0 | Chi-squared threshold for Mahalanobis gate |
| `enable_diou` | 1 | Use DIoU (penalizes center distance) instead of standard IoU |
| `iou_plus_one` | 1 | Use `1 - iou` as the cost value; set to 0 for `1 / (1 + iou)` |
| `max_area_ratio` | 50.0 | Candidate pairs with area ratio above this are suppressed |
| `motion_cost_weight` | 0.05 | Weight of motion penalty in fused cost calculations |

**ReID / appearance**

| Key | Default | Description |
|---|---|---|
| `enable_reid` | 0 | Enable visual feature matching |
| `reid_high_thresh` | 0.3 | Similarity threshold for direct association in the high-confidence ReIDFirst pass or Fused mode |
| `reid_high_iou_weight` | 0.3 | IoU fallback weight in the high-confidence ReIDFirst pass or Fused mode |
| `reid_low_thresh` | 0.5 | Similarity threshold for direct association in the low-confidence ReIDFirst pass or Fused mode |
| `reid_low_iou_weight` | 0.2 | IoU fallback weight in the low-confidence ReIDFirst pass or Fused mode |
| `reid_recovery_thresh` | 0.4 | Similarity threshold for PureReID lost-track visual recovery; active in all ReID paths |

**Kinematic FSM**

| Key | Default | Description |
|---|---|---|
| `enable_fsm` | 0 | Enable per-track kinematic state machine |
| `fsm_stationary_vel_thresh` | 30.0 | Velocity (units/s) below which a track is classified as stationary |
| `fsm_maneuver_accel_thresh` | 150.0 | Acceleration (units/s²) above which a track is classified as maneuvering |

**Timing**

| Key | Default | Description |
|---|---|---|
| `time_aware` | 0 | When 1, treat `dt` as real seconds and normalize to frame steps based on `frame_rate` |

---

## Benchmark Results

Evaluated on MOT17 and MOT20 train sets using public detections. SushiTrack is compared against the ByteTrack reference implementation at matched default parameters.

**MOT17**

| Tracker | HOTA | DetA | AssA | MOTA | IDF1 | IDSW | FP | FN | Lat (µs/frame) |
|---|---|---|---|---|---|---|---|---|---|
| SushiTrack | 41.5% | 40.8% | 43.4% | 46.7% | 50.2% | 1734 | 6448 | 168570 | 46 |
| ByteTrack | 39.1% | 40.9% | 38.7% | 46.5% | 47.1% | 2763 | 8827 | 165486 | 71 |

**MOT20**

| Tracker | HOTA | DetA | AssA | MOTA | IDF1 | IDSW | FP | FN | Lat (µs/frame) |
|---|---|---|---|---|---|---|---|---|---|
| SushiTrack | 24.9% | 16.9% | 42.2% | 17.7% | 24.2% | 189 | 235 | 1037435 | 67 |
| ByteTrack | 23.0% | 15.6% | 40.5% | 16.6% | 21.6% | 200 | 243 | 1045728 | 124 |

**Overall**

| Tracker | HOTA | DetA | AssA | MOTA | IDF1 | IDSW | FP | FN | Lat (µs/frame) |
|---|---|---|---|---|---|---|---|---|---|
| SushiTrack | 38.8% | 37.0% | 43.2% | 42.0% | 46.0% | 1923 | 6683 | 1206005 | 50 |
| ByteTrack | 36.5% | 36.8% | 39.0% | 41.7% | 43.0% | 2963 | 9070 | 1211214 | 80 |

Metrics computed with [TrackEval](https://github.com/JonathonLuiten/TrackEval). Latency measured as mean per-frame tracker update time, excluding I/O. Both trackers run single-threaded.

---

## License

Copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.

This software and all associated source files are proprietary. No part of this codebase may be copied, modified, distributed, sublicensed, or used in any form without the prior written consent of Mustafa Garip & Sushi Systems