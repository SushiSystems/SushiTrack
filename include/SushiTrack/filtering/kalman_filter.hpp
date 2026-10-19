/**************************************************************************/
/* kalman_filter.hpp                                                      */
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
 * @file kalman_filter.hpp
 * @brief Linear Kalman Filter implementation for motion state estimation.
 * @author Mustafa Garip
 */

#pragma once

#include <SushiTrack/filtering/i_kalman_filter.hpp>

#include <Eigen/Dense>
#include <mutex>
#include <vector>

namespace SushiTrack
{
    /**
     * @brief Motion estimation model based on discrete-time Kalman Filter.
     */
    class KalmanFilter : public IKalmanFilter
    {
    public:
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW

        /**
         * @brief Baseline noise constants matching reference tuning.
         */
        static constexpr float kStdWeightPosition = 1.0f / 20.0f;
        static constexpr float kStdWeightVelocity = 1.0f / 160.0f;
        static constexpr float kInitPositionMult  = 2.0f;
        static constexpr float kInitVelocityMult  = 10.0f;

        /**
         * @brief Initializes a Kalman Filter with the configured coordinate system.
         * @param enable_xyah Flag for coordinate system selection.
         */
        explicit KalmanFilter(bool enable_xyah = false);

        /**
         * @brief Virtual destructor for the interface.
         */
        ~KalmanFilter() override = default;

        /**
         * @brief Initializes state mean and covariance from initial observation.
         * @param mean Output state mean vector.
         * @param covariance Output state covariance matrix.
         * @param measurement Input initial observation vector.
         * @param score Detection confidence score.
         */
        void initiate(KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance,
                      const KalmanTypes::DetectBox& measurement, float score = 0.0f) override;

        /**
         * @brief Propagates state mean and covariance using motion model.
         * @param mean State mean vector to predict.
         * @param covariance State covariance matrix to predict.
         * @param dt Time interval for prediction.
         */
        void predict(KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance, float dt) override;

        /**
         * @brief Refines estimated state using a new observation.
         * @param mean State mean vector to update.
         * @param covariance State covariance matrix to update.
         * @param measurement Input observation vector.
         * @param score Detection confidence score.
         */
        void update(KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance,
                    const KalmanTypes::DetectBox& measurement, float score = 0.0f) override;

        /**
         * @brief Calculates squared Mahalanobis distance for an observation.
         * @param mean Current state mean.
         * @param covariance Current state covariance.
         * @param measurement Input observation vector.
         * @param score Detection score for noise scaling.
         * @return Computed squared Mahalanobis distance.
         */
        float mahalanobisDistance(const KalmanTypes::StateMean& mean, const KalmanTypes::StateCov& covariance,
                                  const KalmanTypes::DetectBox& measurement,
                                  float score = 0.0f) const;

        /**
         * @brief Validates if an observation falls within statistical gate.
         * @param chi2_thresh Chi-square threshold for gating.
         * @param score Optional detection score for noise scaling.
         * @return True if observation is within gate.
         */
        bool isInGate(const KalmanTypes::StateMean& mean, const KalmanTypes::StateCov& covariance,
                      const KalmanTypes::DetectBox& measurement, float chi2_thresh,
                      float score = 0.0f) const override;

        /**
         * @brief Performs batch gate validation for multiple observations.
         * @param measurements Collection of input observation vectors.
         * @param chi2_thresh Chi-square threshold for gating.
         * @return Collection of boolean flags indicating gate membership.
         */
        std::vector<bool> getGateFlags(const KalmanTypes::StateMean& mean,
                                       const KalmanTypes::StateCov& covariance,
                                       const KalmanTypes::DetectBoxVector& measurements,
                                       float chi2_thresh,
                                       const std::vector<float>& scores = {}) const override;

        /**
         * @brief Performs batch Mahalanobis distance calculations.
         * @param measurements Collection of input observation vectors.
         * @return Collection of computed squared Mahalanobis distances.
         */
        std::vector<float> getGateSqDistances(const KalmanTypes::StateMean& mean,
                                             const KalmanTypes::StateCov& covariance,
                                             const KalmanTypes::DetectBoxVector& measurements,
                                             const std::vector<float>& scores = {}) const override;

        /**
         * @brief Configures dynamic noise multipliers for process model.
         * @param pos_mult Position noise multiplier.
         * @param size_mult Size noise multiplier.
         * @param vel_mult Velocity noise multiplier.
         */
        void setNoiseMultipliers(float pos_mult, float size_mult, float vel_mult) override;

    protected:
        float dynamic_pos_mult_  = 1.0f;
        float dynamic_size_mult_ = 1.0f;
        float dynamic_vel_mult_  = 1.0f;

        bool  enable_xyah_;

        Eigen::Matrix<float, 8, 8, Eigen::RowMajor> motion_mat_;
        Eigen::Matrix<float, 4, 8, Eigen::RowMajor> update_mat_;

        using StateHMean = Eigen::Matrix<float, 1, 4, Eigen::RowMajor>;
        using StateHCov  = Eigen::Matrix<float, 4, 4, Eigen::RowMajor>;

        mutable const KalmanTypes::StateMean* cached_mean_ptr_ = nullptr;
        mutable const KalmanTypes::StateCov*  cached_cov_ptr_  = nullptr;
        mutable StateHMean             cached_projected_mean_;
        mutable Eigen::LLT<StateHCov>  cached_llt_;
        mutable bool llt_cache_valid_ = false;
        mutable std::mutex llt_cache_mutex_;

        /**
         * @brief Projects state distribution into measurement space.
         * @param projected_mean Output projected mean vector.
         * @param projected_cov Output projected covariance matrix.
         * @param score Confidence score of detection.
         */
        virtual void project(Eigen::Matrix<float, 1, 4, Eigen::RowMajor>& projected_mean,
                             Eigen::Matrix<float, 4, 4, Eigen::RowMajor>& projected_cov,
                             const KalmanTypes::StateMean& mean,
                             const KalmanTypes::StateCov& covariance,
                             float score = 0.0f) const;

        /**
         * @brief Retrieves cached LLT decomposition of projected covariance.
         * @param mean State mean vector.
         * @param covariance State covariance matrix.
         * @param out_projected_mean Output projected mean.
         * @return Reference to cached LLT solver.
         */
        const Eigen::LLT<StateHCov>& getCachedLLT(
            const KalmanTypes::StateMean& mean,
            const KalmanTypes::StateCov& covariance,
            StateHMean& out_projected_mean) const;

        /** @brief Resets the validation cache state. */
        inline void invalidateLLTCache() const
        {
            std::lock_guard<std::mutex> lock(llt_cache_mutex_);
            llt_cache_valid_ = false;
        }
    };

} // namespace SushiTrack