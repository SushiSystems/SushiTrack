/**
 * @file      TrackFSM.cpp
 * @brief     Implementation of the kinematic state machine for adaptive motion modeling.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#include "SushiTrack/TrackFSM.h"
#include <cmath>
#include <algorithm>

namespace SushiTrack
{
    /**
     * @brief Initializes the Kinematic Finite State Machine.
     * 
     * @param config Configuration parameters containing kinematic thresholds.
     */
    TrackFSM::TrackFSM(const TrackerConfig& config)
        : config_(config),
        current_state_(KinematicState::Stationary),
        last_vx_per_sec_(0.0f),
        last_vy_per_sec_(0.0f)
    { }

    /**
     * @brief Updates the kinematic state based on estimated velocity and acceleration.
     * 
     * Analyzes the state vector to determine the object's movement category.
     * Velocity is normalized to units-per-second using the reference frame rate,
     * and acceleration is computed as the derivative of velocity over the 
     * instantaneous time interval. Transitions use a hysteresis margin to 
     * prevent state flickering under detection noise.
     * 
     * @param mean Current state mean vector containing position and velocity.
     * @param is_lost Flag indicating if the tracklet is in the Lost state.
     * @param tracklet_len Number of consecutive frames the tracklet has been active.
     * @param dt Time interval elapsed since the previous update in seconds.
     */
    void TrackFSM::updateState(const KalmanTypes::StateMean& mean, bool is_lost,
                            size_t tracklet_len, float dt)
    {
        if (!config_.enable_fsm) return;

        if (is_lost)
        {
            // Capture the pre-Coasting motion regime so it can be restored on
            // recovery and consumed by lost-buffer / gating policies that need
            // to reason about the underlying motion category while the track is
            // dark. Only sample if we are not already Coasting, otherwise we
            // would clobber the captured state on every successive Lost frame.
            if (current_state_ != KinematicState::Coasting)
                pre_coast_state_ = current_state_;
            current_state_ = KinematicState::Coasting;
            return;
        }

        // ----- Time normalisation -----
        const float actual_dt = config_.time_aware ? dt : (1.0f / static_cast<float>(std::max(1, config_.frame_rate)));
        const float safe_dt = std::max(actual_dt, 0.001f);
        const float reference_fps = static_cast<float>(std::max(1, config_.frame_rate));

        // The Kalman state stores per-frame velocity; convert to per-second so
        // thresholds keep their physical meaning regardless of frame rate.
        const float vx_per_sec = mean(4) * reference_fps;
        const float vy_per_sec = mean(5) * reference_fps;
        const float vel_residual_per_sec = std::sqrt(vx_per_sec * vx_per_sec + vy_per_sec * vy_per_sec);

        // ----- Cold-start ghost-acceleration fix -----
        // Seed the velocity history on the very first call so the first
        // dvx/dvy is zero. Without this, a track born with non-zero velocity
        // would compute (full velocity - 0) / dt on its first acceleration
        // sample and immediately jump into Maneuvering once warmup ends.
        if (!has_velocity_history_)
        {
            last_vx_per_sec_ = vx_per_sec;
            last_vy_per_sec_ = vy_per_sec;
            has_velocity_history_ = true;
        }

        // ----- Coasting recovery -----
        // When an observation arrives after a Lost period, snap the velocity
        // history to the freshly observed velocity (otherwise the gap shows up
        // as a giant acceleration spike) AND restore the kinematic regime that
        // was active before the track went dark. A pedestrian who was
        // Maneuvering before brief occlusion comes back as Maneuvering, not
        // Cruising — preserving the noise / association policy that matched
        // their underlying motion.
        if (current_state_ == KinematicState::Coasting)
        {
            last_vx_per_sec_ = vx_per_sec;
            last_vy_per_sec_ = vy_per_sec;
            current_state_   = pre_coast_state_;
            pending_state_   = pre_coast_state_;
            pending_count_   = 0;
            return;
        }

        // ----- Acceleration estimate -----
        const float dvx = vx_per_sec - last_vx_per_sec_;
        const float dvy = vy_per_sec - last_vy_per_sec_;
        const float accel_per_sec2 = std::sqrt(dvx * dvx + dvy * dvy) / safe_dt;

        last_vx_per_sec_ = vx_per_sec;
        last_vy_per_sec_ = vy_per_sec;

        // ----- Warmup -----
        if (tracklet_len < kWarmupFrames)
        {
            current_state_ = KinematicState::Cruising;
            pending_state_ = KinematicState::Cruising;
            pending_count_ = 0;
            return;
        }

        // ----- Scale-invariant normalisation -----
        // Pixel velocity and acceleration scale with the object's apparent size:
        // a box twice as tall sweeps twice the pixels for the same body-relative
        // motion. Re-express both in "reference-height-equivalent pixels" so the
        // thresholds classify the object's *physical* motion regardless of how
        // large it appears in frame. At box height == fsm_reference_height the
        // factor is exactly 1, so the thresholds keep their absolute-pixel
        // calibration and the decision is identical to the un-normalised path.
        // A non-positive reference disables normalisation (legacy behaviour).
        // The factor is clamped so extreme box sizes cannot saturate the FSM.
        float scale = 1.0f;
        if (config_.fsm_reference_height > 0.0f)
        {
            const float height = std::max(1e-2f, mean(3));
            scale = std::clamp(config_.fsm_reference_height / height,
                               kMinScaleFactor, kMaxScaleFactor);
        }
        const float vel_eff   = vel_residual_per_sec * scale;
        const float accel_eff = accel_per_sec2 * scale;

        // ----- Threshold-based hysteresis (dead-band) -----
        const float margin_down = 0.8f;
        const float margin_up   = 1.2f;

        // Single-frame classification suggested by the current sample. Allows
        // direct Stationary <-> Maneuvering transitions (the previous state
        // machine had to pass through Cruising for a frame, which delayed
        // adaptation when a parked vehicle suddenly accelerated). Sustain
        // rules keep the active state sticky inside the dead-band.
        KinematicState observed;
        if (accel_eff > config_.fsm_maneuver_accel_thresh * margin_up)
        {
            observed = KinematicState::Maneuvering;
        }
        else if (vel_eff < config_.fsm_stationary_vel_thresh * margin_down)
        {
            observed = KinematicState::Stationary;
        }
        else if (current_state_ == KinematicState::Maneuvering &&
                 accel_eff >= config_.fsm_maneuver_accel_thresh * margin_down)
        {
            observed = KinematicState::Maneuvering;
        }
        else if (pending_state_ == KinematicState::Maneuvering &&
                 vel_eff > config_.fsm_stationary_vel_thresh * margin_up)
        {
            // Pending-sustain: a Maneuvering candidate was triggered by a large
            // velocity step. On subsequent frames the instantaneous acceleration
            // may drop to zero because the new velocity is now constant, but the
            // object is still clearly moving fast. Keep the Maneuvering
            // observation alive so the stability counter can reach kStableFrames
            // without being reset by a spurious Cruising classification.
            observed = KinematicState::Maneuvering;
        }
        else if (current_state_ == KinematicState::Stationary &&
                 vel_eff <= config_.fsm_stationary_vel_thresh * margin_up)
        {
            observed = KinematicState::Stationary;
        }
        else
        {
            observed = KinematicState::Cruising;
        }

        // ----- Time-based stability -----
        // A candidate transition must persist for kStableFrames consecutive
        // frames before it is committed. Eliminates one-frame state flicker
        // under detection jitter even when the threshold dead-band is crossed.
        if (observed == current_state_)
        {
            pending_state_ = observed;
            pending_count_ = 0;
            return;
        }

        if (observed == pending_state_)
        {
            if (pending_count_ < 255) pending_count_++;
        }
        else
        {
            pending_state_ = observed;
            pending_count_ = 1;
        }

        if (pending_count_ >= kStableFrames)
        {
            current_state_ = pending_state_;
            pending_count_ = 0;
        }
    }
    
    /**
     * @brief Adjusts state predictor noise parameters based on the current 
     * kinematic state.
     * 
     * @param kf Pointer to the state predictor instance to be configured.
     * @param dt Time interval for prediction (unused).
     */
    void TrackFSM::applyToPredictor(IStatePredictor* kf, float dt) const
    {
        if (!kf) return;

        (void)dt;

        switch (current_state_)
        {
            case KinematicState::Stationary:
                kf->setNoiseMultipliers(0.8f, 0.6f, 0.5f);
                break;

            case KinematicState::Cruising:
                kf->setNoiseMultipliers(1.0f, 1.0f, 1.0f);
                break;

            case KinematicState::Maneuvering:
                kf->setNoiseMultipliers(1.5f, 1.5f, 2.0f);
                break;

            case KinematicState::Coasting:
                kf->setNoiseMultipliers(1.5f, 1.5f, 1.0f);
                break;
        }
    }

} // namespace SushiTrack
