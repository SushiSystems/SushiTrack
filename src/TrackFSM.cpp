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
            // Capture the pre-Coasting regime once (skip if already Coasting) so recovery can restore the underlying motion category.
            if (current_state_ != KinematicState::Coasting)
                pre_coast_state_ = current_state_;
            current_state_ = KinematicState::Coasting;
            return;
        }

        const float actual_dt = config_.time_aware ? dt : (1.0f / static_cast<float>(std::max(1, config_.frame_rate)));
        const float safe_dt = std::max(actual_dt, 0.001f);
        const float reference_fps = static_cast<float>(std::max(1, config_.frame_rate));

        // Kalman velocity is per-frame; convert to per-second so thresholds stay frame-rate-independent.
        const float vx_per_sec = mean(4) * reference_fps;
        const float vy_per_sec = mean(5) * reference_fps;
        const float vel_residual_per_sec = std::sqrt(vx_per_sec * vx_per_sec + vy_per_sec * vy_per_sec);

        // Seed velocity history on the first call so the first accel sample is zero, not a born-with-velocity spike into Maneuvering.
        if (!has_velocity_history_)
        {
            last_vx_per_sec_ = vx_per_sec;
            last_vy_per_sec_ = vy_per_sec;
            has_velocity_history_ = true;
        }

        // On recovery from Lost, snap velocity history to the new observation (avoids a false accel spike) and restore the pre-coast regime.
        if (current_state_ == KinematicState::Coasting)
        {
            last_vx_per_sec_ = vx_per_sec;
            last_vy_per_sec_ = vy_per_sec;
            current_state_   = pre_coast_state_;
            pending_state_   = pre_coast_state_;
            pending_count_   = 0;
            return;
        }

        const float dvx = vx_per_sec - last_vx_per_sec_;
        const float dvy = vy_per_sec - last_vy_per_sec_;
        const float accel_per_sec2 = std::sqrt(dvx * dvx + dvy * dvy) / safe_dt;

        last_vx_per_sec_ = vx_per_sec;
        last_vy_per_sec_ = vy_per_sec;

        if (tracklet_len < kWarmupFrames)
        {
            current_state_ = KinematicState::Cruising;
            pending_state_ = KinematicState::Cruising;
            pending_count_ = 0;
            return;
        }

        // Rescale velocity/accel to reference-height-equivalent pixels so classification tracks physical motion, not apparent size; factor 1 at ref height, clamped, disabled when ref <= 0.
        float scale = 1.0f;
        if (config_.fsm_reference_height > 0.0f)
        {
            const float height = std::max(1e-2f, mean(3));
            scale = std::clamp(config_.fsm_reference_height / height,
                               kMinScaleFactor, kMaxScaleFactor);
        }
        const float vel_eff   = vel_residual_per_sec * scale;
        const float accel_eff = accel_per_sec2 * scale;

        // Threshold-based hysteresis (dead-band)
        const float margin_down = 0.8f;
        const float margin_up   = 1.2f;

        // Single-frame classification; allows direct Stationary<->Maneuvering (the old FSM forced a Cruising frame between, delaying adaptation).
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
            // Pending-sustain: keep a fast-but-now-constant-velocity candidate Maneuvering so accel falling to zero doesn't reset the stability counter.
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

        // A candidate transition must persist kStableFrames consecutive frames before commit, killing one-frame flicker under jitter.
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
                kf->setNoiseMultipliers(1.0f, 1.0f, 1.0f);
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
