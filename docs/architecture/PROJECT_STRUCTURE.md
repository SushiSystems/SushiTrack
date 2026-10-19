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
│   ├── bytetrack/  ocsort/            # reference trackers for the regression harness
│   ├── nlohmann/                      # JSON for Modern C++
│   ├── licenses/                      # licence texts NOTICE.md cites
│   ├── eigen/  TrackEval/  yolox/     # git submodules
│   └── weight/  video/                # model weights and sample clips, ignored
│
├── tests/
│   ├── CMakeLists.txt                 # BUILD_UNIT_TEST, BUILD_REGRESSION_TEST, BUILD_INTEGRATION_TEST
│   ├── bytetrack.json  ocsort.json    # reference tracker configs
│   ├── unit/                          # unit_test: UnitTest.cpp and seventeen Test*.cpp files
│   ├── integration/                   # integration_test: Main.cpp, Helpers.h, ten Test*.cpp files
│   └── regression/
│       ├── RegressionTest.cpp         # GTest entry over (scenario x tracker)
│       ├── ScenarioList.h             # MOT17, MOT20 and DanceTrack scenarios
│       ├── ITrackerAdapter.h          # adapter interface
│       ├── SushiTrackAdapter.h  ByteTrackAdapter.h  OCSORTAdapter.h
│       ├── TrackerRegistry.h  TrackerRegistry.cpp   # C entry points of tracker_bridge
│       ├── MOTRunner.h                # det.txt reader and prediction writer
│       ├── TestInfra.h                # per-frame timing; prints the "[ PERF ]" lines
│       ├── evaluator/                 # TrackEval runner, report writer, data/ sequences
│       └── inference/                 # demo.py, pipeline.py, tracker_bridge.py
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
