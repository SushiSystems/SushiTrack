# SushiTrack Changelog

All notable changes to this project will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

No unreleased changes.

## [1.0.0] - 2026-07-21

First stable release.

### Added
- Interface-driven, dependency-injected tracking core (`SushiTrackTracker`) over the ByteTrack two-stage association pipeline.
- Stable C ABI (`SushiTrackC.h`) with JSON configuration loading (`sushitrack.json`), a `ctypes` Python binding, and `--deploy cpp` / `--deploy python` packaging.
- Motion estimators selectable by `kalman_type`: linear Kalman (0), Noise-Scale-Adaptive Kalman (1), and a four-model IMM (2, constant velocity + constant acceleration + near-constant position + coordinated turn with an unscented CT update), exposing `kinematic_state` and `maneuver_probability`.
- IoU cost variants (`iou_type`): IoU, DIoU, GIoU, SIoU, with score fusion (`fuse_score`) on the primary stage.
- Appearance ReID association (`enable_reid`): ReID-first, fused, and pure-ReID lost recovery paths.
- Mahalanobis gating (`enable_mahalanobis`) with regime-aware gate scaling.
- Tentative track lifecycle (`enable_tentative`) with adaptive, score-banded confirmation, frame-counted and wall-clock age-out, and new-track init suppression.
- Observation-centric momentum (`enable_ocm`) primary re-ranking and observation-centric re-update (`enable_oru`) on lost-track recovery.
- Frame-based and wall-clock (`time_aware`) timing policies.
- LAPJV assignment solver with reused workspace buffers.
- GoogleTest unit and integration suites, a MOTChallenge/DanceTrack regression harness with TrackEval, and a YOLOX video inference pipeline.

### Removed
- The per-track kinematic finite state machine (`TrackFSM`) and its `fsm_*` configuration, replaced by the IMM estimator (`kalman_type=2`).
