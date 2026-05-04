/**
 * @file      KalmanFilter.h
 * @brief     Linear Kalman Filter implementation for motion state estimation.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#pragma once

#include "SushiTrack/IKalmanFilter.h"
#include <Eigen/Dense>
#include <mutex>

namespace SushiTrack
{
    /**
     * @brief Motion estimation model based on the discrete-time Kalman Filter algorithm.
     *
     * Implements state prediction and measurement update cycles for an 
     * 8-dimensional state vector. Includes gating logic based on Mahalanobis 
     * distance for observation association.
     */
    class KalmanFilter : public IKalmanFilter
    {
        public:
            EIGEN_MAKE_ALIGNED_OPERATOR_NEW

            /**
             * @brief Baseline noise constants matching the reference ByteTrack tuning.
             *
             * These values replace the previous configurable hyperparameters
             * (std_weight_position / std_weight_velocity / kf_*_weight). They
             * are calibrated against the ByteTrack reference implementation
             * and exposing them as user knobs offered no measurable benefit
             * across the MOT17 / MOT20 benchmarks.
             */
            static constexpr float kStdWeightPosition = 1.0f / 20.0f;
            static constexpr float kStdWeightVelocity = 1.0f / 160.0f;
            static constexpr float kInitPositionMult  = 2.0f;
            static constexpr float kInitVelocityMult  = 10.0f;

            /**
             * @brief Initializes a Kalman Filter with the configured coordinate system.
             *
             * @param enable_xyah Flag for coordinate system selection.
             */
            explicit KalmanFilter(bool enable_xyah = false);

            ~KalmanFilter() override = default;

            /**
             * @brief Initializes the state mean and covariance from an initial observation.
             * 
             * @param mean Output state mean vector.
             * @param covariance Output state covariance matrix.
             * @param measurement Input initial observation vector.
             */
            void initiate(KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance,
                        const KalmanTypes::DetectBox& measurement, float score = 0.0f) override;

            /**
             * @brief Propagates the state mean and covariance using the motion model.
             *
             * @param mean State mean vector to be predicted.
             * @param covariance State covariance matrix to be predicted.
             * @param dt Time interval for prediction.
             */
            void predict(KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance, float dt) override;

            /**
             * @brief Refines the estimated state using a new observation.
             *
             * @param mean State mean vector to be updated.
             * @param covariance State covariance matrix to be updated.
             * @param measurement Input observation vector.
             */
            void update(KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance,
                        const KalmanTypes::DetectBox& measurement, float score = 0.0f) override;

            /**
             * @brief Calculates the squared Mahalanobis distance for an observation.
             *
             * @param mean Current state mean.
             * @param covariance Current state covariance.
             * @param measurement Input observation vector.
             * @return Computed squared Mahalanobis distance.
             */
            float mahalanobisDistance(const KalmanTypes::StateMean& mean, const KalmanTypes::StateCov& covariance,
                                     const KalmanTypes::DetectBox& measurement) const;

            /**
             * @brief Validates if an observation falls within a statistical gate.
             *
             * @param mean Current state mean.
             * @param covariance Current state covariance.
             * @param measurement Input observation vector.
             * @param chi2_thresh Chi-square threshold for gating.
             * @return True if the observation is within the gate.
             */
            bool isInGate(const KalmanTypes::StateMean& mean, const KalmanTypes::StateCov& covariance,
                        const KalmanTypes::DetectBox& measurement, float chi2_thresh) const override;

            /**
             * @brief Performs batch gate validation for multiple observations.
             * 
             * @param mean Current state mean.
             * @param covariance Current state covariance.
             * @param measurements Collection of input observation vectors.
             * @param chi2_thresh Chi-square threshold for gating.
             * @return Collection of boolean flags indicating gate membership.
             */
            std::vector<bool> getGateFlags(const KalmanTypes::StateMean& mean,
                                         const KalmanTypes::StateCov& covariance,
                                         const KalmanTypes::DetectBoxVector& measurements,
                                         float chi2_thresh) const override;

            /**
             * @brief Performs batch Mahalanobis distance calculations.
             * 
             * @param mean Current state mean.
             * @param covariance Current state covariance.
             * @param measurements Collection of input observation vectors.
             * @return Collection of computed squared Mahalanobis distances.
             */
            std::vector<float> getGateSqDistances(const KalmanTypes::StateMean& mean,
                                                 const KalmanTypes::StateCov& covariance,
                                                 const KalmanTypes::DetectBoxVector& measurements) const override;

            /**
             * @brief Configures dynamic noise multipliers for the process model.
             * 
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

            using StateHMean = Eigen::Matrix<float, 1, 4, Eigen::RowMajor>; ///< Alias for projected state mean in measurement space.
            using StateHCov  = Eigen::Matrix<float, 4, 4, Eigen::RowMajor>; ///< Alias for projected state covariance in measurement space.

            mutable const KalmanTypes::StateMean* cached_mean_ptr_ = nullptr; ///< Pointer to last processed mean for cache validation.
            mutable const KalmanTypes::StateCov*  cached_cov_ptr_  = nullptr;  ///< Pointer to last processed covariance for cache validation.
            mutable StateHMean             cached_projected_mean_; ///< Cached projected mean for the validated state.
            mutable Eigen::LLT<StateHCov>  cached_llt_;            ///< Cached LLT decomposition of projected covariance.
            mutable bool llt_cache_valid_ = false;                 ///< State flag for the validation cache.
            mutable std::mutex llt_cache_mutex_;                   ///< Mutex protecting validation cache access.

            /**
             * @brief Projects the state distribution into the measurement space.
             * 
             * @param projected_mean Output projected mean vector.
             * @param projected_cov Output projected covariance matrix.
             * @param mean Input state mean vector.
             * @param covariance Input state covariance matrix.
             * @param score Confidence score of the detection.
             */
            virtual void project(Eigen::Matrix<float, 1, 4, Eigen::RowMajor>& projected_mean,
                                Eigen::Matrix<float, 4, 4, Eigen::RowMajor>& projected_cov,
                                const KalmanTypes::StateMean& mean, 
                                const KalmanTypes::StateCov& covariance, 
                                float score = 0.0f) const;

            /**
             * @brief Retrieves or recomputes the validated LLT decomposition of the projected covariance.
             * @param mean State mean vector.
             * @param covariance State covariance matrix.
             * @param out_projected_mean Output projected mean.
             * @return Reference to the cached LLT solver.
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