/****************************************************************/
/* nsa_kalman_filter.hpp                                        */
/* SushiTrack - https://github.com/SushiSystems/SushiTrack      */
/* Copyright (c) 2026 Sushi Systems                             */
/* Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.    */
/* Commercial use requires a licence from Sushi Systems.        */
/****************************************************************/

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
