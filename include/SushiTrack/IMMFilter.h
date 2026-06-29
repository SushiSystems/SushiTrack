/**
 * @file      IMMFilter.h
 * @brief     Interacting Multiple Model (IMM) state estimator (kalman_type = 2).
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      29.06.2026
 * @version   v0.1.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#pragma once

#include "SushiTrack/IKalmanFilter.h"
#include "SushiTrack/KalmanFilter.h"

#include <array>
#include <memory>

namespace SushiTrack
{
    /**
     * @brief Multi-model motion estimator that mixes parallel mode filters.
     *
     * Replaces the single constant-velocity model + hand-tuned regime heuristic with a
     * bank of motion models (constant velocity, constant acceleration,
     * coordinated turn, and near-constant-position / Brownian) combined every
     * step through an IMM Markov mixing cycle. The mode probabilities give a
     * continuous motion-regime estimate; maneuverProbability() exposes the mass
     * on the manoeuvring models (CA + CT) and replaces the prior hard
     * KinematicState categories for downstream policies.
     *
     * The filter is selected via kalman_type = 2 and conforms to IKalmanFilter
     * so the tracker, factory and gating paths treat it as any other predictor.
     * It exposes the standard 8-D state to the tracklet while carrying its
     * mode-specific (up to 10-D) sub-states internally; the per-track lifetime
     * of the factory-created predictor instance is what lets it persist that
     * internal mode bank across predict/update calls.
     *
     * Four-model bank — constant velocity, constant acceleration,
     * near-constant-position and coordinated turn — driving the full IMM Markov
     * cycle (mixing -> per-mode predict/update -> likelihood -> probability
     * update -> combination). The coordinated-turn mode is nonlinear and uses an
     * unscented (sigma-point) time update; the measurement model is linear in
     * all modes, so the correction step is a standard Kalman update throughout.
     * Gating runs on the combined estimate via an internal stateless
     * KalmanFilter helper. See docs/IMM_DESIGN.md.
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
             * Continuous [0, 1] regime signal that replaces the prior hard
             * Maneuvering category for association cost shaping, lost-buffer
             * scaling and OCM weighting. Zero until the mode bank is active.
             */
            float maneuverProbability() const override { return maneuver_probability_; }

            /** @brief Probability mass on the near-constant-position model. */
            float stationaryProbability() const override { return stationary_probability_; }

        private:
            /**
             * @brief Mode indices into the model bank.
             *
             * Center kinematics differ per mode; the size sub-state
             * (w, h, vw, vh) is a common constant-velocity block in all of them.
             * Each mode pins the extended slots it does not own (CA owns the
             * acceleration pair, CT owns the turn rate) to zero, so the shared
             * working space keeps a single consistent slot semantics and the
             * mixing / combination math stays model-agnostic.
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
