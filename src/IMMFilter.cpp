/**
 * @file      IMMFilter.cpp
 * @brief     Implementation of the IMM state estimator (kalman_type = 2).
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      29.06.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#include "SushiTrack/IMMFilter.h"
#include "SushiTrack/KalmanFilterRegistry.h"

#include <cmath>
#include <algorithm>

namespace SushiTrack
{
    // Register the IMM estimator as kalman_type = 2 (0 = Linear, 1 = NSA).
    static KalmanFilterRegistrar<IMMFilter> reg_imm(2);

    namespace
    {
        // Reuse the bytetrack-calibrated noise scale shared with KalmanFilter so
        // the CV mode tracks the legacy filter closely on clean sequences.
        constexpr float kSWP = KalmanFilter::kStdWeightPosition;  // position std weight
        constexpr float kSWV = KalmanFilter::kStdWeightVelocity;  // velocity std weight
        constexpr float kInitPos = KalmanFilter::kInitPositionMult;
        constexpr float kInitVel = KalmanFilter::kInitVelocityMult;

        // Markov mode model: probability of staying in the current mode; the
        // remainder is split evenly across the other modes.
        constexpr float kPStay = 0.90f;

        // Coordinated-turn process noise on the turn rate (rad per time unit).
        constexpr float kSigmaOmega = 0.30f;
        // Turn rate below which the coordinated turn degenerates to constant
        // velocity; avoids the 1/omega singularity.
        constexpr float kOmegaEps = 1e-4f;

        constexpr float kEps = 1e-9f;
        constexpr float kTwoPi = 6.283185307179586f;
    }

    IMMFilter::IMMFilter(bool enable_xyah)
        : enable_xyah_(enable_xyah),
          gate_kf_(std::make_unique<KalmanFilter>(enable_xyah))
    {
        // CV-favoured Markov model. A uniform off-diagonal makes Pi's stationary
        // distribution uniform, so when no mode's likelihood dominates mu relaxes
        // to 0.25 each and the combined estimate is 75% polluted by the exotic
        // modes (NCP drags velocity to zero, CT curves, and the mode-spread term
        // inflates the covariance / gate). Weighting the off-diagonal mass toward
        // CV pulls the stationary distribution back to a CV-heavy prior, so the
        // bank behaves like the CV baseline unless a maneuver likelihood spikes.
        const float base[kNumModes] = {0.70f, 0.10f, 0.10f, 0.10f};
        for (int i = 0; i < kNumModes; ++i)
        {
            float denom = 0.0f;
            for (int k = 0; k < kNumModes; ++k) if (k != i) denom += base[k];
            for (int j = 0; j < kNumModes; ++j)
                Pi_(i, j) = (i == j) ? kPStay
                                     : (1.0f - kPStay) * base[j] / denom;
        }

        mu_.setZero();
        cbar_.setZero();
        last_combined_.setZero();
    }

    IMMFilter::MatH IMMFilter::measurementMatrix() const
    {
        MatH H = MatH::Zero();
        H(0, 0) = H(1, 1) = H(2, 2) = H(3, 3) = 1.0f;
        return H;
    }

    IMMFilter::Mat4 IMMFilter::measurementNoise(float height, float width) const
    {
        Eigen::Vector4f s;
        if (enable_xyah_)
            s << kSWP * height, kSWP * height, 1e-1f, kSWP * height;
        else
            s << kSWP * width, kSWP * height, kSWP * width, kSWP * height;
        return s.array().square().matrix().asDiagonal();
    }

    /**
     * @brief Builds the per-mode discrete transition matrix (linear modes only).
     *
     * The size sub-state (w, h, vw, vh) is a constant-velocity block in every
     * mode. Only the centre kinematics differ: CV integrates velocity, CA adds
     * the acceleration coupling, and NCP (near-constant-position) freezes both
     * the centre position integration and the centre velocity. Slots that a mode
     * does not own (accel for CV/NCP, turn rate for all linear modes) are pinned
     * to zero. The coordinated-turn mode is nonlinear and does not use this
     * matrix; see ctTransition / ukfPredict.
     */
    IMMFilter::Mat11 IMMFilter::transitionMatrix(Mode m, float dt) const
    {
        Mat11 F = Mat11::Identity();

        // Common size constant-velocity block: w += vw*dt, h += vh*dt.
        F(2, 6) = dt;
        F(3, 7) = dt;

        F(10, 10) = 0.0f;   // linear modes do not own the turn rate

        switch (m)
        {
            case CV:
                F(0, 4) = dt; F(1, 5) = dt;
                F(8, 8) = 0.0f; F(9, 9) = 0.0f;   // no acceleration state
                break;

            case CA:
                F(0, 4) = dt; F(1, 5) = dt;
                F(0, 8) = 0.5f * dt * dt; F(1, 9) = 0.5f * dt * dt;
                F(4, 8) = dt;             F(5, 9) = dt;
                // accel held constant (identity diagonal already 1)
                break;

            case NCP:
                F(0, 4) = 0.0f; F(1, 5) = 0.0f;   // freeze centre position integration
                F(4, 4) = 0.0f; F(5, 5) = 0.0f;   // damp centre velocity to zero
                F(8, 8) = 0.0f; F(9, 9) = 0.0f;
                break;

            case CT:
                break;   // handled by ctTransition / ukfPredict
        }
        return F;
    }

    /**
     * @brief Per-mode process-noise standard deviations (pre-square).
     *
     * Each mode carries a distinct "personality": CV is the bytetrack baseline,
     * CA injects acceleration (jerk) noise, NCP widens position (random walk)
     * and suppresses velocity, CT injects turn-rate noise. Unowned extended
     * slots are pinned tiny to stay non-singular. This is the principled,
     * Bayes-mixed replacement for hand-switched process-noise multipliers.
     */
    IMMFilter::Vec11 IMMFilter::processNoiseDiag(Mode m, float dt, float width, float height) const
    {
        const float pos_w = kSWP * width  * dt;
        const float pos_h = kSWP * height * dt;
        const float vel_w = kSWV * width  * dt;
        const float vel_h = kSWV * height * dt;

        Vec11 s = Vec11::Zero();

        // Size block (common constant-velocity): slots 2,3 (w,h) and 6,7 (vw,vh).
        if (enable_xyah_)
        {
            s(2) = 1e-2f * dt;  s(3) = pos_h;
            s(6) = 1e-5f * dt;  s(7) = vel_h;
            s(0) = pos_h; s(1) = pos_h;          // xyah centre scales with height
            s(4) = vel_h; s(5) = vel_h;
        }
        else
        {
            s(2) = pos_w; s(3) = pos_h;
            s(6) = vel_w; s(7) = vel_h;
            s(0) = pos_w; s(1) = pos_h;
            s(4) = vel_w; s(5) = vel_h;
        }

        s(8) = 1e-6f; s(9) = 1e-6f;   // accel: live only for CA (overwritten below)
        s(10) = 1e-6f;                // turn rate: live only for CT (overwritten below)

        switch (m)
        {
            case CV:
                break;

            case CA:
                s(4) *= 1.3f; s(5) *= 1.3f;
                s(8) = vel_w > 0.0f ? vel_w : vel_h;
                s(9) = vel_h;
                break;

            case NCP:
                s(0) *= 2.0f; s(1) *= 2.0f;       // position random walk
                s(4) *= 0.1f; s(5) *= 0.1f;       // suppressed velocity
                break;

            case CT:
                s(10) = kSigmaOmega * dt;
                break;
        }
        return s;
    }

    /**
     * @brief Nonlinear coordinated-turn time update.
     *
     * Rotates the centre velocity by omega*dt and integrates the curved path;
     * the size block stays constant-velocity and the acceleration pair is pinned
     * to zero (CT does not own it). Degenerates to constant velocity as the turn
     * rate approaches zero to avoid the 1/omega singularity.
     */
    IMMFilter::Vec11 IMMFilter::ctTransition(const Vec11& x, float dt) const
    {
        Vec11 y = x;

        const float vx = x(4), vy = x(5), w = x(10);

        if (std::fabs(w) > kOmegaEps)
        {
            const float wdt = w * dt;
            const float s = std::sin(wdt), c = std::cos(wdt);
            y(0) = x(0) + (vx * s - vy * (1.0f - c)) / w;
            y(1) = x(1) + (vx * (1.0f - c) + vy * s) / w;
            y(4) = vx * c - vy * s;
            y(5) = vx * s + vy * c;
        }
        else
        {
            y(0) = x(0) + vx * dt;
            y(1) = x(1) + vy * dt;
            // velocity unchanged in the straight-line limit
        }

        // Common size constant-velocity block.
        y(2) = x(2) + x(6) * dt;
        y(3) = x(3) + x(7) * dt;

        y(8) = 0.0f; y(9) = 0.0f;     // CT does not own acceleration
        y(10) = w;                    // constant turn rate

        return y;
    }

    /**
     * @brief Unscented (sigma-point) time update for the coordinated-turn mode.
     *
     * Uses the symmetric set with lambda = 0 (alpha = 1, kappa = 0, beta = 2),
     * which keeps the sigma spread local and the recombined covariance
     * well-conditioned for the mild CT nonlinearity. The measurement model is
     * linear, so only the time update needs the unscented treatment.
     */
    void IMMFilter::ukfPredict(const Vec11& x0, const Mat11& P0, float dt,
                               Vec11& x_out, Mat11& P_out) const
    {
        constexpr int n = kDim;
        constexpr float lambda = 0.0f;          // alpha=1, kappa=0
        constexpr float wc0 = lambda / (n + lambda) + (1.0f - 1.0f + 2.0f); // beta=2
        constexpr float wi  = 1.0f / (2.0f * (n + lambda));
        const float spread = std::sqrt(static_cast<float>(n) + lambda);

        Mat11 Pj = P0;
        Eigen::LLT<Mat11> llt(Pj);
        if (llt.info() != Eigen::Success)
        {
            Pj.diagonal().array() += 1e-6f;
            llt.compute(Pj);
        }
        Mat11 L = llt.matrixL();

        std::array<Vec11, 2 * n + 1> Y;
        Y[0] = ctTransition(x0, dt);
        for (int k = 0; k < n; ++k)
        {
            Vec11 col = spread * L.col(k);
            Y[1 + k]     = ctTransition(x0 + col, dt);
            Y[1 + n + k] = ctTransition(x0 - col, dt);
        }

        // Mean: Wm0 = lambda/(n+lambda) = 0, so the centre point drops out.
        Vec11 xm = Vec11::Zero();
        for (int k = 1; k < 2 * n + 1; ++k) xm += wi * Y[k];

        Mat11 Pm = Mat11::Zero();
        Vec11 d0 = Y[0] - xm;
        Pm += wc0 * (d0 * d0.transpose());
        for (int k = 1; k < 2 * n + 1; ++k)
        {
            Vec11 d = Y[k] - xm;
            Pm += wi * (d * d.transpose());
        }

        x_out = xm;
        P_out = Pm;
    }

    void IMMFilter::initiate(KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance,
                             const KalmanTypes::DetectBox& measurement, float /*score*/)
    {
        const float width  = std::max(1e-2f, measurement(2));
        const float height = std::max(1e-2f, measurement(3));

        Vec11 init_std = Vec11::Zero();
        if (enable_xyah_)
        {
            init_std(0) = kInitPos * kSWP * height;
            init_std(1) = kInitPos * kSWP * height;
            init_std(2) = 1e-2f;
            init_std(3) = kInitPos * kSWP * height;
            init_std(4) = kInitVel * kSWV * height;
            init_std(5) = kInitVel * kSWV * height;
            init_std(6) = 1e-5f;
            init_std(7) = kInitVel * kSWV * height;
        }
        else
        {
            init_std(0) = kInitPos * kSWP * width;
            init_std(1) = kInitPos * kSWP * height;
            init_std(2) = kInitPos * kSWP * width;
            init_std(3) = kInitPos * kSWP * height;
            init_std(4) = kInitVel * kSWV * width;
            init_std(5) = kInitVel * kSWV * height;
            init_std(6) = kInitVel * kSWV * width;
            init_std(7) = kInitVel * kSWV * height;
        }
        init_std(8)  = kInitVel * kSWV * height;   // acceleration prior (loose)
        init_std(9)  = kInitVel * kSWV * height;
        init_std(10) = kSigmaOmega;                // turn-rate prior

        Mat11 P0 = init_std.array().square().matrix().asDiagonal();

        Vec11 x0 = Vec11::Zero();
        x0.head<4>() = measurement.transpose();

        for (int j = 0; j < kNumModes; ++j)
        {
            x_[j] = x0;
            P_[j] = P0;
        }

        // CV-weighted prior: most targets start in steady motion.
        mu_(CV) = 0.70f; mu_(CA) = 0.10f; mu_(NCP) = 0.10f; mu_(CT) = 0.10f;
        cbar_ = mu_;

        initialized_ = true;
        combine(mean, covariance);
        maneuver_probability_ = mu_(CA) + mu_(CT);
        stationary_probability_ = mu_(NCP);
    }

    void IMMFilter::reanchor(const KalmanTypes::StateMean& external_mean)
    {
        // The tracklet may mutate the shared 8-D mean between calls (ByteTrack
        // size-velocity freeze on unobserved tracks). Carry that delta onto
        // every mode so the bank stays consistent with the tracklet's view.
        Eigen::Matrix<float, 8, 1> delta =
            (external_mean - last_combined_).transpose().head<8>();

        if (delta.cwiseAbs().maxCoeff() <= kEps) return;

        for (int j = 0; j < kNumModes; ++j)
            x_[j].head<8>() += delta;
    }

    void IMMFilter::mix(std::array<Vec11, kNumModes>& x0,
                        std::array<Mat11, kNumModes>& P0) const
    {
        for (int j = 0; j < kNumModes; ++j)
        {
            x0[j].setZero();
            const float norm = std::max(cbar_(j), kEps);
            for (int i = 0; i < kNumModes; ++i)
            {
                const float w_ij = Pi_(i, j) * mu_(i) / norm;
                x0[j] += w_ij * x_[i];
            }

            P0[j].setZero();
            for (int i = 0; i < kNumModes; ++i)
            {
                const float w_ij = Pi_(i, j) * mu_(i) / norm;
                Vec11 d = x_[i] - x0[j];
                P0[j] += w_ij * (P_[i] + d * d.transpose());
            }
        }
    }

    void IMMFilter::predict(KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance, float dt)
    {
        if (!initialized_) { gate_kf_->predict(mean, covariance, dt); return; }

        reanchor(mean);

        // Predicted mode probabilities (mixing normalisers); carried to update().
        for (int j = 0; j < kNumModes; ++j)
        {
            float c = 0.0f;
            for (int i = 0; i < kNumModes; ++i) c += Pi_(i, j) * mu_(i);
            cbar_(j) = c;
        }

        std::array<Vec11, kNumModes> x0;
        std::array<Mat11, kNumModes> P0;
        mix(x0, P0);

        for (int j = 0; j < kNumModes; ++j)
        {
            const Mode m = static_cast<Mode>(j);
            const float width  = std::max(1e-2f, x0[j](2));
            const float height = std::max(1e-2f, x0[j](3));
            const Vec11 q = processNoiseDiag(m, dt, width, height);

            if (m == CT)
            {
                ukfPredict(x0[j], P0[j], dt, x_[j], P_[j]);
                P_[j] += q.array().square().matrix().asDiagonal();
            }
            else
            {
                const Mat11 F = transitionMatrix(m, dt);
                x_[j] = F * x0[j];
                P_[j] = F * P0[j] * F.transpose();
                P_[j] += q.array().square().matrix().asDiagonal();
            }
            symmetrize(P_[j]);
        }

        // No measurement yet: the predicted mode probabilities are the best
        // estimate. update() overwrites these when an observation arrives.
        mu_ = cbar_;

        combine(mean, covariance);
        maneuver_probability_ = mu_(CA) + mu_(CT);
        stationary_probability_ = mu_(NCP);
    }

    void IMMFilter::update(KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance,
                           const KalmanTypes::DetectBox& measurement, float /*score*/)
    {
        if (!initialized_) { gate_kf_->update(mean, covariance, measurement, 0.0f); return; }

        const MatH H = measurementMatrix();
        const Eigen::Vector4f z = measurement.transpose();

        Eigen::Matrix<float, kNumModes, 1> likelihood;

        for (int j = 0; j < kNumModes; ++j)
        {
            const float width  = std::max(1e-2f, x_[j](2));
            const float height = std::max(1e-2f, x_[j](3));
            const Mat4 R = measurementNoise(height, width);

            Eigen::Vector4f nu = z - H * x_[j];
            Mat4 S = H * P_[j] * H.transpose() + R;

            Eigen::LLT<Mat4> llt(S);
            if (llt.info() != Eigen::Success)
            {
                S.diagonal().array() += 1e-6f;
                llt.compute(S);
            }

            Eigen::Matrix<float, kDim, 4> K = llt.solve((P_[j] * H.transpose()).transpose()).transpose();
            x_[j] += K * nu;
            P_[j] -= K * H * P_[j];
            symmetrize(P_[j]);

            // Gaussian likelihood of the innovation: Λ_j = N(nu; 0, S).
            Eigen::Vector4f sinv_nu = llt.solve(nu);
            float maha = nu.dot(sinv_nu);
            Mat4 L = llt.matrixL();
            float logdet = 2.0f * L.diagonal().array().log().sum();
            float log_lik = -0.5f * (maha + logdet + 4.0f * std::log(kTwoPi));
            likelihood(j) = std::exp(log_lik);
        }

        // Mode probability update: μ_j ∝ c̄_j · Λ_j.
        float sum = 0.0f;
        for (int j = 0; j < kNumModes; ++j)
        {
            mu_(j) = cbar_(j) * likelihood(j);
            sum += mu_(j);
        }
        if (sum > kEps) mu_ /= sum;
        else            mu_ = cbar_;     // degenerate likelihoods: fall back to prior

        combine(mean, covariance);
        maneuver_probability_ = mu_(CA) + mu_(CT);
        stationary_probability_ = mu_(NCP);
    }

    void IMMFilter::combine(KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance)
    {
        Vec11 xc = Vec11::Zero();
        for (int j = 0; j < kNumModes; ++j) xc += mu_(j) * x_[j];

        Mat11 Pc = Mat11::Zero();
        for (int j = 0; j < kNumModes; ++j)
        {
            Vec11 d = x_[j] - xc;
            Pc += mu_(j) * (P_[j] + d * d.transpose());
        }

        mean = xc.head<8>().transpose();
        covariance = Pc.topLeftCorner<8, 8>();
        last_combined_ = mean;
    }

    void IMMFilter::saveCheckpoint()
    {
        checkpoint_.x             = x_;
        checkpoint_.P             = P_;
        checkpoint_.mu            = mu_;
        checkpoint_.cbar          = cbar_;
        checkpoint_.last_combined = last_combined_;
        checkpoint_.initialized   = initialized_;
        checkpoint_.valid         = true;
    }

    void IMMFilter::restoreCheckpoint()
    {
        if (!checkpoint_.valid) return;
        x_              = checkpoint_.x;
        P_              = checkpoint_.P;
        mu_             = checkpoint_.mu;
        cbar_           = checkpoint_.cbar;
        last_combined_  = checkpoint_.last_combined;
        initialized_    = checkpoint_.initialized;
        maneuver_probability_   = mu_(CA) + mu_(CT);
        stationary_probability_ = mu_(NCP);
    }

    void IMMFilter::setNoiseMultipliers(float pos_mult, float size_mult, float vel_mult)
    {
        // Intentional no-op: the IMM model bank carries per-mode process noise.
        (void)pos_mult; (void)size_mult; (void)vel_mult;
    }

    bool IMMFilter::isInGate(const KalmanTypes::StateMean& mean, const KalmanTypes::StateCov& covariance,
                             const KalmanTypes::DetectBox& measurement, float chi2_thresh,
                             float score) const
    {
        return gate_kf_->isInGate(mean, covariance, measurement, chi2_thresh, score);
    }

    std::vector<bool> IMMFilter::getGateFlags(const KalmanTypes::StateMean& mean,
                                              const KalmanTypes::StateCov& covariance,
                                              const KalmanTypes::DetectBoxVector& measurements,
                                              float chi2_thresh,
                                              const std::vector<float>& scores) const
    {
        return gate_kf_->getGateFlags(mean, covariance, measurements, chi2_thresh, scores);
    }

    std::vector<float> IMMFilter::getGateSqDistances(const KalmanTypes::StateMean& mean,
                                                     const KalmanTypes::StateCov& covariance,
                                                     const KalmanTypes::DetectBoxVector& measurements,
                                                     const std::vector<float>& scores) const
    {
        return gate_kf_->getGateSqDistances(mean, covariance, measurements, scores);
    }

} // namespace SushiTrack
