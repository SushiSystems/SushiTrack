/**************************************************************************/
/* test_nsa_kalman_filter.cpp                                             */
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
 * @file test_nsa_kalman_filter.cpp
 * @brief Tests NSAKalmanFilter.
 * @author Mustafa Garip
 */

#include <cmath>
#include <string>

#include <Eigen/Dense>
#include <gtest/gtest.h>

#include <SushiTrack/filtering/kalman_types.hpp>
#include <SushiTrack/filtering/nsa_kalman_filter.hpp>

#include <measurement_builders.hpp>

using namespace SushiTrack;

using StateMean = KalmanTypes::StateMean;

using StateCov = KalmanTypes::StateCov;

using DetectBox = KalmanTypes::DetectBox;

// NSAKalmanFilter

class Small_NSAKalmanFilterTest : public ::testing::Test
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
TEST_F(Small_NSAKalmanFilterTest, InitiateProducesValidCovariance)
{
    kf_.initiate(mean_, cov_, default_box_, 0.8f);

    for (int i = 0; i < 8; ++i)
        EXPECT_GT(cov_(i, i), 0.f) << "NSA diagonal entry " << i << " not positive";
}

/**
 * @brief NSAKalmanFilter predict-update cycle does not diverge or produce NaN.
 */
TEST_F(Small_NSAKalmanFilterTest, PredictUpdateCycleIsNumericallyStable)
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
TEST_F(Small_NSAKalmanFilterTest, InitiateCovarianceScalesWithScore)
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
TEST_F(Small_NSAKalmanFilterTest, UpdateWeightsMeasurementByScore)
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
TEST_F(Small_NSAKalmanFilterTest, MahalanobisDistanceScalesWithScore)
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
TEST_F(Small_NSAKalmanFilterTest, BatchGatingWithScores)
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
TEST_F(Small_NSAKalmanFilterTest, NegativeScoreHandledGracefully)
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
TEST(Small_NSAKalmanFilterXYAHTest, XYAHModeSupport)
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
 * @brief Validates that NSAKalmanFilter does not produce negative covariance
 *        diagonal entries when updated with a detection score of zero.
 */
TEST_F(Small_NSAKalmanFilterTest, ScoreZeroDoesNotProduceNegativeCovariance)
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
