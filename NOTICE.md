# Notices

SushiTrack is Copyright (c) 2026 Sushi Systems and licensed under the PolyForm Noncommercial
License 1.0.0; see `LICENSE`. The material below keeps its own licence.

## Third-party components

| Component | Where | Source | Version | Licence |
| --- | --- | --- | --- | --- |
| ByteTrack-cpp | `third_party/bytetrack/` | https://github.com/Vertical-Beach/ByteTrack-cpp, a C++ port of https://github.com/ifzhang/ByteTrack | unverified; the tree records no commit | MIT, Copyright (c) 2022 Vertical Beach (lp6m, medalotte), Copyright (c) 2021 Yifu Zhang; text in `third_party/bytetrack/LICENSE` |
| OC_SORT_CPP | `third_party/ocsort/` | https://github.com/Postroggy/OC_SORT_CPP, a C++ port of https://github.com/noahcao/OC_SORT | unverified; the tree records no commit | MIT, Copyright (c) 2023 Yueqi Guo; text in `third_party/ocsort/LICENSE` |
| gatagat/lap | inside `third_party/bytetrack/src/lapjv.cpp` and `third_party/ocsort/src/lapjv.cpp` | https://github.com/gatagat/lap | unverified; both ports copied it without a version | BSD-2-Clause, Copyright (c) 2012-2025, Tomas Kazmar; text in `third_party/licenses/LAP_BSD_2_CLAUSE.txt` |
| JSON for Modern C++ | `third_party/nlohmann/json.hpp` | https://github.com/nlohmann/json | 3.11.3 | MIT, Copyright (c) 2013-2023 Niels Lohmann; text in `third_party/licenses/NLOHMANN_JSON_MIT.txt` |
| Eigen | `third_party/eigen/` (git submodule) | https://gitlab.com/libeigen/eigen | commit `52f99a14` | MPL-2.0 for the library; some files are Apache-2.0, BSD-3-Clause or MINPACK; texts in `third_party/eigen/LICENSES/` |
| TrackEval | `third_party/TrackEval/` (git submodule) | https://github.com/JonathonLuiten/TrackEval | commit `12c8791b` | MIT, Copyright (c) 2020 Jonathon Luiten; text in `third_party/TrackEval/LICENSE` |
| YOLOX | `third_party/yolox/` (git submodule) | https://github.com/Megvii-BaseDetection/YOLOX | commit `6ddff482`, 38 commits after 0.3.0 | Apache-2.0, Copyright (c) 2021-2022 Megvii Inc. All rights reserved.; text in `third_party/yolox/LICENSE` and `third_party/licenses/YOLOX_APACHE_2_0.txt` |

The library links Eigen and JSON for Modern C++. ByteTrack-cpp and OC_SORT_CPP are built only
as reference trackers for the regression harness. TrackEval and YOLOX are used only by the
Python scripts under `tests/regression/`.

## Fetched at build or install time

Nothing in this table is stored in the repository.

| Component | Declared in | Version | Licence |
| --- | --- | --- | --- |
| GoogleTest | `cli/sushistack.deps.toml`, `environment.yml` | 1.17.0 in `environment.yml`; unpinned in the fragment | BSD-3-Clause |
| typer, rich, tomli, pytest | `cli/pyproject.toml` | lower bounds only | MIT |
| sushicore | `cli/pyproject.toml`, `environment.yml` | `>=0.7.0` and `==0.4.0` | A Sushi Systems package under its own licence |
| PyTorch, torchvision, torchaudio | `environment.yml`, `Dockerfile` | 2.5.1, 0.20.1 and 2.5.1 in `Dockerfile`; unpinned in `environment.yml` | unverified |
| `pytorch-cuda` 12.1 and the `nvidia/cuda:12.1.1-devel-ubuntu22.04` base image | `environment.yml`, `Dockerfile` | as named | NVIDIA's own terms; unverified |
| The other conda and pip packages | `environment.yml` | pinned there | unverified, not read one by one |

A Docker image built from `Dockerfile` holds NVIDIA's CUDA libraries and falls under NVIDIA's
terms. No built image is published.

## Ported files

Each file below holds code translated or adapted from the named project. The Sushi Systems
licence covers the changes; the original stays under its own licence, and each file names it
in its license block.

| File | Upstream | Upstream counterpart | Licence |
| --- | --- | --- | --- |
| `source/rect.cpp`, `include/SushiTrack/rect.hpp` | ByteTrack-cpp | `src/Rect.cpp`, `include/ByteTrack/Rect.h` | MIT |
| `source/object.cpp`, `include/SushiTrack/object.hpp` | ByteTrack-cpp | `src/Object.cpp`, `include/ByteTrack/Object.h` | MIT |
| `source/kalman_filter.cpp`, `include/SushiTrack/kalman_filter.hpp`, `include/SushiTrack/kalman_types.hpp` | ByteTrack-cpp | `src/KalmanFilter.cpp`, `include/ByteTrack/KalmanFilter.h` | MIT |
| `source/tracklet.cpp`, `include/SushiTrack/tracklet.hpp` | ByteTrack-cpp | `src/STrack.cpp`, `include/ByteTrack/STrack.h` | MIT |
| `source/tracker.cpp`, `include/SushiTrack/tracker.hpp` | ByteTrack-cpp | `src/BYTETracker.cpp`, `include/ByteTrack/BYTETracker.h` | MIT |
| `source/assignment_solvers.cpp`, `include/SushiTrack/assignment_solvers.hpp` | ByteTrack-cpp | `linearAssignment` and `execLapjv` in `src/BYTETracker.cpp` | MIT |
| `source/lapjv.cpp`, `include/SushiTrack/lapjv.hpp` | ByteTrack-cpp, which took the code from gatagat/lap | `src/lapjv.cpp`, `include/ByteTrack/lapjv.h` | MIT and BSD-2-Clause |
| `tests/regression/inference/pipeline.py` | YOLOX | `preproc` in `yolox/data/data_augment.py`; the inference steps of `Predictor` in `tools/demo.py` | Apache-2.0 |

Upstream copyright lines: ByteTrack-cpp, Copyright (c) 2022 Vertical Beach (lp6m, medalotte)
and Copyright (c) 2021 Yifu Zhang; gatagat/lap, Copyright (c) 2012-2025, Tomas Kazmar; YOLOX,
Copyright (c) 2021-2022 Megvii Inc. All rights reserved.

The other files under `source/`, `include/SushiTrack/` and `tests/regression/inference/` were
read against `third_party/bytetrack`, `third_party/ocsort` and `third_party/yolox` and are not
ports. The observation-centric momentum and re-update code in `source/tracklet.cpp` and
`source/distance_strategies.cpp` follows the OC-SORT method and shares no code with OC_SORT_CPP.

This command writes every license block in the repository, the upstream lines included:

```bash
BT="Holds code ported from ByteTrack-cpp (MIT), see NOTICE.md;Copyright (c) 2022 Vertical Beach (lp6m, medalotte);Copyright (c) 2021 Yifu Zhang"
LAP="$BT;Holds code ported from gatagat/lap (BSD-2-Clause), see NOTICE.md;Copyright (c) 2012-2025, Tomas Kazmar"
YX="Holds code ported from YOLOX (Apache-2.0), see NOTICE.md;Copyright (c) 2021-2022 Megvii Inc. All rights reserved."
python D:/Projects/sushiskills/tools/licensing/write_license_block.py D:/Projects/sushitrack \
    --project "SushiTrack - https://github.com/SushiSystems/SushiTrack" \
    --upstream "source/rect.cpp=$BT" \
    --upstream "include/SushiTrack/rect.hpp=$BT" \
    --upstream "source/object.cpp=$BT" \
    --upstream "include/SushiTrack/object.hpp=$BT" \
    --upstream "source/kalman_filter.cpp=$BT" \
    --upstream "include/SushiTrack/kalman_filter.hpp=$BT" \
    --upstream "include/SushiTrack/kalman_types.hpp=$BT" \
    --upstream "source/tracklet.cpp=$BT" \
    --upstream "include/SushiTrack/tracklet.hpp=$BT" \
    --upstream "source/tracker.cpp=$BT" \
    --upstream "include/SushiTrack/tracker.hpp=$BT" \
    --upstream "source/assignment_solvers.cpp=$BT" \
    --upstream "include/SushiTrack/assignment_solvers.hpp=$BT" \
    --upstream "source/lapjv.cpp=$LAP" \
    --upstream "include/SushiTrack/lapjv.hpp=$LAP" \
    --upstream "tests/regression/inference/pipeline.py=$YX"
```

## Redistributed binaries

The repository tracks no binary. `st deploy` builds a package from the public headers, the
compiled library, the Python binding and `sushitrack.json`; the library in it contains JSON
for Modern C++, Eigen and the ported files above. `st deploy` copies no licence file, so
`LICENSE`, this file and `third_party/licenses/` are added by hand before a package leaves the
machine.

`third_party/weight/` (YOLOX checkpoints) and `third_party/video/` (MOT17 and MOT20 clips) are
ignored by git and are not distributed. Their terms are unverified.

## Third-party data

`tests/regression/evaluator/data/` holds 228 tracked files for 76 sequences: `gt/gt.txt`,
`det/det.txt` and `seqinfo.ini` each. No image is tracked. The repository's licence does not
cover these files.

| Data set | Sequences | Source | Licence |
| --- | --- | --- | --- |
| MOT17 | 28 folders `MOT17-*`: seven train sequences, each with the DPM, FRCNN, SDP and YOLOX detection sets | https://motchallenge.net/data/MOT17/ | unverified; the archived site states no licence, and the audit names CC BY-NC-SA 3.0 |
| MOT20 | 8 folders `MOT20-*`: four train sequences, each with the public and the YOLOX detection set | https://motchallenge.net/data/MOT20/ | unverified; the archived site states no licence, and the audit names CC BY-NC-SA 3.0 |
| DanceTrack | 40 folders `dancetrack*` | https://github.com/DanceTrack/DanceTrack | Annotations: CC BY 4.0. The data set is "available for non-commercial research purposes only" |

Where the `det/det.txt` files of the YOLOX sets and of DanceTrack come from is unverified.
