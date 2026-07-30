"""Assemble a ready-to-consume package under ``package/``.

Two flavours: ``cpp`` (headers + link/runtime library + a CMake example) and
``python`` (the ctypes binding with the native library bundled inside it). Both
are self-contained — the consumer needs neither this repo nor a build tree.
"""

import platform
import shutil

from .. import console
from ..config import IS_WINDOWS
from . import discovery

KINDS = ("cpp", "python")

_CPP_EXAMPLE = """\
/*
 * Minimal SushiTrack C-API usage example.
 *
 * Build (after `ds deploy cpp`):
 *   cd example && cmake -B build && cmake --build build
 *   ./build/sushitrack_example        (Windows: copy ../bin/sushitrack.dll next to the exe)
 */
#include <stdio.h>
#include "SushiTrackC.h"

int main(void)
{
    SushiTrack_Params params;
    SushiTrack_GetDefaultParams(&params);
    params.frame_rate = 30;

    SushiTrackHdl tracker = SushiTrack_Create(&params);
    if (!tracker) { fprintf(stderr, "failed to create tracker\\n"); return 1; }

    SushiTrack_Object dets[1] = { { 100.f, 100.f, 50.f, 80.f, 0.9f, 0, NULL, 0 } };
    SushiTrack_Track  out[64];

    for (int frame = 0; frame < 5; ++frame) {
        dets[0].x += 5.f;  /* move the box */
        int n = SushiTrack_Update(tracker, dets, 1, out, 64, 1.0f / 30.0f);
        printf("frame %d: %d track(s)\\n", frame, n);
        for (int i = 0; i < n; ++i)
            printf("  id=%lld  (%.1f, %.1f, %.1f, %.1f)  state=%d\\n",
                   (long long)out[i].track_id, out[i].x, out[i].y,
                   out[i].width, out[i].height, out[i].state);
    }

    SushiTrack_Destroy(tracker);
    return 0;
}
"""

_CPP_EXAMPLE_CMAKE = """\
cmake_minimum_required(VERSION 3.14)
project(sushitrack_example C)

# The package root is one directory up from this example.
set(SUSHITRACK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

add_executable(sushitrack_example example.c)
target_include_directories(sushitrack_example PRIVATE "${SUSHITRACK_ROOT}/include")

if(WIN32)
    target_link_libraries(sushitrack_example PRIVATE "${SUSHITRACK_ROOT}/lib/sushitrack.lib")
    # Copy the runtime DLL next to the executable so it can be launched directly.
    add_custom_command(TARGET sushitrack_example POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${SUSHITRACK_ROOT}/bin/sushitrack.dll" $<TARGET_FILE_DIR:sushitrack_example>)
else()
    target_link_libraries(sushitrack_example PRIVATE "${SUSHITRACK_ROOT}/lib/libsushitrack.so")
    set_target_properties(sushitrack_example PROPERTIES
        BUILD_RPATH   "${SUSHITRACK_ROOT}/lib"
        INSTALL_RPATH "${SUSHITRACK_ROOT}/lib")
endif()
"""

_CPP_README = """\
# SushiTrack — C/C++ Deployment Package

Self-contained, ready-to-link build of the SushiTrack tracker (C ABI).

## Contents
```
include/         Public C headers (SushiTrackC.h + SushiTrack/SushiTrackTypes.h)
lib/             Link library  ({import_lib})
bin/             Runtime library ({runtime_lib})   [Windows only]
sushitrack.json   Default runtime configuration (editable, no recompile needed)
example/         Minimal usage example + CMakeLists.txt
```

## Quick start
```sh
cd example
cmake -B build
cmake --build build
```

## Integrating into your own project
- Add `include/` to your include path and include `SushiTrackC.h`.
- Link against `lib/{import_lib}`.
- On Windows, ensure `bin/sushitrack.dll` is on the PATH or beside your executable.
- Edit `sushitrack.json` and create the tracker with `SushiTrack_CreateFromConfig("sushitrack.json")`
  to retune the algorithm without recompiling.

Platform: {platform}
"""

_PY_EXAMPLE = """\
\"\"\"Minimal SushiTrack Python usage example.

Run (after `ds deploy python`):
    python example.py
\"\"\"
from sushitrack import SushiTrack, TrackState

# The shared library is bundled inside the sushitrack/ package directory and
# discovered automatically.
tracker = SushiTrack(frame_rate=30, kalman_type=2)

x = 100.0
for frame in range(5):
    x += 5.0
    dets = [(x, 100.0, 50.0, 80.0, 0.9, 0)]   # (x, y, w, h, score, label)
    tracks = tracker.update(dets, dt=1.0 / 30.0)
    print(f"frame {frame}: {len(tracks)} track(s)")
    for t in tracks:
        print(f"  id={t.track_id}  ({t.x:.1f}, {t.y:.1f}, {t.width:.1f}, {t.height:.1f})"
              f"  state={TrackState(t.state).name}")

tracker.close()
"""

_PY_README = """\
# SushiTrack — Python Deployment Package

Self-contained ctypes binding bundled with the native SushiTrack library.

## Contents
```
sushitrack/
    __init__.py
    sushitrack.py     Pure-Python ctypes wrapper (no dependencies)
    {runtime_lib}    Bundled native library
sushitrack.json       Default runtime configuration
example.py           Minimal usage example
```

## Quick start
```sh
python example.py
```

## Integrating into your own project
Copy the `sushitrack/` directory into your project (or add it to PYTHONPATH):
```python
from sushitrack import SushiTrack
tracker = SushiTrack(frame_rate=30, kalman_type=2)
tracks = tracker.update([(x, y, w, h, score, label)], dt=1/30)
```
The native library is loaded from inside the package automatically; no build
tree or environment variable is required.

Platform: {platform}
"""


def _copy_config(paths, pkg_dir):
    config = paths.root / "sushitrack.json"
    if config.exists():
        shutil.copy2(config, pkg_dir / "sushitrack.json")


def _deploy_cpp(paths, pkg_dir):
    runtime = discovery.find_artifact(paths.build, discovery.shared_lib_name())
    if not runtime:
        console.error(f"{discovery.shared_lib_name()} not found. Run `ds build` first.")
        return False
    import_lib = discovery.find_artifact(paths.build, discovery.import_lib_name())
    if not import_lib:
        console.error(f"{discovery.import_lib_name()} not found. Run `ds build` first.")
        return False

    # The C ABI is dependency-free, so only these two headers ship.
    include_dst = pkg_dir / "include"
    (include_dst / "SushiTrack").mkdir(parents=True, exist_ok=True)
    shutil.copy2(paths.root / "include" / "SushiTrackC.h",
                 include_dst / "SushiTrackC.h")
    shutil.copy2(paths.root / "include" / "SushiTrack" / "SushiTrackTypes.h",
                 include_dst / "SushiTrack" / "SushiTrackTypes.h")

    (pkg_dir / "lib").mkdir(parents=True, exist_ok=True)
    shutil.copy2(import_lib, pkg_dir / "lib" / discovery.import_lib_name())
    if IS_WINDOWS:
        (pkg_dir / "bin").mkdir(parents=True, exist_ok=True)
        shutil.copy2(runtime, pkg_dir / "bin" / discovery.shared_lib_name())

    _copy_config(paths, pkg_dir)
    (pkg_dir / "example").mkdir(parents=True, exist_ok=True)
    (pkg_dir / "example" / "example.c").write_text(_CPP_EXAMPLE, encoding="utf-8")
    (pkg_dir / "example" / "CMakeLists.txt").write_text(_CPP_EXAMPLE_CMAKE,
                                                        encoding="utf-8")
    (pkg_dir / "README.md").write_text(
        _CPP_README.format(import_lib=discovery.import_lib_name(),
                           runtime_lib=discovery.shared_lib_name(),
                           platform=platform.system()), encoding="utf-8")
    return True


def _deploy_python(paths, pkg_dir):
    runtime = discovery.find_artifact(paths.build, discovery.shared_lib_name())
    if not runtime:
        console.error(f"{discovery.shared_lib_name()} not found. Run `ds build` first.")
        return False

    binding = paths.root / "bindings" / "python" / "sushitrack.py"
    if not binding.exists():
        console.error(f"Python binding not found at {binding}")
        return False

    module_dir = pkg_dir / "sushitrack"
    module_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(binding, module_dir / "sushitrack.py")
    # Bundled beside the module because the binding's find_library() looks there
    # first, which is what makes the package work with no env var set.
    shutil.copy2(runtime, module_dir / discovery.shared_lib_name())
    (module_dir / "__init__.py").write_text(
        "from .sushitrack import *  # noqa: F401,F403\n", encoding="utf-8")

    _copy_config(paths, pkg_dir)
    (pkg_dir / "example.py").write_text(_PY_EXAMPLE, encoding="utf-8")
    (pkg_dir / "README.md").write_text(
        _PY_README.format(runtime_lib=discovery.shared_lib_name(),
                          platform=platform.system()), encoding="utf-8")
    return True


def deploy(paths, kind):
    """Assemble ``package/`` for *kind* ("cpp" or "python"). Returns an exit code."""
    if kind not in KINDS:
        console.error(f"Unknown deploy kind '{kind}'. Choose: {', '.join(KINDS)}.")
        return 2

    pkg_dir = paths.package
    console.header(f"Deploy ({kind})")
    if pkg_dir.exists():
        console.info(f"Clearing existing package directory: {pkg_dir}")
        shutil.rmtree(pkg_dir, ignore_errors=True)
    pkg_dir.mkdir(parents=True, exist_ok=True)

    console.info(f"Assembling for {platform.system()} -> {pkg_dir}")
    ok = _deploy_cpp(paths, pkg_dir) if kind == "cpp" \
        else _deploy_python(paths, pkg_dir)
    if not ok:
        console.error("Deploy failed.")
        return 1
    console.success(f"Package ready at {pkg_dir}")
    return 0
