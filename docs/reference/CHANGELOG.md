# Changelog

## Unreleased

- 2026-09-22 — build: Took `sushicore` from PyPI instead of a sibling checkout (`cli/pyproject.toml`).

- 2026-09-21 — docs: Added the upstream licence and a `NOTICE` to `third_party/bytetrack` and `third_party/ocsort`, and listed every vendored dependency in `docs/README.md`.

- 2026-09-17 — cli: Rewired `st` onto `sushicore`'s console/config layer, adding `cli/pyproject.toml`, `cli/config.toml`, and `cli/sushistack.deps.toml`.

## v1.0.0 — 2026-09-17

- 2026-09-17 — core: Released initial Apache-2.0 open-source version of SushiTrack tracking engine.
- 2026-09-17 — capi: Established sushitrack_ C ABI layer with unified error codes.
- 2026-09-17 — cli: Implemented unified developer toolchain under st and sushitrack.
- 2026-09-17 — bindings: Delivered native ctypes Python binding with full feature parity.
- 2026-09-17 — layout: Reorganized repository layout into single-module Sushi Systems structure.
