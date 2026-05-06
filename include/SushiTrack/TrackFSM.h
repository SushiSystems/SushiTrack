/**
 * @file      TrackFSM.h
 * @brief     Kinematic state machine for adaptive motion modeling.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#pragma once

#include "SushiTrack/KalmanTypes.h"
#include "SushiTrack/IStatePredictor.h"
#include "SushiTrack/TrackerConfig.h"

namespace SushiTrack
{
    /**
     * @brief Kinematic categories based on object motion analysis.
     */
    enum class KinematicState
    {
        Stationary,  ///< Velocity magnitude is below the defined stationary threshold.
        Cruising,    ///< Velocity is above stationary and acceleration is below maneuver threshold.
        Maneuvering, ///< Acceleration magnitude exceeds the defined maneuver threshold.
        Coasting     ///< State assigned during intervals without valid observations.
    };

    /**
     * @brief Manages kinematic state transitions and adapts prediction parameters.
     * 
     * Analyzes velocity and acceleration from the state vector to determine the 
     * current movement category. Adjusts noise multipliers of the state 
     * predictor based on the active kinematic state.
     */
    class TrackFSM
    {
        public:
            /**
             * @brief Initializes the kinematic state machine.
             * 
             * @param config Configuration parameters containing kinematic thresholds.
             */
            TrackFSM(const TrackerConfig& config);
            
            /**
             * @brief Destroys the TrackFSM instance.
             */
            ~TrackFSM() = default;

            /**
             * @brief Updates the kinematic state based on motion analysis.
             * 
             * Performs multi-stage analysis of the state vector to classify motion. 
             * Normalizes estimated velocity using the reference frame rate and 
             * evaluates acceleration using the derivative of velocity over time. 
             * Implements a hysteresis-based transition logic to stabilize 
             * state assignments under noisy detection conditions.
             * 
             * @param mean Current state mean vector containing position and velocity.
             * @param is_lost Flag indicating if the tracklet is in the Lost state.
             * @param tracklet_len Duration of the tracklet in frames.
             * @param dt Time interval elapsed since the previous update in seconds.
             */
            void updateState(const KalmanTypes::StateMean& mean, bool is_lost,
                            size_t tracklet_len = 0, float dt = 0.0f);

            /**
             * @brief Configures predictor noise based on the current kinematic state.
             * 
             * @param kf Target state predictor instance.
             * @param dt Time interval (unused for direct scaling).
             */
            void applyToPredictor(IStatePredictor* kf, float dt) const;

            /**
             * @brief Retrieves the active kinematic state.
             *
             * @return Current KinematicState enum value.
             */
            KinematicState getCurrentState() const { return current_state_; }

            /**
             * @brief Retrieves the last observed motion regime.
             *
             * When the track is currently Coasting (i.e. between observations) this
             * returns the kinematic state that was active immediately before the
             * track went dark. For all other states the active state itself is
             * returned. This is the canonical input for downstream policies that
             * must reason about the *true* motion regime regardless of whether
             * the track is presently being observed (e.g. lost-track buffer
             * scaling, association-stage gating).
             *
             * @return KinematicState recorded just before the most recent
             *         transition into Coasting, or the active state otherwise.
             */
            KinematicState getLastSeenState() const
            {
                return (current_state_ == KinematicState::Coasting)
                    ? pre_coast_state_
                    : current_state_;
            }

        private:
            TrackerConfig  config_;
            KinematicState current_state_;
            float last_vx_per_sec_ = 0.0f;
            float last_vy_per_sec_ = 0.0f;

            // Lazy-init flag for the velocity history. The first updateState call
            // seeds last_v* from the current velocity so the first dvx/dvy is zero
            // (instead of equal to the full velocity, which would otherwise produce
            // a spurious Maneuvering classification on born-with-velocity tracks).
            bool has_velocity_history_ = false;

            // Pre-Coasting motion regime, used to restore the active state when
            // an observation is reacquired after a Lost period. Without this, every
            // recovery would demote the track to Cruising regardless of what it
            // was doing pre-occlusion, losing kinematic context exactly when
            // adaptive behaviour matters most.
            KinematicState pre_coast_state_ = KinematicState::Cruising;

            // Time-based hysteresis: a candidate transition must be observed for
            // kStableFrames consecutive frames before it is committed. Single-frame
            // jitter cannot flip the state.
            KinematicState pending_state_ = KinematicState::Stationary;
            unsigned char  pending_count_ = 0;

            static constexpr size_t kWarmupFrames = 3;
            static constexpr unsigned char kStableFrames = 2;
    };

} // namespace SushiTrack