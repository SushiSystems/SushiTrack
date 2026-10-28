# Building

`st build` configures and builds the library and, by default, the test programs. Builds go
through `st` only: it hands CMake the pinned SushiLog and sets the paths the programs need at run
time.

```bash
st build                      # library + test programs (default)
st build --no-test            # library only
st build --type release       # debug (default) | release | relwithdebinfo | minsizerel
st build --clean              # wipe the tree first; combinable with the above
st build -j 4                 # parallel jobs (default: the hardware threads)
st build --report             # time and peak memory per kind of edge
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
| `SUSHITRACK_PROFILE_LEVEL` | 1 | The most detailed profiling level compiled in: 0, 1 or 2, written to a generated header |
| `SushiProfiler_DIR` | set by `st` | The config folder of the SushiProfiler package `cli/sushistack.deps.toml` pins |
| `SushiLog_DIR` | set by `st` | The config folder of the SushiLog package `cli/sushistack.deps.toml` pins |

`st` builds each package once through its library's own CLI and reuses it afterwards. CMake
finds both with `find_package(... CONFIG REQUIRED)` and compiles none of their sources.

Each build type has its own tree, `build/<name>-<type>`: `build/default-debug`,
`build/default-release`, and `build/asan-debug` or `build/tsan-debug` for a sanitizer build.
The type is fixed when a tree is created, so switching type never recompiles a tree, and a
configure of another type into an existing tree is refused. Debug is the default; Release is
for releases and for measurements, so build it with `--type release`. `st test`, `st run`
and the other commands that read a tree use the tree of the configured build type, which is
Debug unless `[build] type` or `ST_BUILD_TYPE` says otherwise. `st clean` removes every tree.

cmake configure runs only when the settings (tests on/off, generator, extra defines, the
SushiLog path, the launcher) differ from what the tree was configured with, which
`.sushitrack_config` in the tree records.

Every compile and link runs through the memory gate of sushicore, which admits an edge only when
its expected peak fits in the free memory less a reserve. `st build --report` prints what the
build cost. Create a file named `stop` in `build/build_gate/` to make the gate refuse every
edge; delete it to build again.

Artefacts land in `bin` of the tree, for example `build/default-debug/bin`: `sushitrack.{dll,so}`, `sushilog.{dll,so}`,
`sushitrack_tests{,.exe}`, `sushitrack_application_tests{,.exe}`, `mot_engine{,.exe}` and
`tracker_bridge.{dll,so}`. How the tests
run is in [Test suites](../guides/TESTING.md); how the tree is laid out is in
[Build system](../architecture/BUILD_SYSTEM.md).

`--deploy` additionally assembles a self-contained, redistributable `package/`
directory at the repository root; see [Deploy packages](../guides/DEPLOY_PACKAGES.md).
