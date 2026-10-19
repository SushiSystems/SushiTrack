/**************************************************************************/
/* gate_validator.hpp                                                     */
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
 * @file gate_validator.hpp
 * @brief Declares the interface that gates a measurement against a predicted state.
 * @author Mustafa Garip
 */

#pragma once

#include <vector>

#include <SushiTrack/filtering/kalman_types.hpp>

namespace SushiTrack
{
    /**
     * @brief Abstract interface for search space gating.
     */
    class GateValidator
    {
        public:
            /**
             * @brief Virtual destructor for the interface.
             */
            virtual ~GateValidator() = default;

            /**
             * @brief Validates an observation against a predicted state distribution.
             * @param mean Predicted state mean vector.
             * @param covariance Predicted state covariance matrix.
             * @param measurement Input observation vector to validate.
             * @param chi2_thresh Chi-square threshold for the gate.
             * @return True if observation is within the validation gate.
             */
            [[nodiscard]] virtual bool is_in_gate(const KalmanTypes::StateMean& mean,
                                                  const KalmanTypes::StateCov& covariance,
                                                  const KalmanTypes::DetectBox& measurement,
                                                  float chi2_thresh, float score = 0.0f) const = 0;

            /**
             * @brief Performs batch validation for a collection of observations.
             * @param measurements Collection of input observation vectors.
             * @param chi2_thresh Chi-square threshold for the gate.
             * @return Collection of boolean flags indicating gate membership.
             */
            [[nodiscard]] virtual std::vector<bool>
            get_gate_flags(const KalmanTypes::StateMean& mean,
                           const KalmanTypes::StateCov& covariance,
                           const KalmanTypes::DetectBoxVector& measurements, float chi2_thresh,
                           const std::vector<float>& scores = {}) const = 0;

            /**
             * @brief Computes squared distances for a collection of observations.
             * @param measurements Collection of input observation vectors.
             * @return Collection of computed squared Mahalanobis distances.
             */
            [[nodiscard]] virtual std::vector<float>
            get_gate_sq_distances(const KalmanTypes::StateMean& mean,
                                  const KalmanTypes::StateCov& covariance,
                                  const KalmanTypes::DetectBoxVector& measurements,
                                  const std::vector<float>& scores = {}) const
            {
                (void)mean;
                (void)covariance;
                (void)scores;
                return std::vector<float>(measurements.size(), 0.0f);
            }
    };
} // namespace SushiTrack
