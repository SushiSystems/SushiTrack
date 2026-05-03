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
     * Calculates instantaneous velocity and acceleration from the state mean and 
     * transitions the FSM state using the configured thresholds.
     * 
     * @param mean Current state mean vector containing position and velocity.
     * @param is_lost Flag indicating if the tracklet is in the Lost state.
     * @param tracklet_len Number of consecutive frames the tracklet has been active.
     * @param dt Time interval elapsed since the previous update.
     */
    void TrackFSM::updateState(const KalmanTypes::StateMean& mean, bool is_lost,
                            size_t tracklet_len, float dt)
    {
        if (is_lost)
        {
            current_state_ = KinematicState::Coasting;
            return;
        }

        const float effective_dt = config_.time_aware ? dt : (1.0f / static_cast<float>(std::max(1, config_.frame_rate)));

        const float effective_fps = (effective_dt > 0.001f) 
                                    ? (1.0f / effective_dt)
                                    : static_cast<float>(std::max(1, config_.frame_rate));

        const float vx_per_sec = mean(4) * effective_fps;
        const float vy_per_sec = mean(5) * effective_fps;
        const float vel_residual_per_sec = std::sqrt(vx_per_sec * vx_per_sec + vy_per_sec * vy_per_sec);

        const float dvx = vx_per_sec - last_vx_per_sec_;
        const float dvy = vy_per_sec - last_vy_per_sec_;
        const float accel_per_sec2 = std::sqrt(dvx * dvx + dvy * dvy) * effective_fps;
        last_vx_per_sec_ = vx_per_sec;
        last_vy_per_sec_ = vy_per_sec;

        if (tracklet_len < kWarmupFrames)
        {
            current_state_ = KinematicState::Cruising;
            return;
        }

        if (vel_residual_per_sec < config_.fsm_stationary_vel_thresh)
        {
            current_state_ = KinematicState::Stationary;
        }
        else if (accel_per_sec2 > config_.fsm_maneuver_accel_thresh)
        {
            current_state_ = KinematicState::Maneuvering;
        }
        else
        {
            current_state_ = KinematicState::Cruising;
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
