"""
@file      sushitrack.py
@brief     Command-line interface for the SushiTrack project pipeline.
@author    Mustafa Garip (mustafagarip@sushisystems.io)
@date      12.05.2026
@version   v2.0.0
@copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.

Command reference
-----------------
Build
  sushitrack project build               -- build library only
  sushitrack project build --unit        -- build library + unit_test
  sushitrack project build --regression  -- build library + regression_test + tracker_bridge
  sushitrack project build --integration -- build library + integration_test
  sushitrack project build --clean       -- wipe build dir first
  sushitrack project build --deploy cpp     -- build, then assemble package/ for C/C++ consumers
  sushitrack project build --deploy python  -- build, then assemble package/ for Python consumers

Test
  sushitrack project test                -- run all: unit → integration → label (regression) in sequence
  sushitrack project test unit           -- run unit_test (GTest deterministic)
  sushitrack project test integration    -- run integration_test (GTest end-to-end)
  sushitrack project test label          -- run regression_test on det.txt datasets + TrackEval
  sushitrack project test inference      -- run YOLOX video inference pipeline

Clean / Docker
  sushitrack project clean
  sushitrack docker build
  sushitrack docker run
"""

import os
import sys
import subprocess
import argparse
import platform
from pathlib import Path

from loguru import logger

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
_TOOLS_DIR    = Path(__file__).parent
_PROJECT_ROOT = _TOOLS_DIR.parent

sys.path.insert(0, str(_TOOLS_DIR))
from paths_manager import DIR_EVALUATOR, DIR_INFERENCE, DIR_DATA, DIR_BUILD, DIR_3RDPARTY

_EVAL_DIR     = DIR_EVALUATOR
_INFERENCE_DIR = DIR_INFERENCE

sys.path.insert(0, str(_EVAL_DIR))

try:
    from trackeval_runner import TrackEvalRunner
    from report_generator import print_trackeval_table, export_trackeval_results
    _EVAL_AVAILABLE = True
except ImportError:
    _EVAL_AVAILABLE = False

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------
_TRACKER_MAP = {
    "sushitrack": "SushiTrack",
    "bytetrack": "ByteTrack",
    "ocsort":    "OCSORT",
}
_KNOWN_TRACKERS = list(_TRACKER_MAP.keys())

_IS_WINDOWS = platform.system() == "Windows"


# ===========================================================================
# Path helpers
# ===========================================================================

def get_project_root() -> Path:
    """@brief Returns absolute path to project root."""
    return _PROJECT_ROOT


def get_data_root() -> Path:
    """@brief Returns path to the MOT data directory."""
    eval_data = DIR_EVALUATOR / "data"
    if eval_data.exists():
        return eval_data
    return DIR_DATA



# ===========================================================================
# Build
# ===========================================================================

def project_build(clean: bool, unit: bool, regression: bool, integration: bool) -> bool:
    """
    @brief Invokes the platform build script with the requested options.

    @param clean       Wipe the build directory before configuring.
    @param unit        Build the unit_test target.
    @param regression  Build the regression_test + tracker_bridge targets.
    @param integration Build the integration_test targets.
    @return True on success.
    """
    script = _TOOLS_DIR / "project" / ("build.bat" if _IS_WINDOWS else "build.sh")
    args: list[str] = []
    if clean:       args.append("--clean")
    if unit:        args.append("--unit")
    if regression:  args.append("--regression")
    if integration: args.append("--integration")

    mode_parts = []
    if unit:        mode_parts.append("unit")
    if regression:  mode_parts.append("regression")
    if integration: mode_parts.append("integration")
    if not mode_parts: mode_parts.append("library-only")
    logger.info(f"Building: {', '.join(mode_parts)}{' (clean)' if clean else ''}")

    try:
        subprocess.run([str(script)] + args, check=True, shell=_IS_WINDOWS)
        return True
    except subprocess.CalledProcessError as e:
        logger.error(f"Build failed (exit {e.returncode})")
        return False


# ===========================================================================
# Deploy (package assembly)
# ===========================================================================

# Library file names per platform. On Windows the runtime DLL and its import
# library are distinct files; on Linux the single .so serves both roles.
def _shared_lib_name() -> str:
    return "sushitrack.dll" if _IS_WINDOWS else "libsushitrack.so"


def _import_lib_name() -> str:
    return "sushitrack.lib" if _IS_WINDOWS else "libsushitrack.so"


def _find_artifact(name: str) -> Path | None:
    """
    @brief Locates a freshly built artifact under the build tree.

    @param name  File name (e.g. "sushitrack.dll", "sushitrack.lib").
    @return Path to the artifact, or None if not present.
    """
    candidates = [
        DIR_BUILD / "bin" / "Release" / name,
        DIR_BUILD / "bin" / name,
        DIR_BUILD / "bin" / "Debug" / name,
        DIR_BUILD / "lib" / "Release" / name,
        DIR_BUILD / "lib" / name,
        DIR_BUILD / "lib" / "Debug" / name,
        DIR_BUILD / name,
    ]
    for c in candidates:
        if c.exists():
            return c
    found = list(DIR_BUILD.rglob(name))
    return found[0] if found else None


_CPP_EXAMPLE = """\
/*
 * Minimal SushiTrack C-API usage example.
 *
 * Build (after `sushitrack project build --deploy cpp`):
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

Run (after `sushitrack project build --deploy python`):
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


def _deploy_cpp(pkg_dir: Path) -> bool:
    """@brief Assembles a ready-to-link C/C++ package under @p pkg_dir."""
    import shutil
    root = get_project_root()

    runtime = _find_artifact(_shared_lib_name())
    if not runtime:
        logger.error(f"Shared library {_shared_lib_name()} not found. Build first.")
        return False
    import_lib = _find_artifact(_import_lib_name())
    if not import_lib:
        logger.error(f"Import library {_import_lib_name()} not found. Build first.")
        return False

    # Public C headers (the C ABI is dependency-free: no Eigen required).
    inc_dst = pkg_dir / "include"
    (inc_dst / "SushiTrack").mkdir(parents=True, exist_ok=True)
    shutil.copy2(root / "include" / "SushiTrackC.h", inc_dst / "SushiTrackC.h")
    shutil.copy2(root / "include" / "SushiTrack" / "SushiTrackTypes.h",
                 inc_dst / "SushiTrack" / "SushiTrackTypes.h")

    # Link / runtime libraries.
    (pkg_dir / "lib").mkdir(parents=True, exist_ok=True)
    shutil.copy2(import_lib, pkg_dir / "lib" / _import_lib_name())
    if _IS_WINDOWS:
        (pkg_dir / "bin").mkdir(parents=True, exist_ok=True)
        shutil.copy2(runtime, pkg_dir / "bin" / _shared_lib_name())

    # Config + example + readme.
    cfg = root / "sushitrack.json"
    if cfg.exists():
        shutil.copy2(cfg, pkg_dir / "sushitrack.json")
    (pkg_dir / "example").mkdir(parents=True, exist_ok=True)
    (pkg_dir / "example" / "example.c").write_text(_CPP_EXAMPLE, encoding="utf-8")
    (pkg_dir / "example" / "CMakeLists.txt").write_text(_CPP_EXAMPLE_CMAKE, encoding="utf-8")
    (pkg_dir / "README.md").write_text(
        _CPP_README.format(import_lib=_import_lib_name(), runtime_lib=_shared_lib_name(),
                           platform=platform.system()),
        encoding="utf-8")
    return True


def _deploy_python(pkg_dir: Path) -> bool:
    """@brief Assembles a self-contained Python package under @p pkg_dir."""
    import shutil
    root = get_project_root()

    runtime = _find_artifact(_shared_lib_name())
    if not runtime:
        logger.error(f"Shared library {_shared_lib_name()} not found. Build first.")
        return False

    binding = root / "bindings" / "python" / "sushitrack.py"
    if not binding.exists():
        logger.error(f"Python binding not found at {binding}")
        return False

    mod_dir = pkg_dir / "sushitrack"
    mod_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(binding, mod_dir / "sushitrack.py")
    # Bundle the native library beside the module (find_library checks here).
    shutil.copy2(runtime, mod_dir / _shared_lib_name())
    (mod_dir / "__init__.py").write_text(
        "from .sushitrack import *  # noqa: F401,F403\n", encoding="utf-8")

    cfg = root / "sushitrack.json"
    if cfg.exists():
        shutil.copy2(cfg, pkg_dir / "sushitrack.json")
    (pkg_dir / "example.py").write_text(_PY_EXAMPLE, encoding="utf-8")
    (pkg_dir / "README.md").write_text(
        _PY_README.format(runtime_lib=_shared_lib_name(), platform=platform.system()),
        encoding="utf-8")
    return True


def project_deploy(kind: str) -> bool:
    """
    @brief Assembles a ready-to-use package under <root>/package for the
           requested consumer flavour.

    @param kind  "cpp" or "python".
    @return True on success.
    """
    import shutil
    pkg_dir = get_project_root() / "package"

    if pkg_dir.exists():
        logger.info(f"Clearing existing package directory: {pkg_dir}")
        shutil.rmtree(pkg_dir, ignore_errors=True)
    pkg_dir.mkdir(parents=True, exist_ok=True)

    logger.info(f"Deploying '{kind}' package for {platform.system()} -> {pkg_dir}")
    ok = _deploy_cpp(pkg_dir) if kind == "cpp" else _deploy_python(pkg_dir)
    if ok:
        logger.success(f"Package ready at {pkg_dir}")
    else:
        logger.error("Deploy failed.")
    return ok


def project_clean() -> bool:
    """@brief Runs the clean script."""
    script = _TOOLS_DIR / "project" / ("clean.bat" if _IS_WINDOWS else "clean.sh")
    try:
        subprocess.run([str(script)], check=True, shell=_IS_WINDOWS)
        return True
    except subprocess.CalledProcessError as e:
        logger.error(f"Clean failed (exit {e.returncode})")
        return False


# ===========================================================================
# Docker
# ===========================================================================

def docker_build() -> bool:
    """@brief Builds the Docker image."""
    script = _TOOLS_DIR / "docker" / ("build.bat" if _IS_WINDOWS else "build.sh")
    try:
        subprocess.run([str(script)], check=True, shell=_IS_WINDOWS)
        return True
    except subprocess.CalledProcessError:
        return False


def docker_run() -> bool:
    """@brief Runs the Docker container."""
    script = _TOOLS_DIR / "docker" / ("run.bat" if _IS_WINDOWS else "run.sh")
    try:
        subprocess.run([str(script)], check=True, shell=_IS_WINDOWS)
        return True
    except subprocess.CalledProcessError:
        return False


# ===========================================================================
# Binary discovery
# ===========================================================================

def _find_binary(name: str) -> Path | None:
    """
    @brief Searches standard build output locations for a compiled binary.

    @param name  Executable base name (without extension on Linux).
    @return Path to the found binary, or None.
    """
    root     = get_project_root()
    exe_name = f"{name}.exe" if _IS_WINDOWS else name

    candidates = [
        DIR_BUILD / "bin" / "Release" / exe_name,
        DIR_BUILD / "bin" / exe_name,
        DIR_BUILD / "bin" / "Debug" / exe_name,
        DIR_BUILD / "Release" / exe_name,
        DIR_BUILD / exe_name,
        DIR_BUILD / "Debug" / exe_name,
    ]
    for c in candidates:
        if c.exists():
            return c

    found = list(root.rglob(exe_name))
    return found[0] if found else None


# ===========================================================================
# unit test
# ===========================================================================

def project_test_unit(gtest_filter: str | None, verbose: bool) -> bool:
    """
    @brief Runs the unit_test GTest binary.

    @param gtest_filter  Optional --gtest_filter string (e.g. "TrackletTest.*").
    @param verbose       Forward --gtest_print_time=1 and colour output.
    @return True if all tests pass.
    """
    binary = _find_binary("unit_test")
    if not binary:
        logger.error(
            "unit_test binary not found. "
            "Build first:  sushitrack project build --unit"
        )
        return False

    cmd = [str(binary)]
    if gtest_filter:
        cmd.append(f"--gtest_filter={gtest_filter}")
    if verbose:
        cmd.append("--gtest_print_time=1")

    logger.info(f"Running unit tests: {binary.name}")
    try:
        result = subprocess.run(cmd)
        return result.returncode == 0
    except Exception as e:
        logger.error(f"unit_test execution failed: {e}")
        return False


# ===========================================================================
# integration test
# ===========================================================================

def project_test_integration() -> bool:
    """
    @brief Runs the integration_test GTest binary.

    @return True if all tests pass.
    """
    binary = _find_binary("integration_test")
    if not binary:
        logger.error(
            "integration_test binary not found. "
            "Build first:  sushitrack project build --integration"
        )
        return False

    cmd = [str(binary)]

    logger.info(f"Running integration tests: {binary.name}")
    try:
        result = subprocess.run(cmd)
        return result.returncode == 0
    except Exception as e:
        logger.error(f"integration_test execution failed: {e}")
        return False


# ===========================================================================
# label (regression) test
# ===========================================================================

def _get_filtered_scenarios(scenario: str | None, private: bool, public: bool) -> list[str]:
    """
    @brief Returns a list of scenario names filtered by scenario name, private, or public flags.
    """
    data_root = get_data_root()
    if not data_root.exists():
        return []

    all_seqs = sorted(
        d.name for d in data_root.iterdir()
        if d.is_dir() and (d / "gt" / "gt.txt").exists()
    )

    if scenario:
        if scenario in all_seqs:
            filtered = [scenario]
        else:
            filtered = [s for s in all_seqs if scenario.lower() in s.lower()]
    else:
        filtered = all_seqs

    result = []
    for seq in filtered:
        is_mot = seq.upper().startswith("MOT")
        if is_mot:
            is_yolox = seq.upper().endswith("YOLOX")
            if private and not is_yolox:
                continue
            if public and is_yolox:
                continue
        result.append(seq)

    return result


def _build_regression_filter(scenarios: list[str], compare: bool, tracker: str) -> str:
    """
    @brief Constructs a GTest filter for the regression_test suite.

    @param scenarios List of sequence names to run.
    @param compare   Run all three trackers side-by-side.
    @param tracker   Single tracker name (lowercase).
    @return GTest --gtest_filter string.
    """
    trackers = _KNOWN_TRACKERS if compare else [tracker.lower()]

    parts = []
    for t in trackers:
        for seq in scenarios:
            sc = seq.replace('-', '_')
            parts.append(f"*{sc}_{t}*")

    if not parts:
        return "-*"

    return ":".join(parts)


def _run_regression_binary(binary: Path, gtest_filter: str) -> tuple[bool, dict]:
    """
    @brief Executes the regression_test binary and parses PERF lines.

    @param binary        Path to the regression_test executable.
    @param gtest_filter  GTest filter string.
    @return (success, perf_data) where perf_data maps tracker→{seq→us/frame}.
    """
    try:
        result = subprocess.run(
            [str(binary), f"--gtest_filter={gtest_filter}"],
            capture_output=True, text=True,
        )
        if result.returncode != 0:
            logger.error(f"regression_test failed (exit {result.returncode})")
            print(result.stdout)
            print(result.stderr)
            return False, {}

        perf_data: dict = {t: {} for t in _KNOWN_TRACKERS}
        for line in result.stdout.splitlines():
            if "[ PERF " not in line:
                continue
            try:
                tag_start = line.index("[ PERF ") + len("[ PERF ")
                tag_end   = line.index(" ]", tag_start)
                tracker   = line[tag_start:tag_end].lower()
                rest      = line[tag_end + 2:].strip()
                sc_name, timing_str = rest.split(":", 1)
                timing = float(timing_str.split("us/frame")[0].strip())
                perf_data.setdefault(tracker, {})[sc_name.strip()] = timing
            except (ValueError, IndexError):
                continue

        return True, perf_data
    except Exception as e:
        logger.error(f"regression_test execution error: {e}")
        return False, {}


def project_test_label(
    scenario:   str | None,
    compare:    bool,
    eval_only:  bool,
    export:     str | None,
    trackeval:  bool,
    tracker:    str,
    private:    bool = False,
    public:     bool = False,
) -> bool:
    """
    @brief Runs label-based (det.txt) regression inference and TrackEval metrics.

    @param scenario   Sequence name filter, or None for all available sequences.
    @param compare    Evaluate all three trackers in one pass.
    @param eval_only  Skip binary execution; only re-run TrackEval on existing pred files.
    @param export     Directory to write CSV/JSON exports to.
    @param trackeval  Use the official TrackEval script instead of the internal runner.
    @param tracker    Target tracker (lowercase key).
    @param private    Run only sequences ending in YOLOX (MOT only).
    @param public     Run all sequences except those ending in YOLOX (MOT only).
    @return True on success.
    """
    scenarios = _get_filtered_scenarios(scenario, private, public)
    if not scenarios:
        logger.warning("No sequences matched the specified filter criteria.")
        return True

    perf_data: dict = {t: {} for t in (_KNOWN_TRACKERS if compare else [tracker])}

    if not eval_only:
        binary = _find_binary("regression_test")
        if not binary:
            logger.error(
                "regression_test binary not found. "
                "Build first:  sushitrack project build --regression"
            )
            return False

        gtest_filter = _build_regression_filter(scenarios, compare, tracker)
        logger.info(f"Running regression inference: {binary.name}")
        ok, perf_data = _run_regression_binary(binary, gtest_filter)
        if not ok:
            return False

    if trackeval:
        logger.info("Running TrackEval via official script ...")
        return _run_trackeval_official(scenarios, compare, tracker)

    logger.info("Running internal TrackEval evaluation ...")
    return _run_evaluation_internal(scenarios, compare, perf_data, export, tracker)


# ===========================================================================
# inference (YOLOX video) test
# ===========================================================================

def project_test_inference(args: argparse.Namespace) -> bool:
    """
    @brief Launches the YOLOX video inference pipeline via demo.py.

    Requires tracker_bridge.dll / libtracker_bridge.so to be present in the
    build directory (build with --regression first).

    @param args  Parsed argparse namespace containing all YOLOX/video arguments.
    @return True on success.
    """
    demo_script = _INFERENCE_DIR / "demo.py"
    if not demo_script.exists():
        logger.error(f"demo.py not found at {demo_script}")
        return False

    if not args.source:
        logger.error("--source <video_path> is required for 'test inference'")
        return False

    # Normalise tracker name (demo.py expects registry-cased name)
    tracker_key = args.tracker.lower()
    if tracker_key not in _TRACKER_MAP:
        logger.error(
            f"Unknown tracker '{args.tracker}'. "
            f"Available: {', '.join(_TRACKER_MAP.values())}"
        )
        return False
    tracker_display = _TRACKER_MAP[tracker_key]

    cmd = [
        sys.executable, str(demo_script),
        "--source",      args.source,
        "--tracker",     tracker_display,
        "--name",        args.name,
        "--device",      args.device,
        "--conf",        str(args.conf),
        "--nms",         str(args.nms),
        "--num-classes", str(args.num_classes),
    ]

    if args.ckpt:
        cmd += ["--ckpt", args.ckpt]
    if args.tsize is not None:
        cmd += ["--tsize"] + [str(t) for t in args.tsize]
    if args.stress:
        cmd.append("--stress")
    if args.no_save:
        cmd.append("--no-save")
    if args.save_name:
        cmd += ["--save_name", args.save_name]
    if args.legacy:
        cmd.append("--legacy")
    if args.mot_label:
        cmd.append("--mot-label")
    if args.no_screen:
        cmd.append("--no-screen")
        
    if args.reid:
        cmd.append("--reid")
        if args.reid_model:
            cmd += ["--reid-model", args.reid_model]

    logger.info(f"Starting YOLOX inference pipeline — tracker: {tracker_display}")
    logger.info(f"Source: {args.source}")
    try:
        subprocess.run(cmd, check=True)
        return True
    except subprocess.CalledProcessError as e:
        logger.error(f"Inference pipeline failed (exit {e.returncode})")
        return False


# ===========================================================================
# TrackEval wrappers (unchanged logic, moved here for clarity)
# ===========================================================================

def _run_trackeval_official(scenarios: list[str], compare: bool, tracker: str) -> bool:
    """
    @brief Copies pred/GT files into the TrackEval directory tree and runs the
           official run_mot_challenge.py script.

    @param scenarios List of sequences to run.
    @param compare   Evaluate all trackers.
    @param tracker   Single tracker key.
    @return True on success.
    """
    import shutil
    data_root    = get_data_root()
    trackeval_dir = DIR_3RDPARTY / "TrackEval"

    if not trackeval_dir.exists():
        logger.error("TrackEval submodule not found.")
        return False

    trackers = _KNOWN_TRACKERS if compare else [tracker]

    # Clear old data to prevent stale results and seqmaps
    trackeval_data_dir = trackeval_dir / "data"
    if trackeval_data_dir.exists():
        shutil.rmtree(trackeval_data_dir, ignore_errors=True)

    for tr in trackers:
        for seq_dir in data_root.iterdir():
            if not seq_dir.is_dir() or not (seq_dir / "gt" / "gt.txt").exists():
                continue
            seq = seq_dir.name
            if seq not in scenarios:
                continue
            src_pred = seq_dir / "pred" / tr / f"{seq}.txt"
            if not src_pred.exists():
                continue
            benchmark = "MOT20" if seq.upper().startswith("MOT20") else "MOT17"
            dst_dir = trackeval_dir / "data" / "trackers" / "mot_challenge" / f"{benchmark}-train" / tr / "data"
            dst_dir.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src_pred, dst_dir / f"{seq}.txt")
            gt_dst = trackeval_dir / "data" / "gt" / "mot_challenge" / f"{benchmark}-train" / seq
            if not (gt_dst / "gt" / "gt.txt").exists():
                (gt_dst / "gt").mkdir(parents=True, exist_ok=True)
                src_gt = seq_dir / "gt" / "gt.txt"
                if src_gt.exists():
                    shutil.copy2(src_gt, gt_dst / "gt" / "gt.txt")
                src_info = seq_dir / "seqinfo.ini"
                if src_info.exists():
                    shutil.copy2(src_info, gt_dst / "seqinfo.ini")

    script_path = trackeval_dir / "scripts" / "run_mot_challenge.py"
    if not script_path.exists():
        logger.error(f"Official TrackEval script not found at {script_path}")
        return False

    success = True
    for benchmark in ["MOT17", "MOT20"]:
        valid_trackers, all_seqs = [], set()
        for tr in trackers:
            tdata = trackeval_dir / "data" / "trackers" / "mot_challenge" / f"{benchmark}-train" / tr / "data"
            if tdata.exists():
                seqs = [f.stem for f in tdata.iterdir() if f.suffix == ".txt"]
                if seqs:
                    valid_trackers.append(tr)
                    all_seqs.update(seqs)
        if not valid_trackers:
            continue
        seqmap_dir = trackeval_dir / "data" / "gt" / "mot_challenge" / "seqmaps"
        seqmap_dir.mkdir(parents=True, exist_ok=True)
        seqmap_path = seqmap_dir / f"{benchmark}-train.txt"
        with open(seqmap_path, "w") as f:
            f.write("name\n")
            for s in sorted(all_seqs):
                f.write(f"{s}\n")
        cmd = [sys.executable, str(script_path),
               "--BENCHMARK", benchmark,
               "--TRACKERS_TO_EVAL"] + valid_trackers + ["--SPLIT_TO_EVAL", "train"]
        if len(scenarios) == 1:
            cmd += ["--SEQ_INFO", scenarios[0]]
        try:
            r = subprocess.run(cmd, cwd=str(trackeval_dir))
            if r.returncode != 0:
                success = False
        except Exception as e:
            logger.error(f"TrackEval script error: {e}")
            success = False

    return success


def _run_evaluation_internal(
    scenarios:  list[str],
    compare:    bool,
    perf_data:  dict,
    export_dir: str | None,
    tracker:    str,
) -> bool:
    """
    @brief Runs the internal Python TrackEval wrapper.

    @param scenarios  List of sequences to evaluate.
    @param compare    Evaluate all trackers.
    @param perf_data  FPS data from the regression binary run.
    @param export_dir Optional export directory.
    @param tracker    Single tracker key.
    @return True on success.
    """
    if not _EVAL_AVAILABLE:
        logger.error(
            "Internal evaluator unavailable. Install TrackEval:\n"
            "  pip install git+https://github.com/JonathonLuiten/TrackEval.git"
        )
        return False

    data_root = get_data_root()
    if not data_root.exists():
        logger.error(f"Data directory not found: {data_root}")
        return False

    trackers  = _KNOWN_TRACKERS if compare else [tracker]

    try:
        runner  = TrackEvalRunner(data_root=str(data_root))
        results = runner.evaluate(
            trackers=trackers,
            scenarios=scenarios,
            perf_fps={
                tr: {sc: 1_000_000.0 / us for sc, us in sc_map.items()}
                for tr, sc_map in perf_data.items() if sc_map
            },
        )
        if not results:
            logger.error("No evaluation results. Verify that pred/*.txt files exist.")
            return False
        print_trackeval_table(results, compare=compare)
        if export_dir:
            export_trackeval_results(results, export_dir)
        return True
    except Exception as e:
        logger.error(f"Evaluation failed: {e}")
        return False


# ===========================================================================
# Argument parser
# ===========================================================================

def _build_parser() -> argparse.ArgumentParser:
    """@brief Constructs the full CLI argument parser."""
    parser = argparse.ArgumentParser(
        prog="sushitrack",
        description="SushiTrack Pipeline Tool",
    )
    sub = parser.add_subparsers(dest="command")

    # ------------------------------------------------------------------
    # project
    # ------------------------------------------------------------------
    proj = sub.add_parser("project", help="Project actions (build / test / clean)")
    proj_sub = proj.add_subparsers(dest="action")

    # project build
    p_build = proj_sub.add_parser("build", help="Compile the project")
    p_build.add_argument("--clean",        action="store_true", help="Wipe build dir first")
    p_build.add_argument("--unit",         action="store_true", help="Include unit_test target")
    p_build.add_argument("--regression",   action="store_true", help="Include regression_test + tracker_bridge")
    p_build.add_argument("--integration",  action="store_true", help="Include integration_test target")
    p_build.add_argument("--test",         action="store_true", help="Alias for all test targets")
    p_build.add_argument("--deploy",       choices=["cpp", "python"], default=None,
                         help="After building, assemble a ready-to-use package/ (cpp or python)")

    # project test
    p_test = proj_sub.add_parser("test", help="Run tests")
    p_test.add_argument(
        "test_type",
        nargs="?",
        choices=["all", "unit", "label", "inference", "integration"],
        default="all",
        help=(
            "(default) all — unit tests then integration then label regression in sequence\n"
            "unit           — GTest deterministic unit tests (no external data)\n"
            "integration    — GTest end-to-end integration tests\n"
            "label          — regression_test on det.txt datasets + TrackEval metrics\n"
            "inference      — YOLOX video inference pipeline via tracker_bridge"
        ),
    )

    # shared test options
    p_test.add_argument("--filter",     type=str,  default=None, help="GTest --gtest_filter (unit mode)")
    p_test.add_argument("--verbose",    action="store_true",     help="Verbose GTest output (unit mode)")
    p_test.add_argument("--scenario",   type=str,  default=None, help="Sequence name filter (label mode)")
    p_test.add_argument("--compare",    action="store_true",     help="Run all three trackers (label mode)")
    p_test.add_argument("--eval-only",  action="store_true",     help="Skip binary; re-run TrackEval only (label mode)")
    p_test.add_argument("--trackeval",  action="store_true",     help="Use official TrackEval script (label mode)")
    p_test.add_argument("--export",     type=str,  default=None, help="Export results directory (label mode)")
    p_test.add_argument("--private",    action="store_true",     help="Run only sequences ending in YOLOX (MOT only)")
    p_test.add_argument("--public",     action="store_true",     help="Run all sequences except those ending in YOLOX (MOT only)")

    # YOLOX / video options (inference mode)
    p_test.add_argument("--source",      type=str,               help="Video path or camera index (inference)")
    p_test.add_argument("--tracker",     type=str, default="SushiTrack", help="Tracker name (inference / label)")
    p_test.add_argument("-n", "--name",  type=str, default="yolox-s",   help="YOLOX model name")
    p_test.add_argument("-c", "--ckpt",  type=str, default=None,        help="YOLOX checkpoint path")
    p_test.add_argument("--device",      type=str, default="gpu",       help="cpu or gpu")
    p_test.add_argument("--conf",        type=float, default=0.1,       help="Detection confidence threshold")
    p_test.add_argument("--nms",         type=float, default=0.45,      help="NMS threshold")
    p_test.add_argument("--tsize",       type=int, nargs="+", default=None, help="Inference resolution [H W] or [S]")
    p_test.add_argument("--num-classes", type=int, default=80,          help="YOLOX class count")
    p_test.add_argument("--legacy",      action="store_true",           help="Legacy YOLOX normalisation")
    p_test.add_argument("--stress",      action="store_true",           help="Headless maximum throughput mode")
    p_test.add_argument("--no-save",     action="store_true",           help="Do not save output video")
    p_test.add_argument("--save_name",   type=str, default=None,        help="Output video filename")
    p_test.add_argument("--mot-label",   action="store_true",           help="Write detections to output/det.txt (inference)")
    p_test.add_argument("--no-screen",   action="store_true",           help="Disable UI display (inference)")

    # ReID options (inference)
    p_test.add_argument("--reid",        action="store_true",           help="Enable OSNet ReID extraction (inference)")
    p_test.add_argument("--reid-model",  type=str, default="osnet_x1_0.onnx", help="Path to ReID ONNX model (inference)")

    # project clean
    proj_sub.add_parser("clean", help="Remove build artefacts")

    # ------------------------------------------------------------------
    # docker
    # ------------------------------------------------------------------
    dock = sub.add_parser("docker", help="Docker actions")
    dock_sub = dock.add_subparsers(dest="action")
    dock_sub.add_parser("build", help="Build Docker image")
    dock_sub.add_parser("run",   help="Run Docker container")

    return parser


# ===========================================================================
# Entry point
# ===========================================================================

def main():
    """@brief CLI dispatcher."""
    parser = _build_parser()
    args   = parser.parse_args()

    if args.command == "project":
        success = False

        if args.action == "build":
            # --test builds both unit and regression targets
            unit = args.unit or args.test
            regression = args.regression or args.test
            integration = args.integration or args.test
            success = project_build(
                clean       = args.clean,
                unit        = unit,
                regression  = regression,
                integration = integration,
            )
            # Deploy requires the shared library, which a plain build always
            # produces; assemble the package only if the build succeeded.
            if success and args.deploy:
                success = project_deploy(args.deploy)

        elif args.action == "clean":
            success = project_clean()

        elif args.action == "test":
            if args.private and args.public:
                logger.error("Cannot specify both --private and --public simultaneously.")
                sys.exit(1)

            test_type = args.test_type or "all"

            if test_type == "all":
                # Sequential pipeline: unit → integration → label (regression).
                # Fail-fast: subsequent stages are skipped if previous stage fails.
                logger.info("[Pipeline] Stage 1/3 — unit tests")
                unit_ok = project_test_unit(
                    gtest_filter = args.filter,
                    verbose      = args.verbose,
                )
                if not unit_ok:
                    logger.error("[Pipeline] Unit tests failed — pipeline aborted.")
                    success = False
                else:
                    logger.info("[Pipeline] Stage 2/3 — integration tests")
                    integration_ok = project_test_integration()
                    
                    if not integration_ok:
                        logger.error("[Pipeline] Integration tests failed — pipeline aborted.")
                        success = False
                    else:
                        logger.info("[Pipeline] Stage 3/3 — label regression")
                        tracker_key = args.tracker.lower()
                        if tracker_key not in _TRACKER_MAP:
                            logger.error(
                                f"Unknown tracker '{args.tracker}'. "
                                f"Available: {', '.join(_TRACKER_MAP.values())}"
                            )
                            sys.exit(1)
                        success = project_test_label(
                            scenario  = args.scenario,
                            compare   = args.compare,
                            eval_only = args.eval_only,
                            export    = args.export,
                            trackeval = args.trackeval,
                            tracker   = tracker_key,
                            private   = args.private,
                            public    = args.public,
                        )

            elif test_type == "unit":
                success = project_test_unit(
                    gtest_filter = args.filter,
                    verbose      = args.verbose,
                )
                
            elif test_type == "integration":
                success = project_test_integration()

            elif test_type == "label":
                tracker_key = args.tracker.lower()
                if tracker_key not in _TRACKER_MAP:
                    logger.error(
                        f"Unknown tracker '{args.tracker}'. "
                        f"Available: {', '.join(_TRACKER_MAP.values())}"
                    )
                    sys.exit(1)

                success = project_test_label(
                    scenario  = args.scenario,
                    compare   = args.compare,
                    eval_only = args.eval_only,
                    export    = args.export,
                    trackeval = args.trackeval,
                    tracker   = tracker_key,
                    private   = args.private,
                    public    = args.public,
                )

            elif test_type == "inference":
                success = project_test_inference(args)

        else:
            parser.print_help()
            sys.exit(0)

        if not success:
            sys.exit(1)

    elif args.command == "docker":
        success = False
        if args.action == "build":
            success = docker_build()
        elif args.action == "run":
            success = docker_run()
        if not success:
            sys.exit(1)

    else:
        parser.print_help()


if __name__ == "__main__":
    main()
