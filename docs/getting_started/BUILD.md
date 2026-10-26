# Building

`st build` configures and builds the library and, by default, the test programs. Builds go
through `st` only: it hands CMake the pinned SushiLog and sets the paths the programs need at run
time.

```bash
st build                      # library + test programs (default)
st build --no-test            # library only
st build --type debug         # release (default) | debug | relwithdebinfo | minsizerel
st build --clean              # wipe build/ first; combinable with the above
st build -j 4                 # parallel jobs (default: CPU count)
st build -D CMAKE_UNITY_BUILD=ON   # extra -D passed straight to cmake
st build --deploy cpp         # build, then assemble package/ for C/C++ consumers
st build --deploy python      # build, then assemble package/ for Python consumers
```

The CMake options `st build` sets:

| Option | Default | Effect |
|---|---|---|
| `SUSHITRACK_BUILD_TESTS` | ON | Builds `sushitrack_tests` from the mirror under `tests/`; `--no-test` turns it off |
| `SUSHITRACK_BUILD_APPLICATIONS` | ON | Builds `tracker_bridge` and the `mot_engine` benchmark under `applications/` |
| `SUSHITRACK_ENABLE_LOGGING` | ON | Compiles log calls below ERROR; OFF keeps ERROR and FATAL only |
| `SUSHITRACK_ENABLE_PROFILER` | ON | Records zones, counters and instants through SushiProfiler; OFF compiles the macros out and builds without SushiProfiler |
| `SUSHITRACK_PROFILE_LEVEL` | empty: 1, and 2 in a Debug configuration | The most detailed profiling level compiled in: 0, 1 or 2 |
| `SushiProfiler_DIR` | set by `st` | The config folder of the SushiProfiler package `cli/sushistack.deps.toml` pins |
| `SushiLog_DIR` | set by `st` | The config folder of the SushiLog package `cli/sushistack.deps.toml` pins |

`st` builds each package once through its library's own CLI and reuses it afterwards. CMake
finds both with `find_package(... CONFIG REQUIRED)` and compiles none of their sources.

cmake configure runs only when the requested configuration (build type, tests on/off,
generator, extra defines, the SushiLog path) differs from what the tree was configured with,
which `build/.sushitrack_config` records. A configuration change reconfigures from scratch,
because a single-config generator bakes the build type into its cache and would otherwise
silently hand back the previous one.

Artefacts land in `build/bin`: `sushitrack.{dll,so}`, `sushilog.{dll,so}`,
`sushitrack_tests{,.exe}`, `sushitrack_application_tests{,.exe}`, `mot_engine{,.exe}` and
`tracker_bridge.{dll,so}`. How the tests
run is in [Test suites](../guides/TESTING.md); how the tree is laid out is in
[Build system](../architecture/BUILD_SYSTEM.md).

`--deploy` additionally assembles a self-contained, redistributable `package/`
directory at the repository root; see [Deploy packages](../guides/DEPLOY_PACKAGES.md).
