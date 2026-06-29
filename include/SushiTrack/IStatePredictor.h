/**
 * @file      IStatePredictor.h
 * @brief     Interface definition for motion models and state estimation.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#pragma once

#include "SushiTrack/KalmanTypes.h"

namespace SushiTrack
{
    /**
     * @brief Abstract interface for state estimation and motion modeling.
     * 
     * Defines the functional requirements for algorithms performing state 
     * prediction and measurement updates. Allows for interchangeable motion 
     * models within the tracking pipeline.
     */
    class IStatePredictor
    {
        public:
    /**
     * @brief Virtual destructor for the interface.
     */
    virtual ~IStatePredictor() = default;

            /**
             * @brief Initializes the state mean and covariance from an initial observation.
             * 
             * @param mean Output state mean vector to be initialized.
             * @param covariance Output state covariance matrix to be initialized.
             * @param measurement Input initial observation vector.
             * @param score Detection confidence score used for noise scaling.
             */
            virtual void initiate(KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance,
                                const KalmanTypes::DetectBox& measurement, float score = 0.0f) = 0;

            /**
             * @brief Propagates the state distribution to a future timestamp.
             * 
             * @param mean State mean vector to be predicted.
             * @param covariance State covariance matrix to be predicted.
             * @param dt Time interval for state propagation.
             */
            virtual void predict(KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance, float dt) = 0;

            /**
             * @brief Corrects the predicted state distribution using a new observation.
             * 
             * @param mean State mean vector to be updated.
             * @param covariance State covariance matrix to be updated.
             * @param measurement Input observation vector for state refinement.
             * @param score Detection confidence score used for noise scaling.
             */
            virtual void update(KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance,
                                const KalmanTypes::DetectBox& measurement, float score = 0.0f) = 0;

            /**
             * @brief Adjusts process noise multipliers for dynamic uncertainty scaling.
             * 
             * @param pos_mult Multiplier for position uncertainty.
             * @param size_mult Multiplier for dimension uncertainty.
             * @param vel_mult Multiplier for velocity uncertainty.
             */
            virtual void setNoiseMultipliers(float pos_mult, float size_mult, float vel_mult) = 0;

            /**
             * @brief Probability that the target is manoeuvring.
             *
             * Multi-model estimators (IMM) report the mass on their manoeuvring
             * models as a continuous [0, 1] regime signal that replaces the
             * prior hard Maneuvering category for downstream policies.
             * Single-model predictors return a negative sentinel meaning "no
             * regime estimate available", so callers fall back to their legacy
             * behaviour. Default: not available.
             */
            virtual float maneuverProbability() const { return -1.0f; }

            /**
             * @brief Probability that the target is near-stationary.
             *
             * Companion to maneuverProbability(): the mass an IMM places on its
             * near-constant-position model. Negative when unavailable.
             */
            virtual float stationaryProbability() const { return -1.0f; }
    };

} // namespace SushiTrack
