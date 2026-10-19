# Test suites

Every C++ test sits in one program, `sushitrack_tests`, built from the mirror of the public
headers under `tests/`. The test of `include/SushiTrack/<area>/<name>.hpp` is
`tests/<area>/test_<name>.cpp`; the tracker and the tracklet keep their tests in a folder each,
`tests/tracking/test_tracker/` and `tests/tracking/test_tracklet/`, one file per aspect, and the
C API in `tests/c_api/test_sushitrack_c/`. The helpers the tests share are listed in
[`tests/README.md`](../../tests/README.md).

## Sizes

A suite's name opens with its size, and the size is the CTest label the build gives it, with its
timeout:

| Size | The test may use | Timeout |
| --- | --- | --- |
| `small` | One process and one thread, no file outside the build tree | 60 s |
| `medium` | Threads, files such as `assets/sushitrack.json` | 300 s |
| `large` | Real hardware, or a run longer than `medium` allows | 900 s |

A suite whose name has no size stops the configure. A test that fails now and then is fixed or
quarantined: it keeps its size and also carries the label `quarantined`, which takes it out of
every run that does not name it, and it has an entry in
[Known issues](../reference/KNOWN_ISSUES.md).

Today the configuration loader and the logger's two-thread test are `medium`; every other suite
is `small`.

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

A pytest suite for the `st` package. It needs no build.

`sushitrack_cli` imports `sushicore` at module load, so the suite cannot be collected without it.
sushicore is an ordinary PyPI dependency of the CLI, not a checkout to be found on disk, so
`cli/tests/conftest.py` searches for nothing: sushicore is installed or it is not, and
`conftest.py` raises when it is not.

The package under test sits in `cli/`, one level above `conftest.py`, and the suite is run from the
repository root as often as from `cli/`. `conftest.py` puts `cli/` on `sys.path` so the same
command works from either.

Nothing in the fixtures reaches the machine. A test that runs a real tool, prints through the
shared console, or reads the developer's own project says something different on someone else's
machine.

```bash
python -m pytest cli/tests -q
```
