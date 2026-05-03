/**
 * @file      NSAKalmanFilter.h
 * @brief     Noise Scale Adaptive (NSA) Kalman Filter implementation.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      30.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#pragma once

#include "SushiTrack/KalmanFilter.h"

namespace SushiTrack
{
    /**
     * @brief Kalman Filter variant with adaptive noise scaling based on detection confidence.
     * 
     * Overrides state initiation and projection to apply confidence-based 
     * scaling to measurement noise, improving tracking stability for 
     * low-confidence detections.
     */
    class NSAKalmanFilter : public KalmanFilter
    {
        public:
            EIGEN_MAKE_ALIGNED_OPERATOR_NEW

            using KalmanFilter::KalmanFilter;

            /**
             * @brief Initializes the state mean and covariance with NSA scaling.
             * 
             * @param mean Output state mean vector.
             * @param covariance Output state covariance matrix.
             * @param measurement Input initial observation vector.
             * @param score Confidence score used for noise scaling.
             */
            void initiate(KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance,
                        const KalmanTypes::DetectBox& measurement, float score = 0.0f) override;

        protected:
            /**
             * @brief Projects the state distribution with confidence-based noise scaling.
             * 
             * @param projected_mean Output projected mean vector.
             * @param projected_cov Output projected covariance matrix.
             * @param mean Input state mean vector.
             * @param covariance Input state covariance matrix.
             * @param score Confidence score of the detection.
             */
            void project(Eigen::Matrix<float, 1, 4, Eigen::RowMajor>& projected_mean,
                        Eigen::Matrix<float, 4, 4, Eigen::RowMajor>& projected_cov,
                        const KalmanTypes::StateMean& mean, 
                        const KalmanTypes::StateCov& covariance, 
                        float score = 0.0f) const override;
    };

} // namespace SushiTrack
