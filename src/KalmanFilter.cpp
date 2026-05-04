/**
 * @file      KalmanFilter.cpp
 * @brief     Implementation of the linear Kalman Filter for motion estimation.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#include "SushiTrack/KalmanFilter.h"
#include "SushiTrack/NSAKalmanFilter.h"
#include "SushiTrack/IKalmanFactory.h"
#include "SushiTrack/KalmanFilterRegistry.h"
#include "SushiTrack/Logger.h"

#include <cmath>
#include <cstddef>

namespace SushiTrack
{
    // Register default Kalman filters
    static KalmanFilterRegistrar<KalmanFilter> reg_kf(0);
    static KalmanFilterRegistrar<NSAKalmanFilter> reg_nsa(1);

    using StateMean = KalmanTypes::StateMean;
    using StateCov  = KalmanTypes::StateCov;
    using DetectBox = KalmanTypes::DetectBox;
    using StateHMean = Eigen::Matrix<float, 1, 4, Eigen::RowMajor>;
    using StateHCov  = Eigen::Matrix<float, 4, 4, Eigen::RowMajor>;

    /**
     * @brief Initializes the Kalman filter with the configured coordinate system.
     *
     * Noise constants are fixed to the reference ByteTrack tuning (see
     * KalmanFilter::kStdWeightPosition / kStdWeightVelocity / kInitPositionMult
     * / kInitVelocityMult).
     *
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
     * 
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
     *
     * Process-noise standard deviations scale linearly with dt, which produces a
     * variance contribution proportional to dt-squared. The earlier dt-squared
     * scaling on the standard deviation produced a dt-to-the-fourth growth in
     * variance, which is too aggressive once dt deviates from one and inflates
     * uncertainty enough to cause wrong re-associations on multi-frame recoveries.
     *
     * Process noise is held at the bytetrack reference scale: a single std_pos
     * baseline for the position/size components and a single std_vel baseline
     * for the velocity components, both proportional to the dominant box
     * dimension. The kf_pos_weight / kf_vel_weight multipliers are intentionally
     * NOT applied here. They expand the *initial* covariance in initiate() to
     * match the bytetrack reference (2x position, 10x velocity), but applying
     * them again per step inflates the steady-state process and measurement
     * noise relative to bytetrack and shifts the Kalman gain toward measurement-
     * only updates. That destroys the velocity estimate between frames and
     * shows up as elevated IDSW and reduced AssA on clean sequences.
     *
     * Per-state dynamic_*_mult multipliers (set transiently by the FSM) still
     * apply because they encode short-term motion regime changes.
     *
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

        dynamic_pos_mult_ = 1.0f; dynamic_size_mult_ = 1.0f; dynamic_vel_mult_ = 1.0f;

        mean        = (local_motion_mat * mean.transpose()).transpose();
        covariance.noalias() = (local_motion_mat * covariance * local_motion_mat.transpose()).eval();
        covariance += motion_cov;

        invalidateLLTCache();
    }

    /**
     * @brief Performs state correction based on an observation.
     * 
     * @param mean Reference to state mean for update.
     * @param covariance Reference to state covariance for update.
     * @param measurement Input observation vector.
     */
    void KalmanFilter::update(StateMean& mean, StateCov& covariance,
                                const DetectBox& measurement, float score)
    {
        StateHMean projected_mean;
        StateHCov  projected_cov;
        project(projected_mean, projected_cov, mean, covariance, score);

        Eigen::Matrix<float, 4, 8> B = (covariance * update_mat_.transpose()).transpose();

        auto llt = projected_cov.llt();
        if (llt.info() != Eigen::Success) {
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
     * 
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

        // Address-based validation is safer and faster than floating-point matrix comparison.
        // If the state is modified, llt_cache_valid_ is set to false via invalidateLLTCache().
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
     * 
     * @param mean Current state mean.
     * @param covariance Current state covariance.
     * @param measurement Input observation vector.
     * @return Computed squared Mahalanobis distance.
     */
    float KalmanFilter::mahalanobisDistance(const StateMean& mean,
                                            const StateCov& covariance,
                                            const DetectBox& measurement) const
    {
        StateHMean projected_mean;
        const auto& llt = getCachedLLT(mean, covariance, projected_mean);

        Eigen::Matrix<float, 1, 4> diff = measurement - projected_mean;
        Eigen::Matrix<float, 1, 4> z = llt.solve(diff.transpose()).transpose();
        return (diff * z.transpose())(0, 0);
    }

    /**
     * @brief Validates if a measurement lies within the statistical gate.
     * 
     * @param mean Current state mean.
     * @param covariance Current state covariance.
     * @param measurement Input observation vector.
     * @param chi2_thresh Chi-squared threshold for gating.
     * @return True if the measurement is within the gate.
     */
    bool KalmanFilter::isInGate(const StateMean& mean,
                                const StateCov& covariance,
                                const DetectBox& measurement,
                                float chi2_thresh) const
    {
        return mahalanobisDistance(mean, covariance, measurement) <= chi2_thresh;
    }

    /**
     * @brief Computes gating flags for a collection of measurements.
     * 
     * @param mean Current state mean.
     * @param covariance Current state covariance.
     * @param measurements Collection of input observation vectors.
     * @param chi2_thresh Chi-squared threshold for gating.
     * @return Collection of boolean flags indicating gate membership.
     */
    std::vector<bool> KalmanFilter::getGateFlags(const KalmanTypes::StateMean& mean,
                                                const KalmanTypes::StateCov& covariance,
                                                const KalmanTypes::DetectBoxVector& measurements,
                                                float chi2_thresh) const
    {
        StateHMean projected_mean;
        const auto& llt = getCachedLLT(mean, covariance, projected_mean);

        std::vector<bool> flags;
        flags.reserve(measurements.size());

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
     * 
     * @param mean Current state mean.
     * @param covariance Current state covariance.
     * @param measurements Collection of input observation vectors.
     * @return Collection of squared Mahalanobis distances.
     */
    std::vector<float> KalmanFilter::getGateSqDistances(const KalmanTypes::StateMean& mean,
                                                       const KalmanTypes::StateCov& covariance,
                                                       const KalmanTypes::DetectBoxVector& measurements) const
    {
        StateHMean projected_mean;
        const auto& llt = getCachedLLT(mean, covariance, projected_mean);

        std::vector<float> dists;
        dists.reserve(measurements.size());

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
     * 
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
     *
     * Measurement noise is held at the bytetrack reference scale: a single
     * std_pos baseline proportional to the dominant box dimension. The
     * kf_pos_weight / kf_size_weight multipliers are intentionally NOT
     * applied here for the same reason they are not applied in predict():
     * they only adjust the initial covariance in initiate(), and reusing
     * them at every project step changes the steady-state innovation gain
     * relative to the bytetrack reference.
     *
     * @param projected_mean Output projected mean vector.
     * @param projected_cov Output projected covariance matrix.
     * @param mean Current state mean vector.
     * @param covariance Current state covariance matrix.
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

    /**
     * @brief Initiates state mean and covariance with confidence-aware noise scaling.
     *
     * Scales the position and size components of the initial uncertainty by
     * (1 - score), giving low-confidence detections proportionally larger
     * initial covariance. The velocity components stay unscaled because the
     * detection score does not carry direct information about motion uncertainty.
     *
     * @param mean Output state mean vector.
     * @param covariance Output state covariance matrix.
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
     * @brief Projects the state distribution into the measurement space with NSA scaling.
     *
     * The measurement noise standard deviations are scaled by sqrt(1 - score)
     * so that low-confidence observations contribute less to the Kalman update.
     * The kf_pos_weight / kf_size_weight multipliers are intentionally NOT
     * applied here — see the base KalmanFilter::project rationale: those
     * multipliers only expand the initial covariance in initiate(), and reusing
     * them at every project step shifts the steady-state innovation gain.
     *
     * @param projected_mean Output projected mean vector.
     * @param projected_cov Output projected covariance matrix.
     * @param mean Input state mean vector.
     * @param covariance Input state covariance matrix.
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

    /**
     * @brief Initializes the generic Kalman factory with baseline noise parameters.
     *
     * @param kalman_type Identifier for the Kalman filter implementation.
     * @param std_weight_pos Baseline standard deviation weight for position.
     * @param std_weight_vel Baseline standard deviation weight for velocity.
     * @param kf_pos_weight Multiplier applied to position-related uncertainty.
     * @param kf_size_weight Multiplier applied to size-related uncertainty.
     * @param kf_vel_weight Multiplier applied to velocity-related uncertainty.
     * @param enable_xyah Flag selecting the xyah coordinate parameterization.
     */
    GenericKalmanFactory::GenericKalmanFactory(int kalman_type, bool enable_xyah)
        : kalman_type_(kalman_type), enable_xyah_(enable_xyah) {}

    /**
     * @brief Instantiates a state predictor through the registry.
     *
     * Delegates instantiation to the central registry to keep the factory open
     * for new Kalman implementations without requiring modifications here.
     *
     * @return Unique pointer to the constructed state predictor.
     */
    std::unique_ptr<IStatePredictor> GenericKalmanFactory::create() const
    {
        return KalmanFilterRegistry::getInstance().create(
            kalman_type_, enable_xyah_);
    }

} // namespace SushiTrack