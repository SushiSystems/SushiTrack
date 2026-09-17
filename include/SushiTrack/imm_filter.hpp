/**************************************************************************/
/* imm_filter.hpp                                                         */
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
 * @file imm_filter.hpp
 * @brief Interacting Multiple Model (IMM) state estimator.
 * @author Mustafa Garip
 */

#pragma once

#include <SushiTrack/i_kalman_filter.hpp>
#include <SushiTrack/kalman_filter.hpp>

#include <array>
#include <memory>

namespace SushiTrack
{
    /**
     * @brief Multi-model motion estimator that mixes parallel mode filters.
     *
     * Combines constant-velocity, constant-acceleration, coordinated-turn,
     * and near-constant-position modes through an IMM Markov mixing cycle.
     */
    class IMMFilter : public IKalmanFilter
    {
        public:
            EIGEN_MAKE_ALIGNED_OPERATOR_NEW

            /**
             * @brief Constructs the IMM estimator for the configured coordinate system.
             * @param enable_xyah Flag for [cx, cy, aspect, h] vs [x, y, w, h] format.
             */
            explicit IMMFilter(bool enable_xyah = false);

            ~IMMFilter() override = default;

            void initiate(KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance,
                          const KalmanTypes::DetectBox& measurement, float score = 0.0f) override;

            void predict(KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance, float dt) override;

            void update(KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance,
                        const KalmanTypes::DetectBox& measurement, float score = 0.0f) override;

            /** @brief No-op: per-mode process noise is intrinsic to the IMM model bank. */
            void setNoiseMultipliers(float pos_mult, float size_mult, float vel_mult) override;

            bool isInGate(const KalmanTypes::StateMean& mean, const KalmanTypes::StateCov& covariance,
                          const KalmanTypes::DetectBox& measurement, float chi2_thresh,
                          float score = 0.0f) const override;

            std::vector<bool> getGateFlags(const KalmanTypes::StateMean& mean,
                                           const KalmanTypes::StateCov& covariance,
                                           const KalmanTypes::DetectBoxVector& measurements,
                                           float chi2_thresh,
                                           const std::vector<float>& scores = {}) const override;

            std::vector<float> getGateSqDistances(const KalmanTypes::StateMean& mean,
                                                  const KalmanTypes::StateCov& covariance,
                                                  const KalmanTypes::DetectBoxVector& measurements,
                                                  const std::vector<float>& scores = {}) const override;

            /**
             * @brief Probability mass on the manoeuvring models (CA + CT).
             *
             * Continuous regime signal for association cost shaping and OCM weighting.
             */
            float maneuverProbability() const override { return maneuver_probability_; }

            /** @brief Probability mass on the near-constant-position model. */
            float stationaryProbability() const override { return stationary_probability_; }

            /** @brief Snapshots the full mode bank (states, covariances, probabilities) for ORU rollback. */
            void saveCheckpoint() override;

            /** @brief Restores the mode bank captured by the last saveCheckpoint(). */
            void restoreCheckpoint() override;

        private:
            /**
             * @brief Mode indices into the model bank.
             */
            enum Mode { CV = 0, CA = 1, NCP = 2, CT = 3 };
            static constexpr int kNumModes = 4;
            static constexpr int kDim = 11;   ///< [cx cy w h  vx vy vw vh  ax ay  omega]

            using Vec11 = Eigen::Matrix<float, kDim, 1>;
            using Mat11 = Eigen::Matrix<float, kDim, kDim>;
            using MatH  = Eigen::Matrix<float, 4, kDim>;   ///< Measurement map: state -> [cx, cy, w, h].
            using Mat4  = Eigen::Matrix<float, 4, 4>;

            bool  enable_xyah_;
            bool  initialized_ = false;
            float maneuver_probability_ = 0.0f;
            float stationary_probability_ = 0.0f;

            std::array<Vec11, kNumModes> x_;      ///< Per-mode state mean (11-D).
            std::array<Mat11, kNumModes> P_;      ///< Per-mode state covariance.
            Eigen::Matrix<float, kNumModes, 1> mu_;    ///< Posterior mode probabilities.
            Eigen::Matrix<float, kNumModes, 1> cbar_;  ///< Predicted mode probabilities (mixing normalisers); carried predict -> update.
            Eigen::Matrix<float, kNumModes, kNumModes> Pi_; ///< Markov mode-transition matrix (p_ij = i -> j).

            KalmanTypes::StateMean last_combined_; ///< Last combined 8-D mean written out; anchor for the delta re-sync of Tracklet's external pokes.

            std::unique_ptr<KalmanFilter> gate_kf_; ///< Stateless helper used only for gating on the combined estimate.

            /**
             * @brief Full mode-bank snapshot for ORU rollback.
             *
             * Captures internal state across all modes for prediction rollback.
             */
            struct Checkpoint
            {
                std::array<Vec11, kNumModes>       x;
                std::array<Mat11, kNumModes>       P;
                Eigen::Matrix<float, kNumModes, 1> mu;
                Eigen::Matrix<float, kNumModes, 1> cbar;
                KalmanTypes::StateMean             last_combined;
                bool                               initialized = false;
                bool                               valid       = false;
            };
            Checkpoint checkpoint_;

            MatH measurementMatrix() const;
            Mat4 measurementNoise(float height, float width) const;
            Mat11 transitionMatrix(Mode m, float dt) const;
            Vec11 processNoiseDiag(Mode m, float dt, float width, float height) const;

            /** @brief Nonlinear coordinated-turn time update for a single state. */
            Vec11 ctTransition(const Vec11& x, float dt) const;

            /** @brief Unscented (sigma-point) time update used by the coordinated-turn mode. */
            void ukfPredict(const Vec11& x0, const Mat11& P0, float dt,
                            Vec11& x_out, Mat11& P_out) const;

            /** @brief IMM step 1: mode-conditioned mixing of the prior estimates. */
            void mix(std::array<Vec11, kNumModes>& x0, std::array<Mat11, kNumModes>& P0) const;

            /** @brief IMM step 4: collapse the mode bank into the external 8-D estimate. */
            void combine(KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance);

            /** @brief Re-applies any external mutation of the shared 8-D mean onto every mode. */
            void reanchor(const KalmanTypes::StateMean& external_mean);

            static void symmetrize(Mat11& P) { P = 0.5f * (P + P.transpose().eval()); }
    };

} // namespace SushiTrack
