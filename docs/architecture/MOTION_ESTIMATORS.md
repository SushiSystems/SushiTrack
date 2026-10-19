# Motion estimators

The state predictors a tracklet can run, chosen by `kalman_type`, and the kinematic regime they
report. How they fit the per-frame pipeline is in [Overview](OVERVIEW.md).

All estimators expose an 8-D state to the tracklet and a 4-D measurement map.
The predictor is created per tracklet by the factory so each track owns its
estimator instance and any internal state.

## Linear Kalman filter

`KalmanFilter` (`source/filtering/kalman_filter.cpp`) is a constant-velocity filter over the
8-D state. Process and measurement noise scale with the dominant box dimension,
using the ByteTrack-calibrated compile-time constants
`K_STD_WEIGHT_POSITION = 1/20`, `K_STD_WEIGHT_VELOCITY = 1/160`,
`K_INIT_POSITION_MULT = 2.0`, `K_INIT_VELOCITY_MULT = 10.0`
(`include/SushiTrack/filtering/kalman_filter.hpp`). Process-noise standard deviations scale
linearly with `dt` (variance proportional to `dt^2`). `initiate` seeds the
position/size block from the measurement and zeroes velocity, with the initial
covariance widened by the init multipliers. `project` maps the state to
measurement space and adds measurement noise; the projected LLT decomposition is
cached (mutex-protected) and reused for gating within a frame. `predict` and
`update` invalidate the cache. `mahalanobis_distance` computes the squared
distance in 4-D measurement space; `is_in_gate`/`get_gate_flags`/`get_gate_sq_distances`
apply the chi-squared threshold.

Optional per-step process-noise multipliers exist (`set_noise_multipliers`) and
reset to 1.0 after each `predict`.

## NSA Kalman filter

`NSAKalmanFilter` (Noise-Scale-Adaptive, `kalman_type=1`) overrides `initiate`
and `project` to scale the position/size measurement-noise standard deviations
by `sqrt(max(1 - score, 1e-2))`. Higher-confidence detections therefore get
smaller measurement noise and a larger Kalman gain. Velocity components are not
scaled. All other behaviour is inherited from `KalmanFilter`.

## IMM estimator

`IMMFilter` (`source/filtering/imm_filter.cpp`, `kalman_type=2`) runs an Interacting Multiple
Model filter over a bank of four motion models. It presents the standard 8-D
`GatedStatePredictor` interface outward while carrying wider mode-specific sub-states
internally.

**Model bank** (`IMMFilter::Mode`):

- **CV**: constant velocity (the ByteTrack baseline dynamics).
- **CA**: constant acceleration (carries an acceleration pair).
- **NCP**: near-constant position (position random walk, damped velocity).
- **CT**: coordinated turn (nonlinear; carries a turn-rate state).

**Shared state layout.** Every mode lives in a fixed 11-D working space
`[cx, cy, w, h, vx, vy, vw, vh, ax, ay, omega]` with consistent slot semantics.
The size block `[w, h, vw, vh]` is a common constant-velocity sub-model in all
modes; only the centre kinematics differ. A mode pins the slots it does not own
to zero (CV/NCP: acceleration and turn rate; CA: turn rate; CT: acceleration),
so the mixing and combination math is model-agnostic, with no per-mode masking.

**IMM cycle** (per `predict`/`update`):

1. **Mixing.** Predicted mode probabilities c̄ⱼ = Σᵢ Π(i,j) · μᵢ;
   mixing weights μᵢ|ⱼ = Π(i,j) · μᵢ / c̄ⱼ; mixed mean/covariance per
   mode including the mode-spread term.
2. **Mode-matched predict.** Each mode propagates its mixed estimate with its
   own transition and process noise. Linear modes use their transition matrix;
   CT uses an unscented (sigma-point) time update. The measurement model is
   linear in all modes, so the correction is a standard Kalman update.
3. **Likelihood and probability update** (on an observation). Each mode produces
   a Gaussian innovation likelihood `N(nu; 0, S)` (evaluated in log space with
   the log-determinant and constant terms); the mode probabilities update to
   μⱼ ∝ c̄ⱼ · Λⱼ and normalize. On an unobserved frame the update
   is skipped and the probabilities relax to the predicted c̄.
4. **Combination.** The bank collapses to a single 8-D mean/covariance
   (probability-weighted, with the mode-spread term) written back to the
   tracklet.

**Transition prior.** The Markov transition matrix `Pi` keeps `K_P_STAY = 0.90`
probability of remaining in the current mode; the off-diagonal mass is weighted
toward CV so the stationary distribution is CV-heavy and the bank behaves like
the CV baseline until a manoeuvre likelihood spikes.

**Coordinated turn.** `ct_transition` rotates the centre velocity by
`omega * dt` and integrates the curved path, degenerating to a straight line as
`|omega| -> 0` (avoiding the `1/omega` singularity). `ukf_predict` uses the
symmetric sigma-point set with `lambda = 0` (`alpha = 1`, `kappa = 0`,
`beta = 2`).

**Delta re-anchor.** The tracklet may mutate the shared 8-D mean between calls
(the ByteTrack size-velocity freeze on unobserved tracks). Before each
`predict`, the IMM compares the incoming mean against the last combined mean it
wrote and applies the difference uniformly to every mode, keeping the bank
consistent with the tracklet's view without any tracklet-side change.

**Gating.** Gating runs on the combined estimate through an internal stateless
`KalmanFilter` helper, so the gate sees the same 4-D box projection as the
single-model path.

**Checkpoint.** `save_checkpoint`/`restore_checkpoint` snapshot and restore the
full mode bank (states, covariances, probabilities) for
[ORU rollback](OVERVIEW.md#observation-centric-re-update); the external 8-D projection is
lossy and cannot reconstruct the bank.

The IMM outputs a continuous `maneuver_probability` (mass on CA + CT), a
`stationary_probability` (mass on NCP), and a derived
[kinematic regime](#kinematic-regime).

---

## Kinematic regime

`KinematicState` (`include/SushiTrack/tracking/kinematic_state.hpp`) is a discrete
per-track regime: `Stationary`, `Cruising`, `Maneuvering`, `Coasting`. When an
IMM predictor drives the track, the regime is derived from the mode
probabilities: NCP mass above 0.5 reads Stationary, CA+CT mass above 0.5 reads
Maneuvering, otherwise Cruising; a track without a current observation reads
Coasting, and the pre-Coasting regime is retained for restoration on recovery.

When the predictor is not an IMM, `maneuver_probability()` returns a negative
sentinel; the tracklet's regime getters then fall back to Cruising, which makes
every regime-aware policy degrade to identity behaviour. The regime and the
continuous `maneuver_probability` feed:

- association cost shaping (per-row gate scale and appearance/geometry blend,
  `state_assoc_mods` in `source/association/state_association_modifiers.hpp`),
- the lost-buffer scale (`cleanup_lost`: Stationary 1.5×, Maneuvering 1.2×,
  others 1.0×),
- the OCM weight (Cruising full weight, Maneuvering scaled, Stationary/Coasting
  skipped),
- the C API output (`kinematic_state`, `maneuver_probability`).

---
