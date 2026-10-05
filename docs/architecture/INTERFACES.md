# Interfaces

The runtime is organised around interfaces that the tracker depends on. Concrete implementations are wired up by the C-API entry point (`sushitrack_create_ex` in `source/sushitrack_c.cpp`) based on the `sushitrack_params_t` flags. The core orchestrator `Tracker` (see `include/SushiTrack/tracker.hpp`) consumes only the abstractions.

| Interface | Purpose | Concrete implementations |
|---|---|---|
| `IStatePredictor` | State propagation/update | `KalmanFilter`, `NSAKalmanFilter` |
| `IGateValidator` | Mahalanobis gating | `KalmanFilter`, `NSAKalmanFilter`, `NullGateValidator` |
| `IKalmanFilter` | Unified predictor + validator | `KalmanFilter`, `NSAKalmanFilter` |
| `IKalmanFactory` | Per-tracklet predictor factory | `GenericKalmanFactory` (registry-driven) |
| `ICostCalculator` | Pairwise association cost | `IoUCostCalculator`, `DIoUCostCalculator`, `GIoUCostCalculator`, `SIoUCostCalculator`, `CosineCostCalculator`, `ReIDFirstCostCalculator`, `PureReIDCostCalculator`, `FusedCostCalculator`, `GatedIoUCostCalculator`, `FuseScoreDecorator` |
| `IAssignmentSolver` | Bipartite assignment | `LAPJVSolver` (Jonker–Volgenant with active-row/column reduction) |
| `ITimingPolicy` | dt normalisation, expiry | `FrameTimingPolicy`, `WallClockTimingPolicy` |

`KalmanFilterRegistry` is a singleton plug-in registry. New Kalman variants register themselves through `KalmanFilterRegistrar<T>` and become selectable by `kalman_type`.

Track lifecycle is governed by `TrackState` (`New`, `Tracked`, `Lost`, `Removed`, `Tentative`). When the IMM estimator is selected (`kalman_type=2`), each track also reports a motion regime (`Stationary`, `Cruising`, `Maneuvering`, or `Coasting`) derived from the IMM mode probabilities, plus a continuous `maneuver_probability` on the output track.
