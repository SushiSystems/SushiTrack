/**************************************************************************/
/* imm_filter.cpp                                                         */
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
 * @file imm_filter.cpp
 * @brief Implements the Interacting Multiple Model (IMM) state estimator.
 * @author Mustafa Garip
 */

#include <SushiTrack/filtering/imm_filter.hpp>

#include <algorithm>
#include <array>
#include <cmath>

#include <SushiTrack/filtering/filtering_log_category.hpp>
#include <SushiTrack/filtering/kalman_filter_registry.hpp>
#include <SushiTrack/logging/log_throttle.hpp>
#include <SushiTrack/logging/logger.hpp>

#include <filtering/covariance_conditioning.hpp>

namespace SushiTrack
{
    static KalmanFilterRegistrar<IMMFilter> reg_imm(2);

    namespace
    {
        // Position and velocity std weights shared with KalmanFilter.
        constexpr float K_SWP = KalmanFilter::K_STD_WEIGHT_POSITION;
        constexpr float K_SWV = KalmanFilter::K_STD_WEIGHT_VELOCITY;
        constexpr float K_INIT_POS = KalmanFilter::K_INIT_POSITION_MULT;
        constexpr float K_INIT_VEL = KalmanFilter::K_INIT_VELOCITY_MULT;

        // Markov mode transition probability of staying in current mode.
        constexpr float K_P_STAY = 0.90f;

        // Coordinated-turn process noise on turn rate.
        constexpr float K_SIGMA_OMEGA = 0.30f;
        // Singularity threshold where turn rate degenerates to constant velocity.
        constexpr float K_OMEGA_EPS = 1e-4f;

        constexpr float K_EPS = 1e-9f;
        constexpr float K_TWO_PI = 6.283185307179586f;
    } // namespace

    IMMFilter::IMMFilter(bool enable_xyah)
        : enable_xyah_(enable_xyah), gate_kf_(std::make_unique<KalmanFilter>(enable_xyah))
    {
        // Transition matrix biased toward constant-velocity prior.
        const float base[K_NUM_MODES] = {0.70f, 0.10f, 0.10f, 0.10f};
        for (int i = 0; i < K_NUM_MODES; ++i)
        {
            float denom = 0.0f;
            for (int k = 0; k < K_NUM_MODES; ++k)
            {
                if (k != i)
                {
                    denom += base[k];
                }
            }
            for (int j = 0; j < K_NUM_MODES; ++j)
            {
                pi_(i, j) = (i == j) ? K_P_STAY : (1.0f - K_P_STAY) * base[j] / denom;
            }
        }

        mu_.setZero();
        cbar_.setZero();
        last_combined_.setZero();
    }

    IMMFilter::MatH IMMFilter::measurement_matrix() const
    {
        MatH measurement_map = MatH::Zero();
        measurement_map(0, 0) = measurement_map(1, 1) = measurement_map(2, 2) =
            measurement_map(3, 3) = 1.0f;
        return measurement_map;
    }

    IMMFilter::Mat4 IMMFilter::measurement_noise(float height, float width) const
    {
        Eigen::Vector4f s;
        if (enable_xyah_)
        {
            s << K_SWP * height, K_SWP * height, 1e-1f, K_SWP * height;
        }
        else
        {
            s << K_SWP * width, K_SWP * height, K_SWP * width, K_SWP * height;
        }
        return s.array().square().matrix().asDiagonal();
    }

    /**
     * @brief Builds the per-mode discrete transition matrix for linear modes.
     * @param m Motion mode identifier.
     * @param dt Time interval for state propagation.
     * @return Discrete transition matrix.
     */
    IMMFilter::Mat11 IMMFilter::transition_matrix(Mode m, float dt) const
    {
        Mat11 transition = Mat11::Identity();

        // Common size constant-velocity block: w += vw*dt, h += vh*dt.
        transition(2, 6) = dt;
        transition(3, 7) = dt;

        transition(10, 10) = 0.0f;

        switch (m)
        {
            case Mode::CV:
                transition(0, 4) = dt;
                transition(1, 5) = dt;
                transition(8, 8) = 0.0f;
                transition(9, 9) = 0.0f;
                break;

            case Mode::CA:
                transition(0, 4) = dt;
                transition(1, 5) = dt;
                transition(0, 8) = 0.5f * dt * dt;
                transition(1, 9) = 0.5f * dt * dt;
                transition(4, 8) = dt;
                transition(5, 9) = dt;
                break;

            case Mode::NCP:
                transition(0, 4) = 0.0f;
                transition(1, 5) = 0.0f;
                transition(4, 4) = 0.0f;
                transition(5, 5) = 0.0f;
                transition(8, 8) = 0.0f;
                transition(9, 9) = 0.0f;
                break;

            case Mode::CT: break;
        }
        return transition;
    }

    /**
     * @brief Returns per-mode process-noise standard deviations.
     * @param m Motion mode identifier.
     * @param dt Time interval for state propagation.
     * @param width Bounding box width.
     * @param height Bounding box height.
     * @return Process noise standard deviation vector.
     */
    IMMFilter::Vec11 IMMFilter::process_noise_diag(Mode m, float dt, float width,
                                                   float height) const
    {
        const float pos_w = K_SWP * width * dt;
        const float pos_h = K_SWP * height * dt;
        const float vel_w = K_SWV * width * dt;
        const float vel_h = K_SWV * height * dt;

        Vec11 s = Vec11::Zero();

        if (enable_xyah_)
        {
            s(2) = 1e-2f * dt;
            s(3) = pos_h;
            s(6) = 1e-5f * dt;
            s(7) = vel_h;
            s(0) = pos_h;
            s(1) = pos_h;
            s(4) = vel_h;
            s(5) = vel_h;
        }
        else
        {
            s(2) = pos_w;
            s(3) = pos_h;
            s(6) = vel_w;
            s(7) = vel_h;
            s(0) = pos_w;
            s(1) = pos_h;
            s(4) = vel_w;
            s(5) = vel_h;
        }

        s(8) = 1e-6f;
        s(9) = 1e-6f;
        s(10) = 1e-6f;

        switch (m)
        {
            case Mode::CV: break;

            case Mode::CA:
                s(4) *= 1.3f;
                s(5) *= 1.3f;
                s(8) = vel_w > 0.0f ? vel_w : vel_h;
                s(9) = vel_h;
                break;

            case Mode::NCP:
                s(0) *= 2.0f;
                s(1) *= 2.0f;
                s(4) *= 0.1f;
                s(5) *= 0.1f;
                break;

            case Mode::CT: s(10) = K_SIGMA_OMEGA * dt; break;
        }
        return s;
    }

    /**
     * @brief Nonlinear coordinated-turn time update.
     * @param x Current state vector.
     * @param dt Time interval for propagation.
     * @return Propagated state vector.
     */
    IMMFilter::Vec11 IMMFilter::ct_transition(const Vec11& x, float dt) const
    {
        Vec11 y = x;

        const float vx = x(4);
        const float vy = x(5);
        const float w = x(10);

        if (std::fabs(w) > K_OMEGA_EPS)
        {
            const float wdt = w * dt;
            const float s = std::sin(wdt);
            const float c = std::cos(wdt);
            y(0) = x(0) + (vx * s - vy * (1.0f - c)) / w;
            y(1) = x(1) + (vx * (1.0f - c) + vy * s) / w;
            y(4) = vx * c - vy * s;
            y(5) = vx * s + vy * c;
        }
        else
        {
            y(0) = x(0) + vx * dt;
            y(1) = x(1) + vy * dt;
        }

        y(2) = x(2) + x(6) * dt;
        y(3) = x(3) + x(7) * dt;

        y(8) = 0.0f;
        y(9) = 0.0f;
        y(10) = w;

        return y;
    }

    /**
     * @brief Unscented time update for the coordinated-turn mode.
     * @param x0 Prior state vector.
     * @param prior_cov Prior covariance matrix.
     * @param dt Time interval for propagation.
     * @param x_out Output predicted state vector.
     * @param out_cov Output predicted covariance matrix.
     */
    void IMMFilter::ukf_predict(const Vec11& x0, const Mat11& prior_cov, float dt, Vec11& x_out,
                                Mat11& out_cov) const
    {
        constexpr int n = K_DIM;
        constexpr float lambda = 0.0f;
        constexpr float wc0 = lambda / (n + lambda) + (1.0f - 1.0f + 2.0f);
        constexpr float wi = 1.0f / (2.0f * (n + lambda));
        const float spread = std::sqrt(static_cast<float>(n) + lambda);

        Mat11 mode_cov = prior_cov;
        Eigen::LLT<Mat11> llt(mode_cov);
        if (llt.info() != Eigen::Success)
        {
            Detail::report_regularized_covariance(Detail::ConditionedFilter::Imm, "prediction");
            mode_cov.diagonal().array() += 1e-6f;
            llt.compute(mode_cov);
        }
        Mat11 cholesky = llt.matrixL();

        std::array<Vec11, 2 * n + 1> Y;
        Y[0] = ct_transition(x0, dt);
        for (int k = 0; k < n; ++k)
        {
            Vec11 col = spread * cholesky.col(k);
            Y[1 + k] = ct_transition(x0 + col, dt);
            Y[1 + n + k] = ct_transition(x0 - col, dt);
        }

        // Mean: Wm0 = lambda/(n+lambda) = 0, so the centre point drops out.
        Vec11 xm = Vec11::Zero();
        for (int k = 1; k < 2 * n + 1; ++k)
        {
            xm += wi * Y[k];
        }

        Mat11 mixed_cov = Mat11::Zero();
        Vec11 d0 = Y[0] - xm;
        mixed_cov += wc0 * (d0 * d0.transpose());
        for (int k = 1; k < 2 * n + 1; ++k)
        {
            Vec11 d = Y[k] - xm;
            mixed_cov += wi * (d * d.transpose());
        }

        x_out = xm;
        out_cov = mixed_cov;
    }

    void IMMFilter::initiate(KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance,
                             const KalmanTypes::DetectBox& measurement, float /*score*/)
    {
        const float width = std::max(1e-2f, measurement(2));
        const float height = std::max(1e-2f, measurement(3));

        Vec11 init_std = Vec11::Zero();
        if (enable_xyah_)
        {
            init_std(0) = K_INIT_POS * K_SWP * height;
            init_std(1) = K_INIT_POS * K_SWP * height;
            init_std(2) = 1e-2f;
            init_std(3) = K_INIT_POS * K_SWP * height;
            init_std(4) = K_INIT_VEL * K_SWV * height;
            init_std(5) = K_INIT_VEL * K_SWV * height;
            init_std(6) = 1e-5f;
            init_std(7) = K_INIT_VEL * K_SWV * height;
        }
        else
        {
            init_std(0) = K_INIT_POS * K_SWP * width;
            init_std(1) = K_INIT_POS * K_SWP * height;
            init_std(2) = K_INIT_POS * K_SWP * width;
            init_std(3) = K_INIT_POS * K_SWP * height;
            init_std(4) = K_INIT_VEL * K_SWV * width;
            init_std(5) = K_INIT_VEL * K_SWV * height;
            init_std(6) = K_INIT_VEL * K_SWV * width;
            init_std(7) = K_INIT_VEL * K_SWV * height;
        }
        init_std(8) = K_INIT_VEL * K_SWV * height;
        init_std(9) = K_INIT_VEL * K_SWV * height;
        init_std(10) = K_SIGMA_OMEGA;

        Mat11 prior_cov = init_std.array().square().matrix().asDiagonal();

        Vec11 x0 = Vec11::Zero();
        x0.head<4>() = measurement.transpose();

        for (int j = 0; j < K_NUM_MODES; ++j)
        {
            x_[j] = x0;
            p_[j] = prior_cov;
        }

        // CV-weighted prior: most targets start in steady motion.
        mu_(index(Mode::CV)) = 0.70f;
        mu_(index(Mode::CA)) = 0.10f;
        mu_(index(Mode::NCP)) = 0.10f;
        mu_(index(Mode::CT)) = 0.10f;
        cbar_ = mu_;

        initialized_ = true;
        combine(mean, covariance);
        maneuver_probability_ = mu_(index(Mode::CA)) + mu_(index(Mode::CT));
        stationary_probability_ = mu_(index(Mode::NCP));
    }

    void IMMFilter::reanchor(const KalmanTypes::StateMean& external_mean)
    {
        // Delta between external and last combined mean is carried to all modes.
        Eigen::Matrix<float, 8, 1> delta = (external_mean - last_combined_).transpose().head<8>();

        if (delta.cwiseAbs().maxCoeff() <= K_EPS)
        {
            return;
        }

        for (int j = 0; j < K_NUM_MODES; ++j)
        {
            x_[j].head<8>() += delta;
        }
    }

    void IMMFilter::mix(std::array<Vec11, K_NUM_MODES>& x0,
                        std::array<Mat11, K_NUM_MODES>& prior_cov) const
    {
        for (int j = 0; j < K_NUM_MODES; ++j)
        {
            x0[j].setZero();
            const float norm = std::max(cbar_(j), K_EPS);
            for (int i = 0; i < K_NUM_MODES; ++i)
            {
                const float w_ij = pi_(i, j) * mu_(i) / norm;
                x0[j] += w_ij * x_[i];
            }

            prior_cov[j].setZero();
            for (int i = 0; i < K_NUM_MODES; ++i)
            {
                const float w_ij = pi_(i, j) * mu_(i) / norm;
                Vec11 d = x_[i] - x0[j];
                prior_cov[j] += w_ij * (p_[i] + d * d.transpose());
            }
        }
    }

    void IMMFilter::predict(KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance,
                            float dt)
    {
        if (!initialized_)
        {
            gate_kf_->predict(mean, covariance, dt);
            return;
        }

        reanchor(mean);

        // Predicted mode probabilities (mixing normalisers); carried to update().
        for (int j = 0; j < K_NUM_MODES; ++j)
        {
            float c = 0.0f;
            for (int i = 0; i < K_NUM_MODES; ++i)
            {
                c += pi_(i, j) * mu_(i);
            }
            cbar_(j) = c;
        }

        std::array<Vec11, K_NUM_MODES> x0;
        std::array<Mat11, K_NUM_MODES> prior_cov;
        mix(x0, prior_cov);

        for (int j = 0; j < K_NUM_MODES; ++j)
        {
            const Mode m = static_cast<Mode>(j);
            const float width = std::max(1e-2f, x0[j](2));
            const float height = std::max(1e-2f, x0[j](3));
            const Vec11 q = process_noise_diag(m, dt, width, height);

            if (m == Mode::CT)
            {
                ukf_predict(x0[j], prior_cov[j], dt, x_[j], p_[j]);
                p_[j] += q.array().square().matrix().asDiagonal();
            }
            else
            {
                const Mat11 transition = transition_matrix(m, dt);
                x_[j] = transition * x0[j];
                p_[j] = transition * prior_cov[j] * transition.transpose();
                p_[j] += q.array().square().matrix().asDiagonal();
            }
            symmetrize(p_[j]);
        }

        // Predicted mode probabilities become the prior before measurement arrives.
        mu_ = cbar_;

        combine(mean, covariance);
        maneuver_probability_ = mu_(index(Mode::CA)) + mu_(index(Mode::CT));
        stationary_probability_ = mu_(index(Mode::NCP));
    }

    void IMMFilter::update(KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance,
                           const KalmanTypes::DetectBox& measurement, float /*score*/)
    {
        if (!initialized_)
        {
            gate_kf_->update(mean, covariance, measurement, 0.0f);
            return;
        }

        const MatH measurement_map = measurement_matrix();
        const Eigen::Vector4f z = measurement.transpose();

        Eigen::Matrix<float, K_NUM_MODES, 1> likelihood;

        for (int j = 0; j < K_NUM_MODES; ++j)
        {
            const float width = std::max(1e-2f, x_[j](2));
            const float height = std::max(1e-2f, x_[j](3));
            const Mat4 noise = measurement_noise(height, width);

            Eigen::Vector4f nu = z - measurement_map * x_[j];
            Mat4 innovation_cov = measurement_map * p_[j] * measurement_map.transpose() + noise;

            Eigen::LLT<Mat4> llt(innovation_cov);
            if (llt.info() != Eigen::Success)
            {
                Detail::report_regularized_covariance(Detail::ConditionedFilter::Imm, "update");
                innovation_cov.diagonal().array() += 1e-6f;
                llt.compute(innovation_cov);
            }

            Eigen::Matrix<float, K_DIM, 4> gain =
                llt.solve((p_[j] * measurement_map.transpose()).transpose()).transpose();
            x_[j] += gain * nu;
            p_[j] -= gain * measurement_map * p_[j];
            symmetrize(p_[j]);

            // Gaussian likelihood of the innovation: Λ_j = N(nu; 0, innovation_cov).
            Eigen::Vector4f sinv_nu = llt.solve(nu);
            float maha = nu.dot(sinv_nu);
            Mat4 cholesky = llt.matrixL();
            float logdet = 2.0f * cholesky.diagonal().array().log().sum();
            float log_lik = -0.5f * (maha + logdet + 4.0f * std::log(K_TWO_PI));
            likelihood(j) = std::exp(log_lik);
        }

        // Mode probability update: μ_j ∝ c̄_j · Λ_j.
        float sum = 0.0f;
        for (int j = 0; j < K_NUM_MODES; ++j)
        {
            mu_(j) = cbar_(j) * likelihood(j);
            sum += mu_(j);
        }
        if (sum > K_EPS)
        {
            mu_ /= sum;
        }
        else
        {
            static LogThrottle collapsed_throttle;
            SUSHITRACK_LOG_WARN_LIMITED(K_FILTERING_LOG_CATEGORY, collapsed_throttle,
                                        "IMM filter: mode likelihood sum {} is below {}; mode "
                                        "probabilities reset to the prior.",
                                        sum, K_EPS);
            mu_ = cbar_;
        }

        combine(mean, covariance);
        maneuver_probability_ = mu_(index(Mode::CA)) + mu_(index(Mode::CT));
        stationary_probability_ = mu_(index(Mode::NCP));
    }

    void IMMFilter::combine(KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance)
    {
        Vec11 xc = Vec11::Zero();
        for (int j = 0; j < K_NUM_MODES; ++j)
        {
            xc += mu_(j) * x_[j];
        }

        Mat11 combined_cov = Mat11::Zero();
        for (int j = 0; j < K_NUM_MODES; ++j)
        {
            Vec11 d = x_[j] - xc;
            combined_cov += mu_(j) * (p_[j] + d * d.transpose());
        }

        mean = xc.head<8>().transpose();
        covariance = combined_cov.topLeftCorner<8, 8>();
        last_combined_ = mean;
    }

    void IMMFilter::save_checkpoint()
    {
        checkpoint_.x = x_;
        checkpoint_.cov = p_;
        checkpoint_.mu = mu_;
        checkpoint_.cbar = cbar_;
        checkpoint_.last_combined = last_combined_;
        checkpoint_.initialized = initialized_;
        checkpoint_.valid = true;
    }

    void IMMFilter::restore_checkpoint()
    {
        if (!checkpoint_.valid)
        {
            return;
        }
        x_ = checkpoint_.x;
        p_ = checkpoint_.cov;
        mu_ = checkpoint_.mu;
        cbar_ = checkpoint_.cbar;
        last_combined_ = checkpoint_.last_combined;
        initialized_ = checkpoint_.initialized;
        maneuver_probability_ = mu_(index(Mode::CA)) + mu_(index(Mode::CT));
        stationary_probability_ = mu_(index(Mode::NCP));
    }

    void IMMFilter::set_noise_multipliers(float pos_mult, float size_mult, float vel_mult)
    {
        // Intentional no-op: the IMM model bank carries per-mode process noise.
        (void)pos_mult;
        (void)size_mult;
        (void)vel_mult;
    }

    bool IMMFilter::is_in_gate(const KalmanTypes::StateMean& mean,
                               const KalmanTypes::StateCov& covariance,
                               const KalmanTypes::DetectBox& measurement, float chi2_thresh,
                               float score) const
    {
        return gate_kf_->is_in_gate(mean, covariance, measurement, chi2_thresh, score);
    }

    std::vector<bool> IMMFilter::get_gate_flags(const KalmanTypes::StateMean& mean,
                                                const KalmanTypes::StateCov& covariance,
                                                const KalmanTypes::DetectBoxVector& measurements,
                                                float chi2_thresh,
                                                const std::vector<float>& scores) const
    {
        return gate_kf_->get_gate_flags(mean, covariance, measurements, chi2_thresh, scores);
    }

    std::vector<float> IMMFilter::get_gate_sq_distances(
        const KalmanTypes::StateMean& mean, const KalmanTypes::StateCov& covariance,
        const KalmanTypes::DetectBoxVector& measurements, const std::vector<float>& scores) const
    {
        return gate_kf_->get_gate_sq_distances(mean, covariance, measurements, scores);
    }

} // namespace SushiTrack
