# Remaining work

The single backlog of this repository. Defects that are open are listed in
[Known issues](../reference/KNOWN_ISSUES.md) and are not repeated here; this page holds the work
and the decisions that close them.

## Tracker

Three changes alter tracking output or the IMM internals. They wait for a benchmark re-run that
can validate them.

- Within-pool duplicate suppression. `removeDuplicateTracklets` deduplicates the tracked pool
  against the lost pool only. Running it on the tracked pool against itself would drop the
  same-object duplicate identities that occur in dense scenes.
- IMM mixture-covariance gating. IMM gating runs on the collapsed 8-D estimate through a
  stateless `KalmanFilter` helper. Gating on the mode-probability-weighted mixture innovation
  covariance would tighten the gate under a manoeuvre.
- Lazy coordinated-turn update. The IMM runs the sigma-point time update of the coordinated-turn
  model every frame. Gating it on the manoeuvre probability would cut the largest IMM cost when
  no track is turning.

## Migration leftovers

From [SushiTrack migration and rebranding](SUSHITRACK_MIGRATION.md), section 5.

- Add `.clang-format` and `.editorconfig` at the root.
- Make test discovery write under `build/` while the tests keep the repository root as their
  run directory, then drop the `cmake_test_discovery_*.json` ignore rules.

## Install and release

- Raise the `sushicore` pin in `environment.yml` to what `cli/pyproject.toml` requires, and give
  the `Dockerfile` the same source.
- Make `CMakeLists.txt` the one source of the version; generate the binding's and the CLI's
  numbers from it and drop `__version__ = "2.0.0"`.
- Propose the next release: `## Unreleased` holds more than the fifteen entries that trigger a
  proposal, and two of them break an interface: the Typer command layer and the licence.
- Add the CI workflow that runs the checkers, the tool tests, `st build` and `st test` on
  Windows and Linux.

## Documentation

- Close the 13 findings of `check_source_comments.py` (comment paragraphs, three long module
  docstrings, one history word); the estate programme on code does this. The checker does not
  test that every symbol carries a `@brief`.
- Check `docs/architecture/OVERVIEW.md`, `docs/reference/CLI.md` and
  `docs/reference/CONFIGURATION.md` against the sources line by line.
- Record the date, commit and machine of the next benchmark run in
  `docs/reference/BENCHMARKS.md`.
- Point the fix message in `cli/sushitrack_cli/services/checks.py` and the description in
  `cli/sushistack.deps.toml` at the pages that replaced the one-file manual.

## Open decisions

The owner decides each of these.

- Whether the migration is closed. If it is, `SUSHITRACK_MIGRATION.md` moves to
  `docs/archive/design/`.
- Whether `v1.0.0` was a release: tag the commit, or rename the changelog section.
- Where `bindings/`, the Docker files, `environment.yml` and the default `sushitrack.json`
  belong, since the layout names none of them at the root.
- Whether the headers move under `include/SushiTrack/<area>/`. The include paths are public
  interface.
- Whether `cli/` and `tests/` count as modules and carry their own `README.md`. Today the CLI
  package is described in `docs/architecture/CLI_PACKAGE.md` and the suites in
  `docs/guides/TESTING.md`.
- Whether the benchmark data leaves git (fetched on demand, or Git LFS) and a small fixture
  stays.
- Whether the evaluator and inference pipelines move out of `tests/regression/`, and whether
  the scripts in `cli/misc/` become `st` commands.
- Which of `AGENTS.md` and `CLAUDE.md` is the source for agent instructions; both state the
  same directives today.
- Whether contributions from outside are accepted, and under which contributor agreement.
