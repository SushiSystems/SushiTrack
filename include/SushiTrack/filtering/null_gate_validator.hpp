/**************************************************************************/
/* null_gate_validator.hpp                                                */
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
 * @file null_gate_validator.hpp
 * @brief Declares the gate validator that accepts every measurement.
 * @author Mustafa Garip
 */

#pragma once

#include <memory>
#include <vector>

#include <SushiTrack/filtering/i_gate_validator.hpp>

namespace SushiTrack
{
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
