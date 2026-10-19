/**************************************************************************/
/* imm_filter.hpp                                                         */
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
 * @file imm_filter.hpp
 * @brief Interacting Multiple Model (IMM) state estimator.
 * @author Mustafa Garip
 */

#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include <SushiTrack/filtering/gated_state_predictor.hpp>
#include <SushiTrack/filtering/kalman_filter.hpp>

namespace SushiTrack
{
    /**
     * @brief Multi-model motion estimator that mixes parallel mode filters.
     *
     * Combines constant-velocity, constant-acceleration, coordinated-turn,
     * and near-constant-position modes through an IMM Markov mixing cycle.
     */
    class IMMFilter final : public GatedStatePredictor
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

            void predict(KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance,
                         float dt) override;

            void update(KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance,
                        const KalmanTypes::DetectBox& measurement, float score = 0.0f) override;

            /** @brief No-op: per-mode process noise is intrinsic to the IMM model bank. */
            void set_noise_multipliers(float pos_mult, float size_mult, float vel_mult) override;

            [[nodiscard]] bool is_in_gate(const KalmanTypes::StateMean& mean,
                                          const KalmanTypes::StateCov& covariance,
                                          const KalmanTypes::DetectBox& measurement,
                                          float chi2_thresh, float score = 0.0f) const override;

            [[nodiscard]] std::vector<bool>
            get_gate_flags(const KalmanTypes::StateMean& mean,
                           const KalmanTypes::StateCov& covariance,
                           const KalmanTypes::DetectBoxVector& measurements, float chi2_thresh,
                           const std::vector<float>& scores = {}) const override;

            [[nodiscard]] std::vector<float>
            get_gate_sq_distances(const KalmanTypes::StateMean& mean,
                                  const KalmanTypes::StateCov& covariance,
                                  const KalmanTypes::DetectBoxVector& measurements,
                                  const std::vector<float>& scores = {}) const override;

            /**
             * @brief Probability mass on the manoeuvring models (CA + CT).
             *
             * Continuous regime signal for association cost shaping and OCM weighting.
             */
            [[nodiscard]] float maneuver_probability() const override
            {
                return maneuver_probability_;
            }

            /** @brief Probability mass on the near-constant-position model. */
            [[nodiscard]] float stationary_probability() const override
            {
                return stationary_probability_;
            }

            /** @brief Snapshots the full mode bank (states, covariances, probabilities) for ORU rollback. */
            void save_checkpoint() override;

            /** @brief Restores the mode bank captured by the last save_checkpoint(). */
            void restore_checkpoint() override;

        private:
            /**
             * @brief Mode indices into the model bank.
             */
            enum class Mode : std::uint8_t
            {
                CV = 0,  /**< Constant velocity. */
                CA = 1,  /**< Constant acceleration. */
                NCP = 2, /**< Nearly constant position. */
                CT = 3,  /**< Coordinated turn. */
            };

            /// @brief Returns the position of a mode in the model bank.
            [[nodiscard]] static constexpr int index(Mode mode) noexcept
            {
                return static_cast<int>(mode);
            }
            static constexpr int K_NUM_MODES = 4;
            static constexpr int K_DIM = 11; ///< [cx cy w h  vx vy vw vh  ax ay  omega]

            using Vec11 = Eigen::Matrix<float, K_DIM, 1>;
            using Mat11 = Eigen::Matrix<float, K_DIM, K_DIM>;
            using MatH =
                Eigen::Matrix<float, 4, K_DIM>; ///< Measurement map: state -> [cx, cy, w, h].
            using Mat4 = Eigen::Matrix<float, 4, 4>;

            bool enable_xyah_;
            bool initialized_ = false;
            float maneuver_probability_ = 0.0f;
            float stationary_probability_ = 0.0f;

            std::array<Vec11, K_NUM_MODES> x_;        ///< Per-mode state mean (11-D).
            std::array<Mat11, K_NUM_MODES> p_;        ///< Per-mode state covariance.
            Eigen::Matrix<float, K_NUM_MODES, 1> mu_; ///< Posterior mode probabilities.
            Eigen::Matrix<float, K_NUM_MODES, 1>
                cbar_; ///< Predicted mode probabilities (mixing normalisers); carried predict -> update.
            Eigen::Matrix<float, K_NUM_MODES, K_NUM_MODES>
                pi_; ///< Markov mode-transition matrix (p_ij = i -> j).

            KalmanTypes::StateMean
                last_combined_; ///< Last combined 8-D mean written out; anchor for the delta re-sync of Tracklet's external pokes.

            std::unique_ptr<KalmanFilter>
                gate_kf_; ///< Stateless helper used only for gating on the combined estimate.

            /**
             * @brief Full mode-bank snapshot for ORU rollback.
             *
             * Captures internal state across all modes for prediction rollback.
             */
            struct Checkpoint
            {
                    std::array<Vec11, K_NUM_MODES> x;
                    std::array<Mat11, K_NUM_MODES> cov;
                    Eigen::Matrix<float, K_NUM_MODES, 1> mu;
                    Eigen::Matrix<float, K_NUM_MODES, 1> cbar;
                    KalmanTypes::StateMean last_combined;
                    bool initialized = false;
                    bool valid = false;
            };
            Checkpoint checkpoint_;

            [[nodiscard]] MatH measurement_matrix() const;
            [[nodiscard]] Mat4 measurement_noise(float height, float width) const;
            [[nodiscard]] Mat11 transition_matrix(Mode m, float dt) const;
            [[nodiscard]] Vec11 process_noise_diag(Mode m, float dt, float width,
                                                   float height) const;

            /** @brief Nonlinear coordinated-turn time update for a single state. */
            [[nodiscard]] Vec11 ct_transition(const Vec11& x, float dt) const;

            /** @brief Unscented (sigma-point) time update used by the coordinated-turn mode. */
            void ukf_predict(const Vec11& x0, const Mat11& prior_cov, float dt, Vec11& x_out,
                             Mat11& out_cov) const;

            /** @brief IMM step 1: mode-conditioned mixing of the prior estimates. */
            void mix(std::array<Vec11, K_NUM_MODES>& x0,
                     std::array<Mat11, K_NUM_MODES>& prior_cov) const;

            /** @brief IMM step 4: collapse the mode bank into the external 8-D estimate. */
            void combine(KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance);

            /** @brief Re-applies any external mutation of the shared 8-D mean onto every mode. */
            void reanchor(const KalmanTypes::StateMean& external_mean);

            static void symmetrize(Mat11& cov) { cov = 0.5f * (cov + cov.transpose().eval()); }
    };

} // namespace SushiTrack
