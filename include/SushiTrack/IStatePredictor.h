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
            virtual ~IStatePredictor() = default;

            /**
             * @brief Initializes the state mean and covariance from an initial observation.
             * 
             * @param mean Output state mean vector to be initialized.
             * @param covariance Output state covariance matrix to be initialized.
             * @param measurement Input initial observation vector.
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
    };

} // namespace SushiTrack
