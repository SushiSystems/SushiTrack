# Glossary

| Word | Meaning |
| --- | --- |
| Detection | One box a detector reports for one frame: top-left `x`, `y`, `width`, `height`, a score and a label, as `sushitrack_object_t` |
| Track | What the tracker reports for one object in one frame, as `sushitrack_track_t` |
| Tracklet | The tracker's internal record of one object across frames: estimator state, lifecycle state and feature history (`Tracklet`) |
| Track state | The lifecycle state of a tracklet: `New`, `Tracked`, `Lost`, `Removed` or `Tentative` |
| Tentative | A new track that is not yet confirmed; it needs `tentative_confirm_hits` matches before it is promoted |
| Lost buffer | The period a lost track is kept before removal: `track_buffer` frames, or `track_buffer / frame_rate` seconds when `time_aware=1` |
| Two-stage association | The ByteTrack scheme: detections at or above `track_thresh` are matched first, then the tracks left over try the low-score detections |
| Primary stage, secondary stage | The first and second pass of two-stage association |
| Cost calculator | An `CostCalculator`; it fills the matrix of pairwise costs between tracks and detections |
| IoU, DIoU, GIoU, SIoU | The four overlap measures `iou_type` selects for the primary and secondary stages |
| LAPJV | The Jonker-Volgenant algorithm for the linear assignment problem, used by `LAPJVSolver` |
| Gate | A test that removes a track and detection pair from association before the solver runs |
| Mahalanobis gating | A gate on the chi-squared distance between a detection and the track's predicted box, switched by `enable_mahalanobis` |
| NSA | Noise-scale-adaptive Kalman filter, `kalman_type=1`; it scales measurement noise by detection confidence |
| IMM | Interacting Multiple Model estimator, `kalman_type=2`; it runs four motion models and mixes them by mode probability |
| Kinematic state | The motion regime the IMM reports for a track: `Stationary`, `Cruising`, `Maneuvering` or `Coasting` |
| Maneuver probability | The IMM probability mass on the manoeuvring models, reported per track as `maneuver_probability` |
| OCM | Observation-centric momentum, `enable_ocm`; it re-ranks primary costs by the direction a track was observed to travel |
| VDC | Velocity direction consistency, the OC-SORT term OCM applies, bounded by `ocm_vdc_weight` |
| ORU | Observation-centric re-update, `enable_oru`; on recovery it rolls the estimator back and replays the gap along a straight line |
| ReID | Appearance matching on a feature vector supplied with each detection, switched by `enable_reid` |
| TLWH, XYAH | The two box states of the Kalman filter: top-left, width, height; or centre, aspect ratio, height (`enable_xyah`) |
| Time-aware | `time_aware=1`; `dt` is real seconds and expiry is measured in wall-clock time |
| Timing policy | An `TimingPolicy`; it turns `dt` into filter steps and decides when a track expires |
| C ABI | The binary interface of `include/SushiTrack/c_api/sushitrack_c.h`: the `sushitrack_*` functions and structs |
| Binding | `bindings/python/sushitrack.py`, the `ctypes` wrapper over the C ABI |
| Reference tracker | ByteTrack or OC-SORT as vendored under `third_party/`, built only to compare against |
| Adapter | A `TrackerAdapter` in `applications/tracker_bridge/`; it drives one tracker for the benchmark and the inference pipeline |
| `tracker_bridge` | The shared library built from `applications/tracker_bridge/tracker_bridge.cpp`; the inference pipeline loads it with ctypes |
| Benchmark | A folder under `applications/benchmarks/` with a `data/` folder of sequences: `mot17`, `mot20` or `dancetrack` |
| Sequence | One folder under a benchmark's `data/` with `det/det.txt`, `gt/gt.txt` and `seqinfo.ini` |
| Size | The label of a test, `small`, `medium` or `large`, taken from its suite's prefix; `st test --label` selects by it |
| Private, public detections | In `st eval`, the `-YOLOX` sequences and the other MOT sequences |
| TrackEval | The MOTChallenge evaluator under `third_party/TrackEval`; it computes HOTA, CLEAR and Identity metrics |
| HOTA, DetA, AssA | Higher-order tracking accuracy and its detection and association parts |
| MOTA, IDF1, IDSW | The CLEAR accuracy, the identity F1 score and the count of identity switches |
| Deploy package | The `package/` tree `st deploy` assembles for a consumer outside this repository |
| `st` | The developer CLI of this repository, also installed as `sushitrack` |
| `sushicore` | The Sushi Systems package that gives every Sushi CLI its console, configuration layering and build driver |

## Design topics

A topic is a module or a subject that spans modules, and it has one design document.
The list is generated in `docs/design/README.md`.

**SUSHITRACK_PROFILING**: SushiTrack records its frame, stages, counters and track events through SushiProfiler. See [the design](../design/SUSHITRACK_PROFILING/README.md).

**SUSHITRACK_MIGRATION**: The repository came from a project that carried another name, another licence and a layout of its own. See [the design](../design/SUSHITRACK_MIGRATION/README.md).
