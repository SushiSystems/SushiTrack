# Command line reference

Commands are flat: `st <command> [flags]`, with `st container <action>` and
`st docs bundle` the two subgroups. Arguments after a bare `--` are forwarded to the underlying binary.

| Command | Purpose |
|---|---|
| `st build` | Configure + build; test targets built by default (`--no-test` to skip) |
| `st test` | Run the tests by size label: `--label small / medium / large / quarantined` |
| `st eval` | MOT label benchmark + TrackEval metrics (not a CTest suite) |
| `st infer` | YOLOX video inference pipeline through `tracker_bridge` |
| `st run` | Run a built executable, by name or from a picker |
| `st clean` | Remove every tree under `build/` and `package/` |
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
| `st docs bundle --release X.Y.Z` | Pack the published manual pages into the documentation bundle |
| `st --version` / `st --describe` | Installed version / command catalogue as JSON |

Configuration is layered: built-in defaults → `cli/config.toml` → `cli/config.local.toml`
→ `ST_*` environment variables (`ST_BUILD_TYPE`, `ST_GENERATOR`,
`ST_CMAKE`, `ST_DOCKER_IMAGE`, `ST_TRACKER`, …). `st config`
prints which layer each value came from.

## `st build`

| Flag | Effect |
|---|---|
| `--type, -t <type>` | `debug` (default) / `release` / `relwithdebinfo` / `minsizerel`; each type builds in `build/default-<type>` |
| `--no-test` | Build the library only; test targets are built by default |
| `--clean` | Remove the tree of the build before configuring |
| `--jobs, -j <n>` | Parallel build jobs (default: the hardware threads) |
| `--report` | Print the time and peak memory per kind of edge and write `build_report.json` into the tree |
| `-D VAR=VALUE` | Extra `-D` option forwarded to cmake (repeatable) |
| `--sanitize {address,thread}` | Build SushiTrack's own targets with ASan and UBSan, or with TSan; GCC or Clang only. The next build without it reconfigures the tree |
| `--deploy {cpp,python}` | After a successful build, assemble a redistributable `package/` for the chosen consumer ([details](../guides/DEPLOY_PACKAGES.md)) |

## `st test`

Runs the tests through CTest, selected by size label. Every GoogleTest suite opens with its size,
`Small_`, `Medium_` or `Large_`, and the build registers it under that label, so a new test file
is picked up with no CLI change. [Test suites](../guides/TESTING.md) says what each size may use.

| Flag | Effect |
|---|---|
| `--label, -l <label>` | `small`, `medium`, `large` or `quarantined`; repeatable. Default: `small` and `medium`. Quarantined tests run only when named |
| `--filter, -f <regex>` | `ctest -R` over `Suite.Case` test names |
| `--repeat, -r <n>` | `ctest --repeat until-fail:<n>`: re-run each test until it fails; flaky-test hunting |
| `--jobs, -j <n>` | Run up to N tests concurrently (`ctest --parallel`) |
| `--cli` | Run the pytest suite of the `st` package instead of CTest, with the same labels |
| `--sanitize {address,thread}` | Build with that sanitizer first, as `st build --sanitize` does |

```bash
st test                              # small and medium
st test --label small
st test --label small --sanitize address   # under ASan and UBSan
st test --label small -f 'RectTest'
st test --label medium --repeat 3
st test --label large                # needs real hardware
st test --label quarantined
st test --cli                        # the st package's own tests
```

For GTest-level knobs (shuffle, break-on-failure) run the binary directly instead:

```bash
st run sushitrack_tests -- --gtest_shuffle --gtest_break_on_failure
```

## `st eval`

The MOT label benchmark. Not a CTest suite: it runs `mot_engine` over
det.txt-backed scenarios, reads the per-frame latency of each pair from its profile record in
`profiles/` under the build tree (the `[ PERF ]` line when a pair has none), then runs TrackEval and prints a metrics table (HOTA / DetA / AssA /
MOTA / IDF1 / IDSW / FP / FN / Lat).

| Flag | Effect |
|---|---|
| `--benchmark, -b <name>` | `mot17`, `mot20` or `dancetrack`, a folder under `applications/benchmarks/`; default: every benchmark, each run and scored on its own |
| `--scenario <name>` | Sequence filter (exact match or case-insensitive substring) |
| `--tracker <name>` | `sushitrack` (default), `bytetrack`, or `ocsort` |
| `--compare` | Run all three trackers side-by-side |
| `--eval-only` | Skip the binary; re-score existing `pred/*.txt` files |
| `--trackeval` | Use the official `third_party/TrackEval/scripts/run_mot_challenge.py` instead of the internal runner |
| `--export <dir>` | Export TrackEval JSON/CSV to `<dir>/<benchmark>` |
| `--private` | Only `*-YOLOX` sequences (MOT only) |
| `--public` | All sequences except `*-YOLOX` (MOT only) |

```bash
st eval                              # every benchmark
st eval --benchmark mot17 --compare
st eval --scenario MOT17-04-DPM --compare
st eval --eval-only --export ./results
st eval --trackeval --private
```

The internal runner needs the evaluator's own dependencies (numpy, scipy,
TrackEval) from `.config/conda/environment.yml`; `--trackeval` bypasses it. A benchmark is a folder of
`applications/benchmarks/` with a `data/` folder; its sequences are the folders there that hold
`det/det.txt` and `gt/gt.txt`.

## `st infer`

Launches the YOLOX-driven video inference pipeline
(`applications/inference/demo.py`), which loads `tracker_bridge.{dll,so}` via
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

st infer -n yolox-x -c third_party/weight/mot17x.pth.tar --source applications/benchmarks/mot20/data/MOT20-01/img1 --num-classes 1 --legacy

st infer --source 0 --tracker sushitrack --no-save --no-screen --stress
```

`--source` also accepts an image-sequence directory (`<path>/img1`), not just a video
file or camera index. The frames of a downloaded benchmark sequence work, e.g.
`applications/benchmarks/mot20/data/MOT20-01/img1` or
`applications/benchmarks/dancetrack/data/dancetrack0001/img1`.

If the YOLOX checkpoint filename contains `mot17` and `--tsize` is omitted/640,
the pipeline overrides `tsize` to `(800, 1440)` to match the MOT17 training
resolution.

## `st clean`

Removes every tree under `build/` and the assembled package directory (`package/` by
default, overridable in the `paths` section of `cli/config.toml`).
`--predictions` removes the `pred/` folder of every benchmark sequence instead, and leaves
the build tree.

```bash
st clean
st clean --predictions
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
st run sushitrack_tests
st run sushitrack_tests -- --gtest_filter='Small_TrackletTest.*'
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

## `st docs bundle`

Packs the manual pages `docs/publish.toml` lists into `docs-bundle-<release>.tar.gz`, the
archive `docs.sushisystems.io` reads, and writes `docs-bundle-<release>.tar.gz.sha256` beside
it. The command comes from `sushicore` and behaves the same in every Sushi CLI.

| Flag | Meaning |
|---|---|
| `--release X.Y.Z` | The release the bundle is named after, three integers. Required; `v1.2.3` and `1.2.3-rc1` are refused |
| `--out DIR` | The folder the archive is written to. Default: `build/docs/bundle` under the repository root |

It prints the archive's path with its page count, then `sha256` and the digest. The bundle
records the checked-out commit, so the command needs a git checkout. SushiTrack publishes no
API reference: the bundle holds pages alone, and a bare `st docs` prints the group's help and
exits with code 2.

Every Markdown file under `getting_started/`, `guides/`, `architecture/` and `reference/` is
published. The command stops with one line and exit code 1 when a page has no level-one
heading, when `docs/README.md` does not link it, or when one of its links names a file that
does not exist.

```bash
st docs bundle --release 1.2.3
st docs bundle --release 1.2.3 --out build/site
```

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
| 1 | The command failed: a missing build tree, binary or script, a failed deploy or evaluation, a directory `st clean` could not remove, no sequence for `st eval` to score, a malformed `cli/config.toml`, an unknown build type or tracker, a required `st doctor` check, or a page `st docs bundle` refuses |
| 2 | A usage error: an unknown command or option, `--private` with `--public`, a missing `--source`, `st docs bundle` without `--release`, a bare `st docs` |
| 127 | A tool the command needs (`cmake`, `ctest`, `docker`) is not installed |
| 130 | Interrupted with Ctrl+C |
| other | The exit code of the child process (`cmake`, `ctest`, `docker`, the executable `st run` started) |
