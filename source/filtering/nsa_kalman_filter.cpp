/**************************************************************************/
/* nsa_kalman_filter.cpp                                                  */
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
 * @file nsa_kalman_filter.cpp
 * @brief Defines the noise-scale-adaptive Kalman filter.
 * @author Mustafa Garip
 */

#include <SushiTrack/filtering/nsa_kalman_filter.hpp>

#include <algorithm>
#include <cmath>

#include <SushiTrack/filtering/kalman_filter_registry.hpp>

namespace SushiTrack
{
    using StateMean = KalmanTypes::StateMean;
    using StateCov  = KalmanTypes::StateCov;
    using DetectBox = KalmanTypes::DetectBox;
    using StateHMean = Eigen::Matrix<float, 1, 4, Eigen::RowMajor>;
    using StateHCov  = Eigen::Matrix<float, 4, 4, Eigen::RowMajor>;

    static KalmanFilterRegistrar<NSAKalmanFilter> reg_nsa(1);

    /**
     * @brief Initiates state mean and covariance with confidence-aware noise scaling.
     * @param measurement Input observation vector.
     * @param score Confidence score used for noise scaling.
     */
    void NSAKalmanFilter::initiate(StateMean& mean, StateCov& covariance,
                                    const DetectBox& measurement, float score)
    {
        mean.block<1, 4>(0, 0) = measurement.block<1, 4>(0, 0);
        mean.block<1, 4>(0, 4) = Eigen::RowVector4f::Zero();

        float noise_scale = (score >= 0.0f) ? std::max(1.0f - score, 1e-2f) : 1.0f;
        float linear_std_scale = std::sqrt(noise_scale);

        StateMean std_vec;
        if (enable_xyah_)
        {
            float height = std::max(1e-2f, measurement[3]);
            std_vec(0) = kInitPositionMult * kStdWeightPosition * height * linear_std_scale;
            std_vec(1) = kInitPositionMult * kStdWeightPosition * height * linear_std_scale;
            std_vec(2) = 1e-2f;
            std_vec(3) = kInitPositionMult * kStdWeightPosition * height * linear_std_scale;
            std_vec(4) = kInitVelocityMult * kStdWeightVelocity * height;
            std_vec(5) = kInitVelocityMult * kStdWeightVelocity * height;
            std_vec(6) = 1e-5f;
            std_vec(7) = kInitVelocityMult * kStdWeightVelocity * height;
        }
        else
        {
            float width  = std::max(1e-2f, measurement[2]);
            float height = std::max(1e-2f, measurement[3]);
            std_vec(0) = kInitPositionMult * kStdWeightPosition * width * linear_std_scale;
            std_vec(1) = kInitPositionMult * kStdWeightPosition * height * linear_std_scale;
            std_vec(2) = kInitPositionMult * kStdWeightPosition * width * linear_std_scale;
            std_vec(3) = kInitPositionMult * kStdWeightPosition * height * linear_std_scale;
            std_vec(4) = kInitVelocityMult * kStdWeightVelocity * width;
            std_vec(5) = kInitVelocityMult * kStdWeightVelocity * height;
            std_vec(6) = kInitVelocityMult * kStdWeightVelocity * width;
            std_vec(7) = kInitVelocityMult * kStdWeightVelocity * height;
        }

        covariance = std_vec.array().square().matrix().asDiagonal();
    }

    /**
     * @brief Projects the state distribution into measurement space with NSA scaling.
     * @param projected_mean Output projected mean vector.
     * @param projected_cov Output projected covariance matrix.
     * @param score Confidence score used for noise scaling.
     */
    void NSAKalmanFilter::project(Eigen::Matrix<float, 1, 4, Eigen::RowMajor>& projected_mean,
                                    Eigen::Matrix<float, 4, 4, Eigen::RowMajor>& projected_cov,
                                    const StateMean& mean,
                                    const StateCov&  covariance,
                                    float score) const
    {
        float width = std::max(1e-2f, mean(2));
        float height = std::max(1e-2f, mean(3));
        float noise_scale = (score >= 0.0f) ? std::max(1.0f - score, 1e-2f) : 1.0f;
        float linear_std_scale = std::sqrt(noise_scale);

        DetectBox std_vec;
        if (enable_xyah_)
        {
            std_vec << kStdWeightPosition * height * linear_std_scale,
                    kStdWeightPosition * height * linear_std_scale,
                    1e-1f,
                    kStdWeightPosition * height * linear_std_scale;
        }
        else
        {
            std_vec << kStdWeightPosition * width * linear_std_scale,
                    kStdWeightPosition * height * linear_std_scale,
                    kStdWeightPosition * width * linear_std_scale,
                    kStdWeightPosition * height * linear_std_scale;
        }

        projected_mean = (update_mat_ * mean.transpose()).transpose();
        projected_cov  = update_mat_ * covariance * update_mat_.transpose();

        Eigen::Matrix<float, 4, 4> diag = std_vec.asDiagonal();
        projected_cov += diag.array().square().matrix();
    }
} // namespace SushiTrack
