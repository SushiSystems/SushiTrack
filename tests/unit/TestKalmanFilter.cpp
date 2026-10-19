/**************************************************************************/
/* TestKalmanFilter.cpp                                                   */
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
 * @file TestKalmanFilter.cpp
 * @brief Unit tests for Kalman filter motion prediction.
 * @author Mustafa Garip
 */

#include <cmath>
#include <string>

#include <Eigen/Dense>
#include <gtest/gtest.h>

#include <SushiTrack/filtering/kalman_filter.hpp>
#include <SushiTrack/filtering/kalman_filter_registry.hpp>
#include <SushiTrack/filtering/kalman_types.hpp>
#include <SushiTrack/filtering/nsa_kalman_filter.hpp>
#include <SushiTrack/logging/logger.hpp>

#include "captured_log.hpp"

using namespace SushiTrack;
using StateMean = KalmanTypes::StateMean;
using StateCov = KalmanTypes::StateCov;
using DetectBox = KalmanTypes::DetectBox;

// Helpers

/** @brief Constructs a [x, y, w, h] detection box. */
static DetectBox make_box(float x, float y, float w, float h)
{
    DetectBox b;
    b << x, y, w, h;
    return b;
}

// initiate()

/**
 * @brief After initiation, the position/size block of the mean equals the
 *        measurement and the velocity block is zero.
 */
TEST(KalmanFilterTest, InitiateMeanMatchesMeasurement)
{
    KalmanFilter kf;
    StateMean mean;
    StateCov cov;
    DetectBox box = make_box(100.f, 200.f, 50.f, 80.f);

    kf.initiate(mean, cov, box);

    EXPECT_FLOAT_EQ(mean(0), 100.f);
    EXPECT_FLOAT_EQ(mean(1), 200.f);
    EXPECT_FLOAT_EQ(mean(2), 50.f);
    EXPECT_FLOAT_EQ(mean(3), 80.f);
    // Velocity block must be zero.
    for (int i = 4; i < 8; ++i)
        EXPECT_FLOAT_EQ(mean(i), 0.f) << "velocity component " << i << " is non-zero";
}

/**
 * @brief Covariance matrix produced by initiate() is diagonal and all
 *        diagonal entries are positive.
 */
TEST(KalmanFilterTest, InitiateCovarianceIsDiagonalAndPositive)
{
    KalmanFilter kf;
    StateMean mean;
    StateCov cov;
    kf.initiate(mean, cov, make_box(100.f, 200.f, 50.f, 80.f));

    for (int r = 0; r < 8; ++r)
    {
        EXPECT_GT(cov(r, r), 0.f) << "diagonal entry (" << r << "," << r << ") not positive";
        for (int c = 0; c < 8; ++c)
            if (r != c)
                EXPECT_FLOAT_EQ(cov(r, c), 0.f)
                    << "off-diagonal entry (" << r << "," << c << ") is non-zero";
    }
}

// predict()

/**
 * @brief After one predict step with constant velocity, the position components
 *        shift by (vx * dt, vy * dt, 0, 0) relative to the pre-predict mean.
 *
 * The Kalman motion model uses dt=1 when called with dt=1; the state stores
 * per-frame velocities so a dt=1 step advances position by exactly one frame's
 * worth of velocity.
 */
TEST(KalmanFilterTest, PredictPositionShiftsByVelocityTimesDt)
{
    KalmanFilter kf;
    StateMean mean;
    StateCov cov;
    kf.initiate(mean, cov, make_box(100.f, 200.f, 50.f, 80.f));

    // Manually inject a velocity into the state.
    mean(4) = 5.f; // vx
    mean(5) = 3.f; // vy

    const float dt = 1.0f;
    const float x0 = mean(0), y0 = mean(1);

    kf.predict(mean, cov, dt);

    EXPECT_NEAR(mean(0), x0 + 5.f * dt, 1e-4f);
    EXPECT_NEAR(mean(1), y0 + 3.f * dt, 1e-4f);
}

/**
 * @brief After one predict step, the covariance diagonal entries are strictly
 *        larger than before (uncertainty has grown).
 */
TEST(KalmanFilterTest, PredictIncreasesCovariance)
{
    KalmanFilter kf;
    StateMean mean;
    StateCov cov;
    kf.initiate(mean, cov, make_box(100.f, 200.f, 50.f, 80.f));

    StateCov cov_before = cov;
    kf.predict(mean, cov, 1.0f);

    for (int i = 0; i < 8; ++i)
        EXPECT_GE(cov(i, i), cov_before(i, i))
            << "diagonal entry " << i << " decreased after predict";
}

// update()

/**
 * @brief After a Kalman update with the exact current position as measurement,
 *        the mean remains close to that position.
 */
TEST(KalmanFilterTest, UpdateMeanMovesTowardMeasurement)
{
    KalmanFilter kf;
    StateMean mean;
    StateCov cov;
    DetectBox box = make_box(100.f, 200.f, 50.f, 80.f);
    kf.initiate(mean, cov, box);
    kf.predict(mean, cov, 1.0f);

    // Provide the original position as the measurement.
    kf.update(mean, cov, box, 0.9f);

    EXPECT_NEAR(mean(0), 100.f, 5.f);
    EXPECT_NEAR(mean(1), 200.f, 5.f);
    EXPECT_NEAR(mean(2), 50.f, 10.f);
    EXPECT_NEAR(mean(3), 80.f, 10.f);
}

/**
 * @brief After a Kalman update, the covariance diagonal entries are smaller
 *        than the post-predict values (uncertainty has decreased).
 */
TEST(KalmanFilterTest, UpdateDecreasesCovariance)
{
    KalmanFilter kf;
    StateMean mean;
    StateCov cov;
    DetectBox box = make_box(100.f, 200.f, 50.f, 80.f);
    kf.initiate(mean, cov, box);
    kf.predict(mean, cov, 1.0f);

    StateCov cov_after_predict = cov;
    kf.update(mean, cov, box, 0.9f);

    for (int i = 0; i < 8; ++i)
        EXPECT_LE(cov(i, i), cov_after_predict(i, i))
            << "diagonal entry " << i << " increased after update";
}

// Gating

/**
 * @brief An observation placed exactly at the predicted mean passes the
 *        standard chi-squared gate (chi2 threshold 9.4877 for 4 DOF, 95%).
 */
TEST(KalmanFilterTest, IsInGate_ObservationAtMeanPasses)
{
    KalmanFilter kf;
    StateMean mean;
    StateCov cov;
    DetectBox box = make_box(100.f, 200.f, 50.f, 80.f);
    kf.initiate(mean, cov, box);

    // Observation exactly at the current state mean (in measurement space).
    DetectBox obs;
    obs << mean(0) + mean(2) * 0.f, // x   (centre for XYWH = x + w/2, but state is XYWH directly)
        mean(1), mean(2), mean(3);
    // Actually for XYWH mode the measurement is [cx, cy, w, h] but the filter uses update_mat_ = identity (4x8 top rows), so measurement space = [x,y,w,h]. Use the box we initiated with for a zero-residual gate check.
    EXPECT_TRUE(kf.is_in_gate(mean, cov, box, 9.4877f));
}

/**
 * @brief An observation 1000 pixels away from the predicted mean fails the gate.
 */
TEST(KalmanFilterTest, IsInGate_FarObservationFails)
{
    KalmanFilter kf;
    StateMean mean;
    StateCov cov;
    kf.initiate(mean, cov, make_box(100.f, 200.f, 50.f, 80.f));

    DetectBox far_obs = make_box(5000.f, 5000.f, 50.f, 80.f);
    EXPECT_FALSE(kf.is_in_gate(mean, cov, far_obs, 9.4877f));
}

/**
 * @brief get_gate_flags() batch result matches individual is_in_gate() calls.
 */
TEST(KalmanFilterTest, GetGateFlagsMatchesScalarIsInGate)
{
    KalmanFilter kf;
    StateMean mean;
    StateCov cov;
    kf.initiate(mean, cov, make_box(100.f, 200.f, 50.f, 80.f));

    KalmanTypes::DetectBoxVector obs_vec;
    obs_vec.push_back(make_box(100.f, 200.f, 50.f, 80.f));   // near
    obs_vec.push_back(make_box(5000.f, 5000.f, 50.f, 80.f)); // far

    auto flags = kf.get_gate_flags(mean, cov, obs_vec, 9.4877f);
    ASSERT_EQ(flags.size(), obs_vec.size());

    for (size_t i = 0; i < obs_vec.size(); ++i)
        EXPECT_EQ(flags[i], kf.is_in_gate(mean, cov, obs_vec[i], 9.4877f))
            << "mismatch at index " << i;
}

// set_noise_multipliers

/**
 * @brief With noise multipliers > 1, predict produces larger covariance
 *        growth than with identity multipliers.
 */
TEST(KalmanFilterTest, ElevatedMultipliersIncreaseProcessNoise)
{
    KalmanFilter kf_base, kf_elevated;
    StateMean mean_base, mean_elev;
    StateCov cov_base, cov_elev;
    DetectBox box = make_box(100.f, 200.f, 50.f, 80.f);

    kf_base.initiate(mean_base, cov_base, box);
    kf_elevated.initiate(mean_elev, cov_elev, box);

    kf_elevated.set_noise_multipliers(2.0f, 2.0f, 2.0f);

    kf_base.predict(mean_base, cov_base, 1.0f);
    kf_elevated.predict(mean_elev, cov_elev, 1.0f);

    for (int i = 0; i < 8; ++i)
        EXPECT_GT(cov_elev(i, i), cov_base(i, i))
            << "elevated multipliers did not increase covariance at diagonal " << i;
}

// XYAH coordinate mode

/**
 * @brief In XYAH mode, initiate() stores [cx, cy, aspect, height] correctly.
 */
TEST(KalmanFilterTest, XYAHModeInitiateMeanMatchesMeasurement)
{
    KalmanFilter kf(/*enable_xyah=*/true);
    StateMean mean;
    StateCov cov;

    // [cx, cy, aspect, height]
    DetectBox box;
    box << 150.f, 300.f, 0.6f, 120.f;
    kf.initiate(mean, cov, box);

    EXPECT_FLOAT_EQ(mean(0), 150.f);
    EXPECT_FLOAT_EQ(mean(1), 300.f);
    EXPECT_NEAR(mean(2), 0.6f, 1e-3f);
    EXPECT_FLOAT_EQ(mean(3), 120.f);
    for (int i = 4; i < 8; ++i)
        EXPECT_FLOAT_EQ(mean(i), 0.f);
}

// NSAKalmanFilter

class NSAKalmanFilterTest : public ::testing::Test
{
    protected:
        NSAKalmanFilter kf_;
        StateMean mean_;
        StateCov cov_;
        DetectBox default_box_;

        void SetUp() override { default_box_ = make_box(100.f, 200.f, 50.f, 80.f); }
};

/**
 * @brief NSAKalmanFilter::initiate() produces a valid non-zero covariance.
 */
TEST_F(NSAKalmanFilterTest, InitiateProducesValidCovariance)
{
    kf_.initiate(mean_, cov_, default_box_, 0.8f);

    for (int i = 0; i < 8; ++i)
        EXPECT_GT(cov_(i, i), 0.f) << "NSA diagonal entry " << i << " not positive";
}

/**
 * @brief NSAKalmanFilter predict-update cycle does not diverge or produce NaN.
 */
TEST_F(NSAKalmanFilterTest, PredictUpdateCycleIsNumericallyStable)
{
    kf_.initiate(mean_, cov_, default_box_, 0.8f);

    for (int step = 0; step < 20; ++step)
    {
        kf_.predict(mean_, cov_, 1.0f);
        kf_.update(mean_, cov_, default_box_, 0.8f);
    }

    for (int i = 0; i < 8; ++i)
    {
        EXPECT_FALSE(std::isnan(mean_(i))) << "NaN in mean_ at " << i;
        EXPECT_FALSE(std::isinf(mean_(i))) << "Inf in mean_ at " << i;
        EXPECT_FALSE(std::isnan(cov_(i, i))) << "NaN in cov_ diagonal at " << i;
        EXPECT_GT(cov_(i, i), 0.f) << "non-positive covariance at " << i;
    }
}

/**
 * @brief NSAKalmanFilter initiate scales position/size covariance inversely with score.
 */
TEST_F(NSAKalmanFilterTest, InitiateCovarianceScalesWithScore)
{
    StateMean mean_high, mean_low;
    StateCov cov_high, cov_low;

    kf_.initiate(mean_high, cov_high, default_box_, 0.9f); // High confidence
    kf_.initiate(mean_low, cov_low, default_box_, 0.1f);   // Low confidence

    for (int i = 0; i < 4; ++i)
    {
        EXPECT_GT(cov_low(i, i), cov_high(i, i))
            << "Position/size covariance index " << i << " did not scale appropriately with score.";
    }

    for (int i = 4; i < 8; ++i)
    {
        EXPECT_FLOAT_EQ(cov_low(i, i), cov_high(i, i))
            << "Velocity covariance index " << i << " should not be scaled by score.";
    }
}

/**
 * @brief NSAKalmanFilter update weights measurements based on score.
 * Lower scores imply higher measurement uncertainty, so the state mean
 * should move less toward the new measurement.
 */
TEST_F(NSAKalmanFilterTest, UpdateWeightsMeasurementByScore)
{
    StateMean mean_high, mean_low;
    StateCov cov_high, cov_low;

    kf_.initiate(mean_high, cov_high, default_box_, 0.5f);
    kf_.initiate(mean_low, cov_low, default_box_, 0.5f);

    kf_.predict(mean_high, cov_high, 1.0f);
    kf_.predict(mean_low, cov_low, 1.0f);

    DetectBox new_meas = make_box(110.f, 210.f, 50.f, 80.f);

    kf_.update(mean_high, cov_high, new_meas, 0.9f); // High confidence
    kf_.update(mean_low, cov_low, new_meas, 0.1f);   // Low confidence

    float dist_high = (mean_high.block<1, 4>(0, 0) - new_meas).norm();
    float dist_low = (mean_low.block<1, 4>(0, 0) - new_meas).norm();

    EXPECT_LT(dist_high, dist_low)
        << "High score update should pull mean_ closer to measurement than low score update.";
}

/**
 * @brief Mahalanobis distance effectively shrinks for lower scores due to higher
 * expected measurement noise, making association more permissive.
 */
TEST_F(NSAKalmanFilterTest, MahalanobisDistanceScalesWithScore)
{
    kf_.initiate(mean_, cov_, default_box_, 0.5f);
    kf_.predict(mean_, cov_, 1.0f);

    DetectBox meas = make_box(150.f, 250.f, 50.f, 80.f);

    float dist_high = kf_.mahalanobis_distance(mean_, cov_, meas, 0.9f);
    float dist_low = kf_.mahalanobis_distance(mean_, cov_, meas, 0.1f);

    EXPECT_GT(dist_high, dist_low)
        << "Lower score should reduce Mahalanobis distance by inflating measurement noise.";
}

/**
 * @brief Batch gating methods correctly apply per-measurement scores.
 */
TEST_F(NSAKalmanFilterTest, BatchGatingWithScores)
{
    kf_.initiate(mean_, cov_, default_box_, 0.5f);
    kf_.predict(mean_, cov_, 1.0f);

    KalmanTypes::DetectBoxVector meas_vec = {make_box(120.f, 220.f, 50.f, 80.f),
                                             make_box(120.f, 220.f, 50.f, 80.f)};
    std::vector<float> scores = {0.9f, 0.1f};

    auto dists = kf_.get_gate_sq_distances(mean_, cov_, meas_vec, scores);
    EXPECT_GT(dists[0], dists[1]) << "Low score should result in lower squared distance.";

    float tight_thresh = (dists[0] + dists[1]) / 2.0f; // Threshold between the two distances
    auto flags = kf_.get_gate_flags(mean_, cov_, meas_vec, tight_thresh, scores);

    EXPECT_FALSE(flags[0]) << "High score measurement should fail tight gate.";
    EXPECT_TRUE(flags[1]) << "Low score measurement should pass tight gate due to inflated noise.";
}

/**
 * @brief Negative or zero scores gracefully default to base noise scaling.
 */
TEST_F(NSAKalmanFilterTest, NegativeScoreHandledGracefully)
{
    StateMean mean_neg, mean_zero;
    StateCov cov_neg, cov_zero;

    kf_.initiate(mean_neg, cov_neg, default_box_, -0.5f);
    kf_.initiate(mean_zero, cov_zero, default_box_, 0.0f);

    for (int i = 0; i < 8; ++i)
    {
        EXPECT_FLOAT_EQ(cov_neg(i, i), cov_zero(i, i))
            << "Negative score should be handled identically to zero/missing score.";
    }
}

/**
 * @brief XYAH coordinate mode functions correctly with NSA scaling.
 */
TEST(NSAKalmanFilterXYAHTest, XYAHModeSupport)
{
    NSAKalmanFilter kf_xyah(true);
    StateMean mean;
    StateCov cov;

    DetectBox box;
    box << 150.f, 300.f, 0.6f, 120.f; // cx, cy, aspect, height

    kf_xyah.initiate(mean, cov, box, 0.8f);

    EXPECT_FLOAT_EQ(mean(0), 150.f);
    EXPECT_FLOAT_EQ(mean(1), 300.f);
    EXPECT_NEAR(mean(2), 0.6f, 1e-3f);
    EXPECT_FLOAT_EQ(mean(3), 120.f);

    for (int i = 0; i < 8; ++i)
    {
        EXPECT_GT(cov(i, i), 0.f);
    }
}

/**
 * @brief Verifies multi-step prediction logic when the time step (dt) is greater than one.
 *        The position components are expected to shift linearly based on constant velocity.
 */
TEST(KalmanFilterTest, PredictWithDtGreaterThanOne)
{
    KalmanFilter kf(false); // standard xywh
    KalmanTypes::StateMean mean;
    KalmanTypes::StateCov cov;
    KalmanTypes::DetectBox initial_box;
    initial_box << 100.0f, 100.0f, 50.0f, 200.0f;

    kf.initiate(mean, cov, initial_box, 1.0f);
    mean(4) = 10.0f; // vx
    mean(5) = 5.0f;  // vy

    float dt = 2.5f; // Multi-step
    kf.predict(mean, cov, dt);

    EXPECT_FLOAT_EQ(mean(0), 100.0f + 10.0f * dt); // x = x + vx * dt
    EXPECT_FLOAT_EQ(mean(1), 100.0f + 5.0f * dt);  // y = y + vy * dt
}

/**
 * @brief Evaluates numerical stability in XYAH coordinate format during a
 *        predict-update cycle with constant measurements.
 */
TEST(KalmanFilterTest, XyahPredictUpdateStability)
{
    KalmanFilter kf(true); // enable_xyah = true
    KalmanTypes::StateMean mean;
    KalmanTypes::StateCov cov;
    KalmanTypes::DetectBox box;
    box << 50.0f, 50.0f, 0.5f, 100.0f; // x, y, a, h

    kf.initiate(mean, cov, box, 1.0f);
    kf.predict(mean, cov, 1.0f);
    kf.update(mean, cov, box, 1.0f); // Update with same box

    EXPECT_NEAR(mean(2), 0.5f, 1e-4); // Check aspect ratio remains stable
}

/**
 * @brief Validates that NSAKalmanFilter does not produce negative covariance
 *        diagonal entries when updated with a detection score of zero.
 */
TEST_F(NSAKalmanFilterTest, ScoreZeroDoesNotProduceNegativeCovariance)
{
    NSAKalmanFilter nsa_kf(false);
    KalmanTypes::StateMean mean_;
    KalmanTypes::StateCov cov_;
    KalmanTypes::DetectBox box;
    box << 10.0f, 10.0f, 20.0f, 20.0f;

    nsa_kf.initiate(mean_, cov_, box, 1.0f);
    nsa_kf.predict(mean_, cov_, 1.0f);

    nsa_kf.update(mean_, cov_, box, 0.0f);

    for (int i = 0; i < 8; ++i)
        EXPECT_GE(cov_(i, i), 0.0f);
}

/**
 * @brief Ensures the covariance matrix remains symmetric after a predict-update cycle.
 */
TEST(KalmanFilterTest, CovarianceRemainsSymmetricAfterUpdate)
{
    KalmanFilter kf(false);
    KalmanTypes::StateMean mean;
    KalmanTypes::StateCov cov;
    KalmanTypes::DetectBox box;
    box << 30.0f, 30.0f, 15.0f, 40.0f;

    kf.initiate(mean, cov, box, 1.0f);
    kf.predict(mean, cov, 1.0f);
    kf.update(mean, cov, box, 0.9f);

    // Check symmetry: A == A^T
    EXPECT_TRUE(cov.isApprox(cov.transpose(), 1e-5f));
}

// KalmanFilterRegistry

/**
 * @brief An unregistered type yields no filter and one ERROR line naming the type.
 */
TEST(KalmanFilterRegistryTest, UnregisteredTypeLogsTheTypeAndReturnsNull)
{
    std::unique_ptr<GatedStatePredictor> filter;
    std::string logged;
    {
        CapturedLog log(SushiLog::LogLevel::Error);
        filter = KalmanFilterRegistry::get_instance().create(987654, false);
        logged = log.last("Unregistered Kalman type requested");
    }

    EXPECT_EQ(filter, nullptr);
    EXPECT_NE(logged.find("Unregistered Kalman type requested: 987654"), std::string::npos)
        << "logged: " << logged;
}
