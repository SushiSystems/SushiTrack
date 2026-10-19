/**************************************************************************/
/* kalman_filter.cpp                                                      */
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
/*                                                                        */
/* Holds code ported from ByteTrack-cpp (MIT), see NOTICE.md              */
/* Copyright (c) 2022 Vertical Beach (lp6m, medalotte)                    */
/* Copyright (c) 2021 Yifu Zhang                                          */
/**************************************************************************/

/**
 * @file kalman_filter.cpp
 * @brief Implements linear Kalman filter and noise scale adaptive variants.
 * @author Mustafa Garip
 */

#include <SushiTrack/filtering/kalman_filter.hpp>

#include <cmath>
#include <cstddef>

#include <SushiTrack/filtering/kalman_filter_registry.hpp>
#include <SushiTrack/logging/logger.hpp>
#include <SushiTrack/filtering/nsa_kalman_filter.hpp>

namespace SushiTrack
{
    static KalmanFilterRegistrar<KalmanFilter> reg_kf(0);
    using StateMean = KalmanTypes::StateMean;
    using StateCov  = KalmanTypes::StateCov;
    using DetectBox = KalmanTypes::DetectBox;
    using StateHMean = Eigen::Matrix<float, 1, 4, Eigen::RowMajor>;
    using StateHCov  = Eigen::Matrix<float, 4, 4, Eigen::RowMajor>;

    /**
     * @brief Initializes the Kalman filter with the configured coordinate system.
     * @param enable_xyah Flag for using [center_x, center_y, aspect_ratio, height] format.
     */
    KalmanFilter::KalmanFilter(bool enable_xyah)
        : enable_xyah_(enable_xyah)
    {
        motion_mat_.setIdentity();
        update_mat_.setIdentity();
    }

    /**
     * @brief Initiates state mean and covariance from an initial measurement.
     * @param mean Output state mean vector.
     * @param covariance Output state covariance matrix.
     * @param measurement Input observation vector.
     */
    void KalmanFilter::initiate(StateMean& mean, StateCov& covariance,
                                const DetectBox& measurement, float /*score*/)
    {
        mean.block<1, 4>(0, 0) = measurement.block<1, 4>(0, 0);
        mean.block<1, 4>(0, 4) = Eigen::RowVector4f::Zero();

        constexpr float k_pos_init = kInitPositionMult * kStdWeightPosition;
        constexpr float k_vel_init = kInitVelocityMult * kStdWeightVelocity;

        StateMean std_vec;
        if (enable_xyah_)
        {
            float height = std::max(1e-2f, measurement[3]);
            std_vec(0) = k_pos_init * height;
            std_vec(1) = k_pos_init * height;
            std_vec(2) = 1e-2f;
            std_vec(3) = k_pos_init * height;
            std_vec(4) = k_vel_init * height;
            std_vec(5) = k_vel_init * height;
            std_vec(6) = 1e-5f;
            std_vec(7) = k_vel_init * height;
        }
        else
        {
            float width  = std::max(1e-2f, measurement[2]);
            float height = std::max(1e-2f, measurement[3]);
            std_vec(0) = k_pos_init * width;
            std_vec(1) = k_pos_init * height;
            std_vec(2) = k_pos_init * width;
            std_vec(3) = k_pos_init * height;
            std_vec(4) = k_vel_init * width;
            std_vec(5) = k_vel_init * height;
            std_vec(6) = k_vel_init * width;
            std_vec(7) = k_vel_init * height;
        }

        covariance = std_vec.array().square().matrix().asDiagonal();
    }

    /**
     * @brief Propagates the state mean and covariance using the motion model.
     * @param mean Reference to state mean for prediction.
     * @param covariance Reference to state covariance for prediction.
     * @param dt Time interval for prediction.
     */
    void KalmanFilter::predict(StateMean& mean, StateCov& covariance, float dt)
    {
        Eigen::Matrix<float, 8, 8> local_motion_mat = motion_mat_;
        for (size_t i = 0; i < 4; ++i)
        {
            local_motion_mat(i, 4 + i) = dt;
        }

        StateMean std_vec;
        if (enable_xyah_)
        {
            float height = std::max(1e-2f, mean(3));
            std_vec(0) = dynamic_pos_mult_  * kStdWeightPosition * height * dt;
            std_vec(1) = dynamic_pos_mult_  * kStdWeightPosition * height * dt;
            std_vec(2) = 1e-2f * dt;
            std_vec(3) = dynamic_size_mult_ * kStdWeightPosition * height * dt;
            std_vec(4) = dynamic_vel_mult_  * kStdWeightVelocity * height * dt;
            std_vec(5) = dynamic_vel_mult_  * kStdWeightVelocity * height * dt;
            std_vec(6) = 1e-5f * dt;
            std_vec(7) = dynamic_vel_mult_  * kStdWeightVelocity * height * dt;
        }
        else
        {
            float width  = std::max(1e-2f, mean(2));
            float height = std::max(1e-2f, mean(3));
            std_vec(0) = dynamic_pos_mult_  * kStdWeightPosition * width  * dt;
            std_vec(1) = dynamic_pos_mult_  * kStdWeightPosition * height * dt;
            std_vec(2) = dynamic_size_mult_ * kStdWeightPosition * width  * dt;
            std_vec(3) = dynamic_size_mult_ * kStdWeightPosition * height * dt;
            std_vec(4) = dynamic_vel_mult_  * kStdWeightVelocity * width  * dt;
            std_vec(5) = dynamic_vel_mult_  * kStdWeightVelocity * height * dt;
            std_vec(6) = dynamic_vel_mult_  * kStdWeightVelocity * width  * dt;
            std_vec(7) = dynamic_vel_mult_  * kStdWeightVelocity * height * dt;
        }

        StateMean tmp = std_vec.array().square();
        StateCov  motion_cov = tmp.asDiagonal();

        dynamic_pos_mult_ = 1.0f;
        dynamic_size_mult_ = 1.0f;
        dynamic_vel_mult_ = 1.0f;

        mean        = (local_motion_mat * mean.transpose()).transpose();
        covariance.noalias() = (local_motion_mat * covariance * local_motion_mat.transpose()).eval();
        covariance += motion_cov;

        invalidateLLTCache();
    }

    /**
     * @brief Performs state correction based on an observation.
     * @param mean Reference to state mean for update.
     * @param covariance Reference to state covariance for update.
     * @param measurement Input observation vector.
     * @param score Detection confidence score.
     */
    void KalmanFilter::update(StateMean& mean, StateCov& covariance,
                                const DetectBox& measurement, float score)
    {
        StateHMean projected_mean;
        StateHCov  projected_cov;
        project(projected_mean, projected_cov, mean, covariance, score);

        Eigen::Matrix<float, 4, 8> B = (covariance * update_mat_.transpose()).transpose();

        auto llt = projected_cov.llt();
        if (llt.info() != Eigen::Success)
        {
            SUSHITRACK_LOG_WARN("[KalmanFilter] Matrix update conditioning failure. Regularizing covariance.");
            projected_cov.diagonal().array() += 1e-6f;
            llt = projected_cov.llt();
        }

        Eigen::Matrix<float, 8, 4> kalman_gain = llt.solve(B).transpose();
        Eigen::Matrix<float, 1, 4> innovation  = measurement - projected_mean;

        mean       = (mean.array() + (innovation * kalman_gain.transpose()).array()).matrix();
        covariance.noalias() -= kalman_gain * projected_cov * kalman_gain.transpose();

        invalidateLLTCache();
    }

    /**
     * @brief Retrieves the cached LLT decomposition for a projected covariance.
     * @param mean Current state mean.
     * @param covariance Current state covariance.
     * @param out_projected_mean Output projected mean vector.
     * @return Reference to the cached LLT solver instance.
     */
    const Eigen::LLT<StateHCov>& KalmanFilter::getCachedLLT(
        const StateMean& mean,
        const StateCov& covariance,
        StateHMean& out_projected_mean) const
    {
        std::lock_guard<std::mutex> lock(llt_cache_mutex_);

        // Validate cache by address; mutations call invalidateLLTCache() to clear validity.
        if (llt_cache_valid_ &&
            &mean == cached_mean_ptr_ && 
            &covariance == cached_cov_ptr_)
        {
            out_projected_mean = cached_projected_mean_;
            return cached_llt_;
        }

        StateHCov projected_cov;
        project(out_projected_mean, projected_cov, mean, covariance);

        cached_llt_ = projected_cov.llt();

        if (cached_llt_.info() != Eigen::Success)
        {
            SUSHITRACK_LOG_WARN("[KalmanFilter] Matrix projection conditioning failure. Regularizing covariance.");
            projected_cov.diagonal().array() += 1e-6f;
            cached_llt_ = projected_cov.llt();
        }

        cached_mean_ptr_       = &mean;
        cached_cov_ptr_        = &covariance;
        cached_projected_mean_ = out_projected_mean;
        llt_cache_valid_       = true;

        return cached_llt_;
    }

    /**
     * @brief Computes Mahalanobis distance between a state and an observation.
     * @param score Detection score (0.0 = legacy unscaled projection).
     * @return Computed squared Mahalanobis distance.
     */
    float KalmanFilter::mahalanobisDistance(const StateMean& mean,
                                            const StateCov& covariance,
                                            const DetectBox& measurement,
                                            float score) const
    {
        if (score > 0.0f)
        {
            StateHMean projected_mean;
            StateHCov  projected_cov;
            project(projected_mean, projected_cov, mean, covariance, score);

            auto llt = projected_cov.llt();
            if (llt.info() != Eigen::Success)
            {
                SUSHITRACK_LOG_WARN("[KalmanFilter] Score-aware gate conditioning failure. Regularizing covariance.");
                projected_cov.diagonal().array() += 1e-6f;
                llt = projected_cov.llt();
            }
            Eigen::Matrix<float, 1, 4> diff = measurement - projected_mean;
            Eigen::Matrix<float, 1, 4> z = llt.solve(diff.transpose()).transpose();
            return (diff * z.transpose())(0, 0);
        }

        StateHMean projected_mean;
        const auto& llt = getCachedLLT(mean, covariance, projected_mean);

        Eigen::Matrix<float, 1, 4> diff = measurement - projected_mean;
        Eigen::Matrix<float, 1, 4> z = llt.solve(diff.transpose()).transpose();
        return (diff * z.transpose())(0, 0);
    }

    /**
     * @brief Validates if a measurement lies within the statistical gate.
     * @param chi2_thresh Chi-squared threshold for gating.
     * @param score Detection score for confidence-aware projection.
     * @return True if the measurement is within the gate.
     */
    bool KalmanFilter::isInGate(const StateMean& mean,
                                const StateCov& covariance,
                                const DetectBox& measurement,
                                float chi2_thresh,
                                float score) const
    {
        return mahalanobisDistance(mean, covariance, measurement, score) <= chi2_thresh;
    }

    /**
     * @brief Computes gating flags for a collection of measurements.
     * @param chi2_thresh Chi-squared threshold for gating.
     * @param scores Optional per-measurement detection scores.
     * @return Collection of boolean flags indicating gate membership.
     */
    std::vector<bool> KalmanFilter::getGateFlags(const KalmanTypes::StateMean& mean,
                                                const KalmanTypes::StateCov& covariance,
                                                const KalmanTypes::DetectBoxVector& measurements,
                                                float chi2_thresh,
                                                const std::vector<float>& scores) const
    {
        std::vector<bool> flags;
        flags.reserve(measurements.size());

        const bool use_scores = (scores.size() == measurements.size());

        if (use_scores)
        {
            // Per-measurement projection so score-scaled noise applies during gating.
            for (size_t i = 0; i < measurements.size(); ++i)
            {
                StateHMean projected_mean;
                StateHCov  projected_cov;
                project(projected_mean, projected_cov, mean, covariance, scores[i]);

                auto llt = projected_cov.llt();
                if (llt.info() != Eigen::Success)
                {
                    projected_cov.diagonal().array() += 1e-6f;
                    llt = projected_cov.llt();
                }
                Eigen::Matrix<float, 1, 4> diff = measurements[i] - projected_mean;
                Eigen::Matrix<float, 1, 4> z = llt.solve(diff.transpose()).transpose();
                flags.push_back((diff * z.transpose())(0, 0) <= chi2_thresh);
            }
            return flags;
        }

        StateHMean projected_mean;
        const auto& llt = getCachedLLT(mean, covariance, projected_mean);

        for (const auto& measurement : measurements)
        {
            Eigen::Matrix<float, 1, 4> diff = measurement - projected_mean;
            Eigen::Matrix<float, 1, 4> z = llt.solve(diff.transpose()).transpose();
            float dist = (diff * z.transpose())(0, 0);
            flags.push_back(dist <= chi2_thresh);
        }

        return flags;
    }

    /**
     * @brief Computes squared Mahalanobis distances for a collection of measurements.
     * @param measurements Collection of input observation vectors.
     * @param scores Optional per-measurement detection scores.
     * @return Collection of squared Mahalanobis distances.
     */
    std::vector<float> KalmanFilter::getGateSqDistances(const KalmanTypes::StateMean& mean,
                                                       const KalmanTypes::StateCov& covariance,
                                                       const KalmanTypes::DetectBoxVector& measurements,
                                                       const std::vector<float>& scores) const
    {
        std::vector<float> dists;
        dists.reserve(measurements.size());

        const bool use_scores = (scores.size() == measurements.size());

        if (use_scores)
        {
            for (size_t i = 0; i < measurements.size(); ++i)
            {
                StateHMean projected_mean;
                StateHCov  projected_cov;
                project(projected_mean, projected_cov, mean, covariance, scores[i]);

                auto llt = projected_cov.llt();
                if (llt.info() != Eigen::Success)
                {
                    projected_cov.diagonal().array() += 1e-6f;
                    llt = projected_cov.llt();
                }
                Eigen::Matrix<float, 1, 4> diff = measurements[i] - projected_mean;
                Eigen::Matrix<float, 1, 4> z = llt.solve(diff.transpose()).transpose();
                dists.push_back((diff * z.transpose())(0, 0));
            }
            return dists;
        }

        StateHMean projected_mean;
        const auto& llt = getCachedLLT(mean, covariance, projected_mean);

        for (const auto& measurement : measurements)
        {
            Eigen::Matrix<float, 1, 4> diff = measurement - projected_mean;
            Eigen::Matrix<float, 1, 4> z = llt.solve(diff.transpose()).transpose();
            dists.push_back((diff * z.transpose())(0, 0));
        }

        return dists;
    }

    /**
     * @brief Configures multipliers for process noise adjustment.
     * @param pos_mult Multiplier for position noise.
     * @param size_mult Multiplier for size noise.
     * @param vel_mult Multiplier for velocity noise.
     */
    void KalmanFilter::setNoiseMultipliers(float pos_mult, float size_mult, float vel_mult)
    {
        dynamic_pos_mult_  = pos_mult;
        dynamic_size_mult_ = size_mult;
        dynamic_vel_mult_  = vel_mult;
    }

    /**
     * @brief Projects the state distribution into the measurement space.
     * @param projected_mean Output projected mean vector.
     * @param projected_cov Output projected covariance matrix.
     */
    void KalmanFilter::project(Eigen::Matrix<float, 1, 4, Eigen::RowMajor>& projected_mean,
                                Eigen::Matrix<float, 4, 4, Eigen::RowMajor>&  projected_cov,
                                const StateMean& mean,
                                const StateCov&  covariance,
                                float /*score*/) const
    {
        float width = std::max(1e-2f, mean(2));
        float height = std::max(1e-2f, mean(3));

        DetectBox std_vec;
        if (enable_xyah_)
        {
            std_vec << kStdWeightPosition * height,
                    kStdWeightPosition * height,
                    1e-1f,
                    kStdWeightPosition * height;
        }
        else
        {
            std_vec << kStdWeightPosition * width,
                    kStdWeightPosition * height,
                    kStdWeightPosition * width,
                    kStdWeightPosition * height;
        }

        projected_mean = (update_mat_ * mean.transpose()).transpose();
        projected_cov  = update_mat_ * covariance * update_mat_.transpose();

        Eigen::Matrix<float, 4, 4> diag = std_vec.asDiagonal();
        projected_cov += diag.array().square().matrix();
    }

} // namespace SushiTrack