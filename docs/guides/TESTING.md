# Test suites

Every C++ test sits in one program, `sushitrack_tests`, built from the mirror of the public
headers under `tests/`. The test of `include/SushiTrack/<area>/<name>.hpp` is
`tests/<area>/test_<name>.cpp`; the tracker and the tracklet keep their tests in a folder each,
`tests/tracking/test_tracker/` and `tests/tracking/test_tracklet/`, one file per aspect, and the
C API in `tests/c_api/test_sushitrack_c/`. The helpers the tests share are listed in
[`tests/README.md`](../../tests/README.md).

## Sizes

A suite's name opens with its size, and the size is the CTest label the build gives it. What a size
may touch is the `testing` skill's rule:

| Size | The test may touch | Never | Runs |
| --- | --- | --- | --- |
| `small` | The memory of its own process, one thread, an injected clock and seed, and a scoped in-memory log sink (`CapturedLog`) | A file, the environment, a thread, a sleep, the real clock, a process-wide instance it changes | Every build |
| `medium` | One machine: a temporary folder of its own that it removes, threads, files such as `assets/sushitrack.json`, the environment | A sleep, state left behind | Every build |
| `large` | Anything, including whole benchmark sequences | | Nightly or weekly, never before a merge |

The skill's timeouts per test case are 1 s small, 10 s medium and 120 s large. The build still
sets 60 s, 300 s and 900 s; RW-36 moves it to the skill's values.

A suite whose name has no size stops the configure. A test that fails now and then is fixed or
quarantined: it keeps its size and also carries the label `quarantined`, which takes it out of
every run that does not name it, and it has an entry in
[Known issues](../backlog/README.md).

A test that reads a file, the environment named by `SUSHITRACK_CONFIG` or a registry it does not
own is `medium`. A small test reaches the same logic through a seam: a stream in place of a file,
a `KalmanFilterRegistry` or `TrackerRegistry` it owns, or parameters passed to an adapter.

## Running

```bash
st test                              # small and medium, the quarantine left out
st test --label small                # one size
st test --label small -f 'RectTest'  # a ctest -R regex over Suite.Case names
st test --label medium --repeat 3    # re-run until the first failure
st test --label quarantined          # only the quarantined tests
st run sushitrack_tests -- --gtest_filter='Small_TrackletTest.*'
```

`st test` and the build set `SUSHITRACK_CONFIG` to `assets/sushitrack.json` for the tests.

The applications under `applications/` have their own mirror, `tests/applications/`, built
into `sushitrack_application_tests`. The MOT benchmark is a program, not a test; it is in
[Evaluation pipeline](EVALUATION.md).

## CLI tests (`cli/tests/`)

A pytest suite for the `st` package. It needs no build. It mirrors the package: the test of
`cli/sushitrack_cli/services/<name>.py` is `cli/tests/services/test_<name>.py`, and `cli.py`,
whose tests do not fit one file, has `cli/tests/test_cli/`. Each file carries its size as
`pytestmark`, and `conftest.py` refuses a test with no size and gives each its timeout.

`sushitrack_cli` imports `sushicore` at module load, so the suite cannot be collected without it.
sushicore is an ordinary PyPI dependency of the CLI, not a checkout to be found on disk, so
`cli/tests/conftest.py` searches for nothing: sushicore is installed or it is not, and
`conftest.py` raises when it is not.

The package under test sits in `cli/`, one level above `conftest.py`, and the suite is run from the
repository root as often as from `cli/`. `conftest.py` puts `cli/` on `sys.path` so the same
command works from either.

The `session` fixture of `conftest.py` is a `Workspace` over sushicore's `RecordingRunner` and
`RecordingConsole`. A service takes it as `workspace=`, and a command test hands it in as the context object, so no test
patches a function of the CLI.

Nothing in the fixtures reaches the machine. A test that runs a real tool, prints through the
shared console, or reads the developer's own project says something different on someone else's
machine.

```bash
st test --cli                 # small and medium
st test --cli --label small
```
