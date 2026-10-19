# Project structure

The repository holds one module: public headers under `include/SushiTrack/`, sources under
`source/`. Files that git ignores are marked.

```
sushitrack/
├── README.md  LICENSE  NOTICE.md  COMMERCIAL.md
├── AGENTS.md  CLAUDE.md               # agent instructions
├── CMakeLists.txt                     # library, vendored trackers, tests
│
├── include/SushiTrack/
│   ├── sushitrack_c.h                 # C API
│   ├── sushitrack_export.hpp          # SUSHITRACK_API export macro
│   ├── types.hpp                      # status codes, log levels, type aliases
│   ├── tracker.hpp                    # core orchestrator
│   ├── tracker_config.hpp             # C++ config struct, mirror of sushitrack_params_t
│   ├── config_loader.hpp              # sushitrack.json loading
│   ├── tracklet.hpp                   # per-track state, lifecycle, feature history
│   ├── object.hpp                     # detection input
│   ├── rect.hpp                       # bounding-box template
│   ├── kalman_filter.hpp              # linear Kalman filter
│   ├── nsa_kalman_filter.hpp          # noise-scale-adaptive variant
│   ├── imm_filter.hpp                 # IMM multi-model estimator
│   ├── kalman_filter_registry.hpp     # plug-in registry
│   ├── kalman_types.hpp               # Eigen vector and matrix types
│   ├── kinematic_state.hpp            # motion-regime enum
│   ├── i_kalman_filter.hpp            # IStatePredictor + IGateValidator
│   ├── i_kalman_factory.hpp           # predictor factory interface
│   ├── i_state_predictor.hpp          # predict/update interface
│   ├── i_gate_validator.hpp           # gating interface and NullGateValidator
│   ├── i_cost_calculator.hpp          # cost-matrix interface
│   ├── i_assignment_solver.hpp        # solver interface
│   ├── i_timing_policy.hpp            # timing interface
│   ├── distance_strategies.hpp        # every cost calculator
│   ├── assignment_solvers.hpp         # LAPJVSolver
│   ├── lapjv.hpp                      # Jonker-Volgenant routine
│   ├── cost_matrix.hpp                # contiguous 2-D cost storage
│   ├── timing_policies.hpp            # frame and wall-clock policies
│   └── logger.hpp                     # log macros and callback dispatch
│
├── source/                            # twelve .cpp files, named after their headers
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
