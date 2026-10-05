# Building

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

Each test target registers with CTest under one label (`unit`,
`integration` or `regression`), set on its `gtest_discover_tests` call, and
`enable_testing()` at the top level makes the root `CTestTestfile.cmake` recurse
into the test directories. So `ctest` works from the build root:

```bash
ctest --test-dir build -L '^unit$' --output-on-failure     # what `st test --suite unit` runs
ctest --test-dir build --print-labels
```

Through the CLI (preferred). Test targets are built **by default**; there is no
per-suite build flag. What runs is selected at test time by CTest label:

```bash
st build                      # library + every test target (default)
st build --no-test            # library only
st build --type debug         # release (default) | debug | relwithdebinfo | minsizerel
st build --clean              # wipe build/ first; combinable with the above
st build -j 8                 # parallel jobs (default: CPU count)
st build -D CMAKE_UNITY_BUILD=ON   # extra -D passed straight to cmake
st build --deploy cpp         # build, then assemble package/ for C/C++ consumers
st build --deploy python      # build, then assemble package/ for Python consumers
```

cmake configure runs only when the requested configuration (build type,
tests on/off, generator, extra defines) differs from what the tree was configured
with, which `build/.sushitrack_config` records. A configuration change reconfigures
from scratch, because a single-config generator bakes the build type into its
cache and would otherwise silently hand back the previous one.

Artefacts: `build/bin/{Release,Debug}/sushitrack.{dll,so}`, `build/bin/.../unit_test{,.exe}`, `build/bin/.../regression_test{,.exe}`, `build/bin/.../integration_test{,.exe}`, `build/bin/.../tracker_bridge.{dll,so}`.

`--deploy` additionally assembles a self-contained, redistributable `package/`
directory at the repository root; see [Deploy packages](../guides/DEPLOY_PACKAGES.md).
