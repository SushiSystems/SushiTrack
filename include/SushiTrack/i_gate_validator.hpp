/**************************************************************************/
/* i_gate_validator.hpp                                                   */
/**************************************************************************/
/*                          This file is part of:                         */
/*                               SushiTrack                               */
/*               https://github.com/SushiSystems/SushiTrack               */
/*                         https://sushisystems.io                        */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the PolyForm Noncommercial License 1.0.0 (the           */
/* "License"); you may not use this file except in compliance with the    */
/* License. You may obtain a copy of the License at                       */
/*                                                                        */
/*     https://polyformproject.org/licenses/noncommercial/1.0.0           */
/*                                                                        */
/* Noncommercial use is free. Commercial use requires a separate licence  */
/* from Sushi Systems; see COMMERCIAL.md. The software is provided        */
/* "as is", without warranty of any kind.                                 */
/**************************************************************************/

/**
 * @file i_gate_validator.hpp
 * @brief Interface definition for search space gating components.
 * @author Mustafa Garip
 */

#pragma once

#include <SushiTrack/kalman_types.hpp>

#include <memory>
#include <vector>

namespace SushiTrack
{
    /**
     * @brief Abstract interface for search space gating.
     */
    class IGateValidator
    {
    public:
        /**
         * @brief Virtual destructor for the interface.
         */
        virtual ~IGateValidator() = default;

        /**
         * @brief Validates an observation against a predicted state distribution.
         * @param mean Predicted state mean vector.
         * @param covariance Predicted state covariance matrix.
         * @param measurement Input observation vector to validate.
         * @param chi2_thresh Chi-square threshold for the gate.
         * @return True if observation is within the validation gate.
         */
        virtual bool isInGate(const KalmanTypes::StateMean& mean, const KalmanTypes::StateCov& covariance,
                              const KalmanTypes::DetectBox& measurement, float chi2_thresh,
                              float score = 0.0f) const = 0;

        /**
         * @brief Performs batch validation for a collection of observations.
         * @param measurements Collection of input observation vectors.
         * @param chi2_thresh Chi-square threshold for the gate.
         * @return Collection of boolean flags indicating gate membership.
         */
        virtual std::vector<bool> getGateFlags(const KalmanTypes::StateMean& mean,
                                               const KalmanTypes::StateCov& covariance,
                                               const KalmanTypes::DetectBoxVector& measurements,
                                               float chi2_thresh,
                                               const std::vector<float>& scores = {}) const = 0;

        /**
         * @brief Computes squared distances for a collection of observations.
         * @param measurements Collection of input observation vectors.
         * @return Collection of computed squared Mahalanobis distances.
         */
        virtual std::vector<float> getGateSqDistances(const KalmanTypes::StateMean& mean,
                                                      const KalmanTypes::StateCov& covariance,
                                                      const KalmanTypes::DetectBoxVector& measurements,
                                                      const std::vector<float>& scores = {}) const
        {
            (void)mean; (void)covariance; (void)scores;
            return std::vector<float>(measurements.size(), 0.0f);
        }
    };

    /**
     * @brief Pass-through validator implementation that performs no filtering.
     */
    class NullGateValidator : public IGateValidator
    {
    public:
        /**
         * @brief Retrieves the singleton instance of NullGateValidator.
         * @return Shared pointer to static validator instance.
         */
        static std::shared_ptr<NullGateValidator> getInstance()
        {
            static std::shared_ptr<NullGateValidator> instance =
                std::make_shared<NullGateValidator>();
            return instance;
        }

        /**
         * @brief Validates an observation without restriction.
         * @return Always returns true.
         */
        bool isInGate(const KalmanTypes::StateMean& mean, const KalmanTypes::StateCov& covariance,
                      const KalmanTypes::DetectBox& measurement, float chi2_thresh,
                      float score = 0.0f) const override
        {
            (void)mean; (void)covariance; (void)measurement; (void)chi2_thresh; (void)score;
            return true;
        }

        /**
         * @brief Performs unrestricted batch validation.
         * @return Collection of true boolean flags.
         */
        std::vector<bool> getGateFlags(const KalmanTypes::StateMean& mean,
                                      const KalmanTypes::StateCov& covariance,
                                      const KalmanTypes::DetectBoxVector& measurements,
                                      float chi2_thresh,
                                      const std::vector<float>& scores = {}) const override
        {
            (void)mean; (void)covariance; (void)chi2_thresh; (void)scores;
            return std::vector<bool>(measurements.size(), true);
        }

        /**
         * @brief Computes zero distances for pass-through validation.
         * @return Collection of zeroed distance values.
         */
        std::vector<float> getGateSqDistances(const KalmanTypes::StateMean& mean,
                                             const KalmanTypes::StateCov& covariance,
                                             const KalmanTypes::DetectBoxVector& measurements,
                                             const std::vector<float>& scores = {}) const override
        {
            (void)mean; (void)covariance; (void)scores;
            return std::vector<float>(measurements.size(), 0.0f);
        }
    };

} // namespace SushiTrack