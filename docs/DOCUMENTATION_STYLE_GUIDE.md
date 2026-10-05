# Documentation style guide

Prose in this repository follows the `humanizer` skill. Where a document goes, how it is named
and what a changelog entry looks like follow the `documentation` skill. Source comments follow
`source-comments`. None of the three is restated here.

## Where a fact goes

| Fact | Home |
| --- | --- |
| How the Python binding is used or finds its library | `bindings/python/README.md` |
| A checker and its settings | `tools/README.md` |
| A command, a flag or an exit code of `st` | `docs/reference/CLI.md` |
| A tracker parameter or its default | `docs/reference/CONFIGURATION.md` |
| How the pipeline, an estimator or the lifecycle works | `docs/architecture/OVERVIEW.md` |
| Work that is planned or deferred | `docs/design/REMAINING_WORK.md` |
| A defect that is known and open | `docs/reference/KNOWN_ISSUES.md` |

A page links to the page that owns a fact and does not repeat it.

## Paths and names

A cited file is written from the repository root in backticks, as in
`include/SushiTrack/tracker.hpp`, and must exist. Headers and sources are `snake_case`; the
test files under `tests/` are still `PascalCase` and are cited as they are named.

A parameter is written as its key in `sushitrack.json`, as in `kalman_type`. A command is
written as the user types it, as in `st test --suite unit`.

## Words

English, British spelling in prose ("licence" the noun, "normalise"). Identifiers keep the
spelling the code uses, such as `maneuver_probability` and `LICENSE`. The words the manual uses
with a fixed meaning are in the [glossary](reference/GLOSSARY.md).

## Numbers

A benchmark figure names its run. The recorded runs are in
[Benchmarks](reference/BENCHMARKS.md); a figure quoted anywhere else links there.
