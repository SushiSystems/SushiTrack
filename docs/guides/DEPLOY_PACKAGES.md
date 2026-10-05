# Deploy packages

`st build --deploy {cpp,python}` builds the library and then
assembles a **self-contained, copy-and-run package** under `package/` at the
repository root. The package bundles what a downstream project needs (the
compiled library, the headers or binding, a default config, and a working
example), so it can be lifted out of this repository and dropped into another
project without a build tree or environment variables. The directory is
regenerated from scratch on every deploy and is git-ignored.

The deploy is **platform-specific**: it copies the artefacts for the OS it runs
on. Run it on Windows to get a Windows package (`.dll` + `.lib`), and on Linux to
get a Linux package (`.so`). There is no cross-compilation: to ship both
platforms, run the deploy once on each.

```bash
st build --deploy cpp        # C/C++ package
st build --deploy python     # Python package
st build --clean --deploy cpp   # clean rebuild, then deploy
```

Deploy reuses the same artefacts a plain `st build` produces; it never needs
a test target. If the shared library (or, on Windows, its import library) is
missing, the deploy fails with an explicit message telling you to build first.

## `--deploy cpp` layout

Targets a C or C++ consumer through the **C ABI** (`SushiTrack/sushitrack_c.h`). That header
includes `SushiTrack/sushitrack_export.hpp`, which holds preprocessor macros only, and nothing
else from the library, so a C consumer needs no Eigen and no C++ header. The package still
carries every header of `include/SushiTrack/`.

`sushitrack_c.h` includes that file and does not repeat the macro, so `SUSHITRACK_API` is defined
in exactly one place. A second copy could drift from the one `sushitrack_export.hpp` carries.

```
package/
├── include/
│   └── SushiTrack/                   # every public header; a C consumer includes SushiTrack/sushitrack_c.h
├── lib/
│   └── sushitrack.lib                # Windows import library (Linux: libsushitrack.so)
├── bin/
│   └── sushitrack.dll                # Windows runtime library (Linux: omitted; the .so in lib/ is both)
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
include `SushiTrack/sushitrack_c.h`, link against `package/lib/sushitrack.lib` (Windows) or
`package/lib/libsushitrack.so` (Linux), and ensure the runtime library is
resolvable at load time (DLL on `PATH`/beside the exe on Windows; RPATH or
`LD_LIBRARY_PATH` on Linux). Create the tracker with
`sushitrack_create_from_config("sushitrack.json")` to pick up the bundled config, and
edit that JSON to retune the algorithm without recompiling.

## `--deploy python` layout

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
from sushitrack import Tracker, TrackState
tracker = Tracker(frame_rate=30, kalman_type=2)
tracks  = tracker.update([(x, y, w, h, score, label)], dt=1/30)
```

The binding looks beside its own module for the library before anywhere else, which is why the
package needs no configuration. The full search order is in
[`bindings/python/README.md`](../../bindings/python/README.md#finding-the-library).
