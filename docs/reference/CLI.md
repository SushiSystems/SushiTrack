# Command line reference

Commands are flat: `st <command> [flags]`, with `st container <action>` the only
subgroup. Arguments after a bare `--` are forwarded to the underlying binary.

| Command | Purpose |
|---|---|
| `st build` | Configure + build; test targets built by default (`--no-test` to skip) |
| `st test` | Run a CTest label group: `--suite unit / integration / regression / functional / all` |
| `st eval` | MOT label benchmark + TrackEval metrics (not a CTest suite) |
| `st infer` | YOLOX video inference pipeline through `tracker_bridge` |
| `st run` | Run a built executable, by name or from a picker |
| `st clean` | Remove `build/` and `package/` |
| `st deploy {cpp,python}` | Assemble `package/` from the current build |
| `st status` | What is built, what is missing, which data sets are present |
| `st setup [--dry-run] [--yes] [--toolchain NAME] [--no-gpu]` | Provision the dependencies in `cli/sushistack.deps.toml`, then run the doctor |
| `st doctor [--for GROUP]` | Check readiness per group (`build`, `test`, `infer`, `eval`); non-zero exit when a required check fails |
| `st link [--workspace PATH]` | Register this checkout in a SushiStack workspace |
| `st unlink [--workspace PATH]` | Remove this checkout from a SushiStack workspace |
| `st config` | Resolved configuration and the origin of every value |
| `st paths` | Resolved project directories |
| `st env [--all]` | Environment cmake and the test binaries execute under |
| `st container build` / `st container run` | Image build / bind-mounted container |
| `st --version` / `st --describe` | Installed version / command catalogue as JSON |

Configuration is layered: built-in defaults → `cli/config.toml` → `cli/config.local.toml`
→ `ST_*` environment variables (`ST_BUILD_DIR`, `ST_BUILD_TYPE`, `ST_GENERATOR`,
`ST_CMAKE`, `ST_DOCKER_IMAGE`, `ST_TRACKER`, …). `st config`
prints which layer each value came from.

## `st build`

| Flag | Effect |
|---|---|
| `--type, -t <type>` | `release` (default) / `debug` / `relwithdebinfo` / `minsizerel` |
| `--no-test` | Build the library only; test targets are built by default |
| `--clean` | Remove the build directory before configuring |
| `--jobs, -j <n>` | Parallel build jobs (default: CPU count) |
| `-D VAR=VALUE` | Extra `-D` option forwarded to cmake (repeatable) |
| `--deploy {cpp,python}` | After a successful build, assemble a redistributable `package/` for the chosen consumer ([details](../guides/DEPLOY_PACKAGES.md)) |

## `st test`

Runs tests through CTest labels. Each test target carries one label, assigned in
`tests/*/CMakeLists.txt` on its `gtest_discover_tests` call, so a new test file,
or a whole target, is picked up with no CLI change.

| Suite | Label selection | Contents |
|---|---|---|
| `unit` | `-L '^unit$'` | `unit_test`: deterministic component tests, no external data |
| `integration` | `-L '^integration$'` | `integration_test`: end-to-end coverage through the C API |
| `regression` | `-L '^regression$'` | `regression_test`: MOT scenarios, pass/fail only (metrics: `st eval`) |
| `functional` | `-L '^(unit\|integration\|regression)$'` | all three; **the default** |
| `all` | no `-L` | every registered test |

| Flag | Effect |
|---|---|
| `--suite, -s <name>` | Label group to run (default `functional`) |
| `--filter, -f <regex>` | `ctest -R` over `Suite.Case` test names |
| `--repeat, -r <n>` | `ctest --repeat until-fail:<n>`: re-run each test until it fails; flaky-test hunting |
| `--jobs, -j <n>` | Run up to N tests concurrently (`ctest --parallel`) |

```bash
st test                              # functional: unit + integration + regression
st test --suite unit
st test --suite unit -f 'RectTest'
st test --suite integration --repeat 3
st test --suite all -j 8
```

For GTest-level knobs (shuffle, break-on-failure) run the binary directly instead:

```bash
st run unit_test -- --gtest_shuffle --gtest_break_on_failure
```

## `st eval`

The MOT label benchmark. Not a CTest suite: it runs `regression_test` over
det.txt-backed scenarios, parses the per-sequence `[ PERF ]` timings out of its
output, then runs TrackEval and prints a metrics table (HOTA / DetA / AssA /
MOTA / IDF1 / IDSW / FP / FN / Lat). `st test --suite regression` answers only
pass/fail for the same binary.

| Flag | Effect |
|---|---|
| `--scenario <name>` | Sequence filter (exact match or case-insensitive substring) |
| `--tracker <name>` | `sushitrack` (default), `bytetrack`, or `ocsort` |
| `--compare` | Run all three trackers side-by-side |
| `--eval-only` | Skip the binary; re-score existing `pred/*.txt` files |
| `--trackeval` | Use the official `third_party/TrackEval/scripts/run_mot_challenge.py` instead of the internal runner |
| `--export <dir>` | Export TrackEval JSON/CSV to `<dir>` |
| `--private` | Only `*-YOLOX` sequences (MOT only) |
| `--public` | All sequences except `*-YOLOX` (MOT only) |

```bash
st eval
st eval --compare
st eval --scenario MOT17-04-DPM --compare
st eval --eval-only --export ./results
st eval --trackeval --private
```

The internal runner needs the evaluator's own dependencies (numpy, scipy,
TrackEval) from `environment.yml`; `--trackeval` bypasses it. Scenarios registered
in `tests/regression/ScenarioList.h`: full MOT17 train set (21 sequences across DPM/FRCNN/SDP
detectors plus `-YOLOX` variants for each), MOT20 (`-01/-02/-03/-05` with `-YOLOX`
variants), and DanceTrack sequences (`dancetrack0001`, `0002`, `0006`, `0008`,
`0012`, `0015`, …).

## `st infer`

Launches the YOLOX-driven video inference pipeline
(`tests/regression/inference/demo.py`), which loads `tracker_bridge.{dll,so}` via
ctypes. Arguments after `--` are forwarded to `demo.py` untouched.

| Flag | Default | Effect |
|---|---|---|
| `--source <path>` | required | Video file path or camera index |
| `--tracker <name>` | `sushitrack` | Tracker selector (`sushitrack` / `bytetrack` / `ocsort`) |
| `-n, --name <name>` | `yolox-s` | YOLOX architecture |
| `-c, --ckpt <path>` | – | Pre-trained YOLOX weights |
| `--device {cpu,gpu}` | `gpu` | Inference device |
| `--conf <float>` | `0.1` | Detection confidence floor |
| `--nms <float>` | `0.45` | NMS IoU threshold |
| `--tsize <H W>` | – | Inference resolution; pass the same value twice for a square size |
| `--num-classes <int>` | `80` | Class count (set `1` for MOT) |
| `--legacy` | off | Legacy YOLOX normalisation (BGR→RGB, 0–1 scale, ImageNet mean/std) |
| `--stress` | off | Headless max-throughput mode |
| `--no-save` | off | Disable output-video writer |
| `--save_name <name>` | – | Output filename override |
| `--mot-label` | off | Append detections to `output/det.txt` in MOT format |
| `--no-screen` | off | Disable UI display |
| `--reid` | off | Enable OSNet ReID embedding extraction (fills `feat` per detection) |
| `--reid-model <path>` | `osnet_x1_0.onnx` | Path to the ReID ONNX model |

```bash
st infer -n yolox-x -c third_party/weight/mot17x.pth.tar --source third_party/video/MOT17-04.mp4 --num-classes 1 --legacy

st infer -n yolox-x -c third_party/weight/mot17x.pth.tar --source tests/regression/evaluator/data/MOT20-01/img1 --num-classes 1 --legacy

st infer --source 0 --tracker sushitrack --no-save --no-screen --stress
```

`--source` also accepts an image-sequence directory (`<path>/img1`), not just a video
file or camera index. `tests/regression/evaluator/data/` already holds 76 real
tracker benchmark sequences (MOT17, MOT20, DanceTrack) used by the regression
suite. Any of their names works, e.g. `tests/regression/evaluator/data/MOT20-01/img1`
or `tests/regression/evaluator/data/dancetrack0001/img1`.

If the YOLOX checkpoint filename contains `mot17` and `--tsize` is omitted/640,
the pipeline overrides `tsize` to `(800, 1440)` to match the MOT17 training
resolution.

## `st clean`

Removes the build directory and the assembled package directory (`build/` and
`package/` by default; both overridable in the `paths` section of `cli/config.toml`).

```bash
st clean
```

## `st run`

Runs an executable from the build tree. The target is matched by exact name first, then
by substring; with no target, or with `--select`, `st` lists the executables and asks
which one to run. Arguments after `--` go to the executable.

| Flag | Effect |
|---|---|
| `--select, -s` | Show the picker even when a target is given |

```bash
st run
st run unit_test
st run unit_test -- --gtest_filter='TrackletTest.*'
```

## `st deploy`

`st deploy {cpp,python}` assembles `package/` from the current build without building
first; `st build --deploy {cpp,python}` does both. The layouts are under
[Deploy packages](../guides/DEPLOY_PACKAGES.md).

```bash
st deploy cpp
st deploy python
```

## `st container build` / `st container run`

`docker build` / `docker run` are issued directly; there are no shell wrappers. `run` mounts the repo at the configured `docker.workdir` and passes `--gpus all` unless `--no-gpu` is given:
`docker run -it --rm --gpus all -v "<root>:/workspace/sushitrack" -w /workspace/sushitrack sushitrack bash`. Args after `--` replace the default `bash` with the command run inside the container.

```bash
st container build
st container build --no-cache
st container run
st container run -- bash -lc "echo hi"
```

`st docker build` and `st docker run` are the old spellings. They still run, are left out
of `st --help` and `st --describe`, print one notice naming the new spelling when standard
error is a terminal, and are removed after 2027-03-25. `ST_NO_DEPRECATION_NOTICE=1`
silences the notice.

## Diagnostics

All four are read-only.

| Command | Prints |
|---|---|
| `st status` | The build tree, the library, the test binaries, `tracker_bridge`, the data sets and `package/`, each with its state |
| `st config` | Every resolved setting, its value and the layer it came from |
| `st paths` | Every resolved project directory and whether it exists |
| `st env` | The build-relevant variables of the environment cmake, ctest and the test binaries run under; `--all, -a` prints every variable |

## Provisioning

`setup`, `doctor`, `link` and `unlink` come from `sushicore` and behave the same in every
Sushi CLI.

| Command | Flags |
|---|---|
| `st setup` | `--dry-run` shows what would be installed; `--yes` answers the LLVM-download prompt; `--toolchain NAME` (repeatable) also installs `intel-llvm`, `adaptivecpp` or `oneapi`; `--no-gpu` skips the GPU toolkit |
| `st doctor` | `--for GROUP` limits the report to `build`, `test`, `infer` or `eval` |
| `st link` / `st unlink` | `--workspace PATH` names the SushiStack workspace |

## Root options

| Option | Effect |
|---|---|
| `--version` | Prints `sushitrack-cli <version>`, the installed package version, and exits |
| `--describe` | Prints every visible command and its parameters as JSON and exits |
| `--help` | Prints the help page; a bare `st` prints the same page |

## Exit codes

| Code | Meaning |
|---|---|
| 0 | The command succeeded |
| 1 | The command failed: a missing build tree, binary or script, a failed deploy or evaluation, a directory `st clean` could not remove, no sequence for `st eval` to score, a malformed `cli/config.toml`, an unknown build type or tracker, or a required `st doctor` check |
| 2 | A usage error: an unknown command or option, `--private` with `--public`, a missing `--source` |
| 127 | A tool the command needs (`cmake`, `ctest`, `docker`) is not installed |
| 130 | Interrupted with Ctrl+C |
| other | The exit code of the child process (`cmake`, `ctest`, `docker`, the executable `st run` started) |
