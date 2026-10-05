/****************************************************************/
/* i_state_predictor.hpp                                        */
/* SushiTrack - https://github.com/SushiSystems/SushiTrack      */
/* Copyright (c) 2026 Sushi Systems                             */
/* Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.    */
/* Commercial use requires a licence from Sushi Systems.        */
/****************************************************************/

/**
 * @file i_state_predictor.hpp
 * @brief Interface definition for motion models and state estimation.
 * @author Mustafa Garip
 */

#pragma once

#include <SushiTrack/kalman_types.hpp>

namespace SushiTrack
{
    /**
     * @brief Abstract interface for state estimation and motion modeling.
     */
    class IStatePredictor
    {
    public:
        /**
         * @brief Virtual destructor for the interface.
         */
        virtual ~IStatePredictor() = default;

        /**
         * @brief Initializes state mean and covariance from an initial observation.
         * @param mean Output state mean vector to be initialized.
         * @param covariance Output state covariance matrix to be initialized.
         * @param measurement Input initial observation vector.
         * @param score Detection confidence score used for noise scaling.
         */
        virtual void initiate(KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance,
                              const KalmanTypes::DetectBox& measurement, float score = 0.0f) = 0;

        /**
         * @brief Propagates the state distribution to a future timestamp.
         * @param mean State mean vector to be predicted.
         * @param covariance State covariance matrix to be predicted.
         * @param dt Time interval for state propagation.
         */
        virtual void predict(KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance, float dt) = 0;

        /**
         * @brief Corrects predicted state distribution using a new observation.
         * @param mean State mean vector to be updated.
         * @param covariance State covariance matrix to be updated.
         * @param measurement Input observation vector for state refinement.
         * @param score Detection confidence score used for noise scaling.
         */
        virtual void update(KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance,
                            const KalmanTypes::DetectBox& measurement, float score = 0.0f) = 0;

        /**
         * @brief Adjusts process noise multipliers for dynamic uncertainty scaling.
         * @param pos_mult Multiplier for position uncertainty.
         * @param size_mult Multiplier for dimension uncertainty.
         * @param vel_mult Multiplier for velocity uncertainty.
         */
        virtual void setNoiseMultipliers(float pos_mult, float size_mult, float vel_mult) = 0;

        /**
         * @brief Returns probability that the target is manoeuvring in range [0, 1].
         * @return Maneuver likelihood, or negative sentinel when unavailable.
         */
        virtual float maneuverProbability() const { return -1.0f; }

        /**
         * @brief Returns probability that the target is near-stationary.
         * @return Stationary likelihood, or negative sentinel when unavailable.
         */
        virtual float stationaryProbability() const { return -1.0f; }

        /**
         * @brief Snapshots the estimator's internal state into a private buffer.
         */
        virtual void saveCheckpoint() {}

        /**
         * @brief Restores the internal state saved by the last saveCheckpoint().
         */
        virtual void restoreCheckpoint() {}
    };

} // namespace SushiTrack
