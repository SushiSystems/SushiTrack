/**************************************************************************/
/* nsa_kalman_filter.hpp                                                  */
/**************************************************************************/
/*                          This file is part of:                         */
/*                               SushiTrack                               */
/*              https://github.com/SushiSystems/SushiTrack                */
/*                          https://sushisystems.io                       */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                          */
/**************************************************************************/

/**
 * @file nsa_kalman_filter.hpp
 * @brief Noise Scale Adaptive (NSA) Kalman Filter implementation.
 * @author Mustafa Garip
 */

#pragma once

#include <SushiTrack/kalman_filter.hpp>

namespace SushiTrack
{
    /**
     * @brief Kalman Filter variant with adaptive noise scaling.
     */
    class NSAKalmanFilter : public KalmanFilter
    {
    public:
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW

        using KalmanFilter::KalmanFilter;

        /**
         * @brief Initializes state mean and covariance with NSA scaling.
         * @param mean Output state mean vector.
         * @param covariance Output state covariance matrix.
         * @param measurement Input initial observation vector.
         * @param score Confidence score used for noise scaling.
         */
        void initiate(KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance,
                      const KalmanTypes::DetectBox& measurement, float score = 0.0f) override;

    protected:
        /**
         * @brief Projects state distribution with confidence-based noise scaling.
         * @param projected_mean Output projected mean vector.
         * @param projected_cov Output projected covariance matrix.
         * @param score Confidence score of the detection.
         */
        void project(Eigen::Matrix<float, 1, 4, Eigen::RowMajor>& projected_mean,
                     Eigen::Matrix<float, 4, 4, Eigen::RowMajor>& projected_cov,
                     const KalmanTypes::StateMean& mean,
                     const KalmanTypes::StateCov& covariance,
                     float score = 0.0f) const override;
    };

} // namespace SushiTrack
