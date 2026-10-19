# Project structure

The repository holds one module: public headers under `include/SushiTrack/`, sources under
`source/`. Files that git ignores are marked.

```
sushitrack/
├── README.md  LICENSE  NOTICE.md  COMMERCIAL.md
├── AGENTS.md  CLAUDE.md               # agent instructions
├── CMakeLists.txt                     # project(), options, add_subdirectory
├── .clang-format  .editorconfig       # formatting the editors and clang-format find
├── .config/                           # Dockerfile and conda environment; see .config/README.md
├── assets/sushitrack.json             # default tracker configuration the package ships
├── cmake/                             # build policy and the Eigen target; see BUILD_SYSTEM.md
│
├── include/SushiTrack/                # one primary type per header, grouped by area
│   ├── association/                   # cost matrix, cost calculators and decorators, LAPJV solver
│   ├── c_api/                         # the C API, its type aliases and the export macro
│   ├── configuration/                 # sushitrack.json loading
│   ├── detection/                     # the detection input and the bounding-box template
│   ├── filtering/                     # Kalman, NSA and IMM estimators, gating, their registry
│   ├── logging/                       # log macros and callback dispatch
│   └── tracking/                      # the tracker, tracklets, their states and timing policies
│
├── source/                            # one .cpp per header, under the same area folders
│
├── bindings/python/                   # ctypes binding; see its README.md
│
├── third_party/
│   ├── bytetrack/  ocsort/            # reference trackers for the MOT benchmark
│   ├── nlohmann/                      # JSON for Modern C++
│   ├── licenses/                      # licence texts NOTICE.md cites
│   ├── eigen/  TrackEval/  yolox/     # git submodules
│   └── weight/  video/                # model weights and sample clips, ignored
│
├── tests/
│   ├── CMakeLists.txt                 # sushitrack_tests, by size label
│   ├── README.md  *.hpp               # the helpers the tests share
│   ├── <area>/test_<name>.cpp         # one file per public header, or a folder per unit
│   └── applications/                  # the tests of applications/, by the same mirror
│
├── applications/
│   ├── tracker_bridge/                # the tracker adapters and the C library ctypes loads
│   ├── benchmarks/mot_evaluation/     # the MOT benchmark, its evaluator/ and data/
│   └── inference/                     # the YOLOX video pipeline behind st infer
│
├── cli/                               # the st CLI; see CLI_PACKAGE.md
│   ├── pyproject.toml  config.toml  sushistack.deps.toml
│   ├── sushitrack_cli/
│   ├── tests/                         # pytest suite for the CLI
│   └── misc/                          # clean_preds.sh, clean_preds.bat, construct_trackeval_data.ps1
│
├── tools/                             # checkers; see tools/README.md
└── docs/                              # this manual; see docs/README.md
```

`cli/config.local.toml` holds per-machine overrides and is ignored. `build/` and `package/` are
build output and are ignored.

The files of the CLI package are listed in [The `st` package](CLI_PACKAGE.md). The test files
are described in [Test suites](../guides/TESTING.md).
