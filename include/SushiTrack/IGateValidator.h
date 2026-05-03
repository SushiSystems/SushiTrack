/**
 * @file      IGateValidator.h
 * @brief     Interface definition for search space gating components.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#pragma once

#include "SushiTrack/KalmanTypes.h"
#include <vector>
#include <memory>

namespace SushiTrack
{
    /**
     * @brief Abstract interface for search space gating.
     *
     * Defines functional requirements for components validating observations 
     * against predicted states based on statistical distance thresholds.
     */
    class IGateValidator
    {
        public:
            virtual ~IGateValidator() = default;

            /**
             * @brief Validates a single observation against a predicted state distribution.
             *
             * @param mean Current predicted state mean vector.
             * @param covariance Current predicted state covariance matrix.
             * @param measurement Input observation vector to validate.
             * @param chi2_thresh Chi-square statistical threshold for the gate.
             * @return True if the observation is within the validation gate.
             */
            virtual bool isInGate(const KalmanTypes::StateMean& mean, const KalmanTypes::StateCov& covariance,
                                const KalmanTypes::DetectBox& measurement, float chi2_thresh) const = 0;

            /**
             * @brief Performs batch validation for a collection of observations.
             *
             * @param mean Current predicted state mean vector.
             * @param covariance Current predicted state covariance matrix.
             * @param measurements Collection of input observation vectors.
             * @param chi2_thresh Chi-square statistical threshold for the gate.
             * @return Collection of boolean flags indicating gate membership.
             */
            virtual std::vector<bool> getGateFlags(const KalmanTypes::StateMean& mean,
                                                const KalmanTypes::StateCov& covariance,
                                                const KalmanTypes::DetectBoxVector& measurements,
                                                float chi2_thresh) const = 0;

            /**
             * @brief Computes squared distances for a collection of observations.
             *
             * @param mean Current predicted state mean vector.
             * @param covariance Current predicted state covariance matrix.
             * @param measurements Collection of input observation vectors.
             * @return Collection of computed squared Mahalanobis distances.
             */
            virtual std::vector<float> getGateSqDistances(const KalmanTypes::StateMean& mean,
                                                        const KalmanTypes::StateCov& covariance,
                                                        const KalmanTypes::DetectBoxVector& measurements) const
            {
                std::vector<float> out(measurements.size(), 0.0f);
                return out;
            }
    };

    /**
     * @brief Pass-through validator implementation that performs no filtering.
     *
     * Note: Distance and flag calculations return non-restrictive dummy values 
     * (e.g., 0.0f distance, true flags). Used when search space gating is 
     * disabled. Implements a singleton pattern for efficient shared access.
     */
    class NullGateValidator : public IGateValidator
    {
        public:
            /**
             * @brief Retrieves the singleton instance of the NullGateValidator.
             *
             * The static local is initialised exactly once on first call and is
             * guaranteed thread-safe by the C++11 standard (N3337 §6.7/4).
             *
             * @return Shared pointer to the static validator instance.
             */
            static std::shared_ptr<NullGateValidator> getInstance()
            {
                static std::shared_ptr<NullGateValidator> instance =
                    std::make_shared<NullGateValidator>();
                return instance;
            }

            bool isInGate(const KalmanTypes::StateMean& mean, const KalmanTypes::StateCov& covariance,
                        const KalmanTypes::DetectBox& measurement, float chi2_thresh) const override
            {
                return true;
            }

            std::vector<bool> getGateFlags(const KalmanTypes::StateMean& mean,
                                        const KalmanTypes::StateCov& covariance,
                                        const KalmanTypes::DetectBoxVector& measurements,
                                        float chi2_thresh) const override
            {
                return std::vector<bool>(measurements.size(), true);
            }

            std::vector<float> getGateSqDistances(const KalmanTypes::StateMean& mean,
                                                const KalmanTypes::StateCov& covariance,
                                                const KalmanTypes::DetectBoxVector& measurements) const override
            {
                return std::vector<float>(measurements.size(), 0.0f);
            }
    };

} // namespace SushiTrack