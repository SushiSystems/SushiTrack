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
/* Licensed under the Apache License, Version 2.0 (the "License"); you    */
/* may not use this file except in compliance with the License. You may   */
/* obtain a copy of the License at                                        */
/*                                                                        */
/*     https://www.apache.org/licenses/LICENSE-2.0                        */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/**************************************************************************/

/**
 * @file test_nsa_kalman_filter.cpp
 * @brief Tests NSAKalmanFilter.
 * @author Mustafa Garip
 */

#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/filtering/kalman_types.hpp>
#include <SushiTrack/filtering/nsa_kalman_filter.hpp>

#include <measurement_builders.hpp>
#include <support/tolerance.hpp>

using namespace SushiTrack;

using StateMean = KalmanTypes::StateMean;

using StateCov = KalmanTypes::StateCov;

using DetectBox = KalmanTypes::DetectBox;

namespace
{
    /** @brief Returns the state [x, y, w, h, vx, vy, vw, vh] with the eight values given. */
    StateMean make_state(float x, float y, float w, float h, float vx, float vy, float vw, float vh)
    {
        StateMean state;
        state << x, y, w, h, vx, vy, vw, vh;
        return state;
    }

    /** @brief Runs @p cycles predict-update cycles of one second each on the same box. */
    void run_cycles(NSAKalmanFilter& filter, StateMean& mean, StateCov& cov, const DetectBox& box,
                    float score, int cycles)
    {
        for (int cycle = 0; cycle < cycles; ++cycle)
        {
            filter.predict(mean, cov, 1.0f);
            filter.update(mean, cov, box, score);
        }
    }

    /**
     * @brief Returns the squared Mahalanobis distance of a box 20 px right and 20 px below the
     *        predicted start box (100, 200, 50, 80) for a noise scale of @p noise_scale.
     *
     * The predicted variances of the start box at score 0.5 are 28.515625 for x and 73 for y;
     * the measurement noise adds 6.25 and 16 times the noise scale, and the difference is
     * (20, 20, 0, 0), so the distance is 20^2 / S_x + 20^2 / S_y.
     */
    double expected_distance(double noise_scale)
    {
        return 400.0 / (28.515625 + 6.25 * noise_scale) + 400.0 / (73.0 + 16.0 * noise_scale);
    }
} // namespace

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
 * @brief NSAKalmanFilter::initiate() produces a covariance whose diagonal is positive.
 */
TEST_F(Small_NSAKalmanFilterTest, InitiateCovarianceDiagonalIsPositive)
{
    kf_.initiate(mean_, cov_, default_box_, 0.8f);

    EXPECT_GT(cov_.diagonal().minCoeff(), 0.f);
}

/**
 * @brief Twenty predict-update cycles on one box leave the mean and covariance finite and the
 *        covariance diagonal positive.
 */
TEST_F(Small_NSAKalmanFilterTest, PredictUpdateCycleIsNumericallyStable)
{
    kf_.initiate(mean_, cov_, default_box_, 0.8f);

    run_cycles(kf_, mean_, cov_, default_box_, 0.8f, 20);

    EXPECT_TRUE(mean_.allFinite());
    EXPECT_TRUE(cov_.allFinite());
    EXPECT_GT(cov_.diagonal().minCoeff(), 0.f);
}

/**
 * @brief initiate() scales the position and size variances by the noise scale 1 - score.
 *
 * Each of these variances is (k * dimension)^2 times the noise scale, so the variance at score
 * 0.1 (noise scale 1 - 0.1) over the one at score 0.9 (noise scale 1 - 0.9) is the ratio of the
 * two scales, about 9. The scales are taken from the float scores. The bound is 12 u times 9.
 */
TEST_F(Small_NSAKalmanFilterTest, InitiatePositionVarianceScalesWithTheNoiseScale)
{
    StateMean mean_high, mean_low;
    StateCov cov_high, cov_low;
    const double expected_ratio = (1.0 - static_cast<double>(0.1f)) / (1.0 - static_cast<double>(0.9f));

    kf_.initiate(mean_high, cov_high, default_box_, 0.9f);
    kf_.initiate(mean_low, cov_low, default_box_, 0.1f);

    const Eigen::Array<double, 4, 1> ratio = cov_low.diagonal().head<4>().cast<double>().array() /
                                             cov_high.diagonal().head<4>().cast<double>().array();
    const double deviation = (ratio - expected_ratio).abs().maxCoeff();
    EXPECT_TRUE(TestSupport::is_within(TestSupport::measure_error(
        deviation, 0.0, 12.0 * TestSupport::unit_roundoff<float>() * expected_ratio)));
}

/**
 * @brief initiate() leaves the velocity variances unscaled: they do not depend on the score.
 */
TEST_F(Small_NSAKalmanFilterTest, InitiateVelocityVarianceIgnoresTheScore)
{
    StateMean mean_high, mean_low;
    StateCov cov_high, cov_low;

    kf_.initiate(mean_high, cov_high, default_box_, 0.9f);
    kf_.initiate(mean_low, cov_low, default_box_, 0.1f);

    EXPECT_TRUE(cov_low.diagonal().tail<4>() == cov_high.diagonal().tail<4>());
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
 * @brief The batch squared distances use each measurement's own score: a measurement 20 px off
 *        in x and y has distance 20^2 / S_x + 20^2 / S_y, with the noise scale 1 - score in S.
 *
 * At score 0.9 that is about 19.09 and at score 0.1 about 16.29. The bound is 24 u times the
 * expected distance, for the covariance chain and the solve.
 */
TEST_F(Small_NSAKalmanFilterTest, BatchSquaredDistancesFollowTheScores)
{
    kf_.initiate(mean_, cov_, default_box_, 0.5f);
    kf_.predict(mean_, cov_, 1.0f);
    const KalmanTypes::DetectBoxVector measurements = {make_box(120.f, 220.f, 50.f, 80.f),
                                                       make_box(120.f, 220.f, 50.f, 80.f)};

    const std::vector<float> distances =
        kf_.get_gate_sq_distances(mean_, cov_, measurements, {0.9f, 0.1f});

    ASSERT_EQ(distances.size(), 2U);
    const double high = expected_distance(1.0 - static_cast<double>(0.9f));
    const double low = expected_distance(1.0 - static_cast<double>(0.1f));
    const double unit = TestSupport::unit_roundoff<float>();
    EXPECT_TRUE(TestSupport::is_within(
        TestSupport::measure_error(distances[0], high, 24.0 * unit * high)));
    EXPECT_TRUE(TestSupport::is_within(
        TestSupport::measure_error(distances[1], low, 24.0 * unit * low)));
}

/**
 * @brief The batch gate flags use each measurement's own score.
 *
 * The two distances are about 19.09 at score 0.9 and about 16.29 at score 0.1 (see the test of
 * the batch distances), so a threshold of 17.5 lies between them: the high-score measurement
 * fails the gate and the low-score one passes.
 */
TEST_F(Small_NSAKalmanFilterTest, BatchGateFlagsFollowTheScores)
{
    kf_.initiate(mean_, cov_, default_box_, 0.5f);
    kf_.predict(mean_, cov_, 1.0f);
    const KalmanTypes::DetectBoxVector measurements = {make_box(120.f, 220.f, 50.f, 80.f),
                                                       make_box(120.f, 220.f, 50.f, 80.f)};

    const std::vector<bool> flags =
        kf_.get_gate_flags(mean_, cov_, measurements, 17.5f, {0.9f, 0.1f});

    EXPECT_EQ(flags, (std::vector<bool>{false, true}));
}

/**
 * @brief A negative score gives the covariance a score of zero gives.
 *
 * Both take a noise scale of 1: a negative score selects 1 and a score of zero gives
 * max(1 - 0, 0.01) = 1, so the two covariances are the same bit for bit.
 */
TEST_F(Small_NSAKalmanFilterTest, NegativeScoreGivesTheCovarianceOfScoreZero)
{
    StateMean mean_neg, mean_zero;
    StateCov cov_neg, cov_zero;

    kf_.initiate(mean_neg, cov_neg, default_box_, -0.5f);
    kf_.initiate(mean_zero, cov_zero, default_box_, 0.0f);

    EXPECT_EQ(cov_neg, cov_zero);
}

/**
 * @brief In XYAH mode, initiate() stores [cx, cy, aspect, height] as given with zero velocity.
 */
TEST(Small_NSAKalmanFilterXYAHTest, InitiateMeanMatchesMeasurement)
{
    NSAKalmanFilter kf_xyah(true);
    StateMean mean;
    StateCov cov;

    kf_xyah.initiate(mean, cov, make_box(150.f, 300.f, 0.6f, 120.f), 0.8f);

    EXPECT_EQ(mean, make_state(150.f, 300.f, 0.6f, 120.f, 0.f, 0.f, 0.f, 0.f));
}

/**
 * @brief In XYAH mode, initiate() produces a covariance whose diagonal is positive.
 */
TEST(Small_NSAKalmanFilterXYAHTest, InitiateCovarianceDiagonalIsPositive)
{
    NSAKalmanFilter kf_xyah(true);
    StateMean mean;
    StateCov cov;

    kf_xyah.initiate(mean, cov, make_box(150.f, 300.f, 0.6f, 120.f), 0.8f);

    EXPECT_GT(cov.diagonal().minCoeff(), 0.f);
}

/**
 * @brief An update with a detection score of zero leaves no covariance diagonal entry negative.
 */
TEST_F(Small_NSAKalmanFilterTest, ScoreZeroDoesNotProduceNegativeCovariance)
{
    const DetectBox box = make_box(10.f, 10.f, 20.f, 20.f);
    kf_.initiate(mean_, cov_, box, 1.0f);
    kf_.predict(mean_, cov_, 1.0f);

    kf_.update(mean_, cov_, box, 0.0f);

    EXPECT_GE(cov_.diagonal().minCoeff(), 0.0f);
}
