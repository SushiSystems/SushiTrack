/**************************************************************************/
/* in_memory_gate_validator.hpp                                           */
/**************************************************************************/
/*                          This file is part of:                         */
/*                               SushiTrack                               */
/*               https://github.com/SushiSystems/SushiTrack               */
/*                         https://sushisystems.io                        */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License"); you    */
/* may not use this file except in compliance with the License. You may   */
/* obtain a copy of the License at                                        */
/*                                                                        */
/*     https://www.apache.org/licenses/LICENSE-2.0                        */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/**************************************************************************/

/**
 * @file in_memory_gate_validator.hpp
 * @brief Declares the in-memory gate validator that gives one fixed verdict.
 * @author Mustafa Garip
 */

#pragma once

#include <vector>

#include <SushiTrack/filtering/gate_validator.hpp>

namespace SushiTrack
{
    /**
     * @brief In-memory GateValidator that accepts every measurement or rejects every one.
     *
     * It implements only what GateValidator leaves pure, so get_gate_sq_distances() is the
     * interface default.
     */
    class InMemoryGateValidator final : public GateValidator
    {
        public:
            /**
             * @brief Builds a validator with a fixed verdict.
             * @param accepts True to accept every measurement, false to reject every one.
             */
            explicit InMemoryGateValidator(bool accepts = true) noexcept : accepts_(accepts)
            {
            }

            /**
             * @brief Returns the fixed verdict for one measurement.
             * @return The verdict the validator was built with.
             */
            [[nodiscard]] bool is_in_gate(const KalmanTypes::StateMean& mean,
                                          const KalmanTypes::StateCov& covariance,
                                          const KalmanTypes::DetectBox& measurement,
                                          float chi2_thresh, float score = 0.0f) const override
            {
                (void)mean;
                (void)covariance;
                (void)measurement;
                (void)chi2_thresh;
                (void)score;
                return accepts_;
            }

            /**
             * @brief Returns the fixed verdict once per measurement.
             * @return One flag per measurement, each the verdict the validator was built with.
             */
            [[nodiscard]] std::vector<bool>
            get_gate_flags(const KalmanTypes::StateMean& mean,
                           const KalmanTypes::StateCov& covariance,
                           const KalmanTypes::DetectBoxVector& measurements, float chi2_thresh,
                           const std::vector<float>& scores = {}) const override
            {
                (void)mean;
                (void)covariance;
                (void)chi2_thresh;
                (void)scores;
                return std::vector<bool>(measurements.size(), accepts_);
            }

        private:
            /// @brief The verdict given for every measurement.
            bool accepts_;
    };
} // namespace SushiTrack
