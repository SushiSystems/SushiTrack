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

        private:
            TrackerConfig  config_;        
            KinematicState current_state_; 
            float last_vx_per_sec_ = 0.0f; 
            float last_vy_per_sec_ = 0.0f; 

            static constexpr size_t kWarmupFrames = 3;
    };

} // namespace SushiTrack