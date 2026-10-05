# SushiTrack manual

SushiTrack is a C++17 multi-object tracking library built on the ByteTrack two-stage association
pipeline. Motion models, cost calculators, gating and the assignment solver sit behind
interfaces and are selected at run time through the C API. The repository also holds a Python
binding, the `st` developer CLI, a MOTChallenge and DanceTrack regression harness and a YOLOX
video inference pipeline.

## Getting started

| Document | Holds |
| --- | --- |
| [Installing](getting_started/INSTALL.md) | Requirements, the `st` CLI, the conda environment and the Docker image |
| [Building](getting_started/BUILD.md) | The CMake options, `st build` and where the artefacts land |

## Architecture

| Document | Holds |
| --- | --- |
| [Overview](architecture/OVERVIEW.md) | The per-frame pipeline, the motion estimators, the association stages and the track lifecycle |
| [Interfaces](architecture/INTERFACES.md) | Each interface the tracker depends on and its implementations |
| [Features](architecture/FEATURES.md) | What each optional feature does and which parameter switches it |
| [Project structure](architecture/PROJECT_STRUCTURE.md) | The tree, folder by folder |
| [The `st` package](architecture/CLI_PACKAGE.md) | The files of the CLI package and what each owns |

## Guides

| Document | Holds |
| --- | --- |
| [Test suites](guides/TESTING.md) | What the unit, integration and regression suites cover and how to run them |
| [Evaluation pipeline](guides/EVALUATION.md) | How `st eval` chains the regression binary and TrackEval |
| [Inference pipeline](guides/INFERENCE.md) | The YOLOX video pipeline behind `st infer` |
| [Deploy packages](guides/DEPLOY_PACKAGES.md) | The `package/` tree `st deploy` assembles for C, C++ and Python consumers |
| [Ultralytics integration](guides/ULTRALYTICS_INTEGRATION.md) | How to put SushiTrack behind `model.track()` |

## Reference

| Document | Holds |
| --- | --- |
| [Command line](reference/CLI.md) | Every `st` command, its flags and the exit codes |
| [C API](reference/C_API.md) | The entry points, status codes and track fields of `sushitrack_c.h` |
| [Configuration](reference/CONFIGURATION.md) | `sushitrack.json` and every tracker parameter with its default |
| [Data layout](reference/DATA_LAYOUT.md) | Where the regression harness reads sequences and writes predictions |
| [Benchmarks](reference/BENCHMARKS.md) | The recorded MOT17, MOT20 and DanceTrack runs |
| [Changelog](reference/CHANGELOG.md) | What changed, by release |
| [Glossary](reference/GLOSSARY.md) | The words these documents use |
| [Known issues](reference/KNOWN_ISSUES.md) | Defects that are recorded and not yet fixed |

## Project

| Document | Holds |
| --- | --- |
| [Contributing](CONTRIBUTING.md) | Code style, what a change must carry and how it lands |
| [Documentation style guide](DOCUMENTATION_STYLE_GUIDE.md) | How prose is written and where a document goes |
| [Design map](design/README.md) | Which design document covers which topic |
| [Remaining work](design/REMAINING_WORK.md) | The backlog |

The Python binding is described in [`bindings/python/README.md`](../bindings/python/README.md)
and the checkers in [`tools/README.md`](../tools/README.md).

Agent work folders are under `agent/`. Finished material is under `archive/` and is not edited.

## Licence

SushiTrack is free for non-commercial use under the PolyForm Noncommercial License 1.0.0;
[LICENSE](../LICENSE) is the binding text and [COMMERCIAL.md](../COMMERCIAL.md) says how a
company obtains a commercial licence. [NOTICE.md](../NOTICE.md) lists the third-party code, the
ported files and the benchmark data with their own licences.
