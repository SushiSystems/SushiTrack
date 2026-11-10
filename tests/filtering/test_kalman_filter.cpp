/**************************************************************************/
/* test_kalman_filter.cpp                                                 */
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
 * @file test_kalman_filter.cpp
 * @brief Tests KalmanFilter.
 * @author Mustafa Garip
 */

#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/filtering/kalman_filter.hpp>
#include <SushiTrack/filtering/kalman_types.hpp>

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
} // namespace

/**
 * @brief After initiation, the position and size block of the mean equals the measurement and
 *        the velocity block is zero. Both are copied or set, so the comparison is exact.
 */
TEST(Small_KalmanFilterTest, InitiateMeanMatchesMeasurement)
{
    KalmanFilter kf;
    StateMean mean;
    StateCov cov;

    kf.initiate(mean, cov, make_box(100.f, 200.f, 50.f, 80.f));

    EXPECT_EQ(mean, make_state(100.f, 200.f, 50.f, 80.f, 0.f, 0.f, 0.f, 0.f));
}

/**
 * @brief Every diagonal entry of the covariance produced by initiate() is positive.
 */
TEST(Small_KalmanFilterTest, InitiateCovarianceDiagonalIsPositive)
{
    KalmanFilter kf;
    StateMean mean;
    StateCov cov;

    kf.initiate(mean, cov, make_box(100.f, 200.f, 50.f, 80.f));

    EXPECT_GT(cov.diagonal().minCoeff(), 0.f);
}

/**
 * @brief After one predict step, every covariance diagonal entry is larger than before.
 *
 * Each entry gains the process noise, which is positive for the box 50 by 80, and the position
 * entries also gain the velocity variance.
 */
TEST(Small_KalmanFilterTest, PredictIncreasesCovariance)
{
    KalmanFilter kf;
    StateMean mean;
    StateCov cov;
    kf.initiate(mean, cov, make_box(100.f, 200.f, 50.f, 80.f));
    const StateCov cov_before = cov;

    kf.predict(mean, cov, 1.0f);

    EXPECT_GT((cov.diagonal() - cov_before.diagonal()).minCoeff(), 0.f);
}

/**
 * @brief An update moves the position by the Kalman gain times the innovation.
 *
 * x = 100, w = 50, no velocity: P_xx starts at 25 (0.1 * 50, squared) and P_vx at 9.765625
 * (3.125, squared). Predict (dt 1) gives P_xx = 25 + 9.765625 + 6.25 (0.05 * 50, squared) =
 * 41.015625; S_xx adds 6.25 for 47.265625. Gain = 2625 / 3025 = 105 / 121, innovation 10, so
 * x = 100 + 10 * 105 / 121. Bound: 32 u times 108.7.
 */
TEST(Small_KalmanFilterTest, UpdateMovesThePositionByTheGainTimesTheInnovation)
{
    KalmanFilter kf;
    StateMean mean;
    StateCov cov;
    kf.initiate(mean, cov, make_box(100.f, 200.f, 50.f, 80.f));
    kf.predict(mean, cov, 1.0f);

    kf.update(mean, cov, make_box(110.f, 200.f, 50.f, 80.f), 0.9f);

    const double bound = 32.0 * TestSupport::unit_roundoff<float>() * 108.7;
    EXPECT_TRUE(TestSupport::is_within(
        TestSupport::measure_error(mean(0), 100.0 + 10.0 * 105.0 / 121.0, bound)));
}

/**
 * @brief After a Kalman update, every covariance diagonal entry is smaller than after the
 *        predict. The predict correlates each velocity with its position, so the update lowers
 *        the velocity entries too.
 */
TEST(Small_KalmanFilterTest, UpdateDecreasesCovariance)
{
    KalmanFilter kf;
    StateMean mean;
    StateCov cov;
    const DetectBox box = make_box(100.f, 200.f, 50.f, 80.f);
    kf.initiate(mean, cov, box);
    kf.predict(mean, cov, 1.0f);
    const StateCov cov_after_predict = cov;

    kf.update(mean, cov, box, 0.9f);

    EXPECT_GT((cov_after_predict.diagonal() - cov.diagonal()).minCoeff(), 0.f);
}

/**
 * @brief A measurement equal to the box the state started on is in the chi-squared gate
 *        (threshold 9.4877 for 4 degrees of freedom): its squared distance is zero.
 */
TEST(Small_KalmanFilterTest, IsInGateAcceptsTheMeasurementTheStateStartedOn)
{
    KalmanFilter kf;
    StateMean mean;
    StateCov cov;
    const DetectBox box = make_box(100.f, 200.f, 50.f, 80.f);
    kf.initiate(mean, cov, box);

    EXPECT_TRUE(kf.is_in_gate(mean, cov, box, 9.4877f));
}

/**
 * @brief get_gate_flags() gives, for a near and a far box, the verdict is_in_gate() gives for
 *        each: the near box in and the far one out.
 */
TEST(Small_KalmanFilterTest, GetGateFlagsMatchesScalarIsInGate)
{
    KalmanFilter kf;
    StateMean mean;
    StateCov cov;
    kf.initiate(mean, cov, make_box(100.f, 200.f, 50.f, 80.f));
    const DetectBox near_box = make_box(100.f, 200.f, 50.f, 80.f);
    const DetectBox far_box = make_box(5000.f, 5000.f, 50.f, 80.f);
    KalmanTypes::DetectBoxVector boxes;
    boxes.push_back(near_box);
    boxes.push_back(far_box);

    const std::vector<bool> flags = kf.get_gate_flags(mean, cov, boxes, 9.4877f);

    ASSERT_EQ(flags.size(), 2U);
    EXPECT_EQ(flags[0], kf.is_in_gate(mean, cov, near_box, 9.4877f));
    EXPECT_EQ(flags[1], kf.is_in_gate(mean, cov, far_box, 9.4877f));
    EXPECT_EQ(flags, (std::vector<bool>{true, false}));
}

/**
 * @brief With noise multipliers above 1, predict grows every covariance diagonal entry more than
 *        it does with the identity multipliers.
 */
TEST(Small_KalmanFilterTest, ElevatedMultipliersIncreaseProcessNoise)
{
    KalmanFilter kf_base, kf_elevated;
    StateMean mean_base, mean_elev;
    StateCov cov_base, cov_elev;
    const DetectBox box = make_box(100.f, 200.f, 50.f, 80.f);
    kf_base.initiate(mean_base, cov_base, box);
    kf_elevated.initiate(mean_elev, cov_elev, box);
    kf_elevated.set_noise_multipliers(2.0f, 2.0f, 2.0f);

    kf_base.predict(mean_base, cov_base, 1.0f);
    kf_elevated.predict(mean_elev, cov_elev, 1.0f);

    EXPECT_GT((cov_elev.diagonal() - cov_base.diagonal()).minCoeff(), 0.f);
}

/**
 * @brief In XYAH mode, initiate() stores [cx, cy, aspect, height] as given with zero velocity.
 */
TEST(Small_KalmanFilterTest, XYAHModeInitiateMeanMatchesMeasurement)
{
    KalmanFilter kf(/*enable_xyah=*/true);
    StateMean mean;
    StateCov cov;

    kf.initiate(mean, cov, make_box(150.f, 300.f, 0.6f, 120.f));

    EXPECT_EQ(mean, make_state(150.f, 300.f, 0.6f, 120.f, 0.f, 0.f, 0.f, 0.f));
}

/**
 * @brief Verifies multi-step prediction logic when the time step (dt) is greater than one.
 *        The position components are expected to shift linearly based on constant velocity.
 */
TEST(Small_KalmanFilterTest, PredictWithDtGreaterThanOne)
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
 * @brief In XYAH mode, a predict and an update with the box the state started on leave the mean
 *        as it was.
 *
 * The velocities are zero, so predict adds 0 to every position. The innovation is the box minus
 * the projected mean, which is the mean itself, so it is exactly zero and the update adds zero.
 */
TEST(Small_KalmanFilterTest, XyahUpdateWithTheStartBoxKeepsTheMean)
{
    KalmanFilter kf(true);
    StateMean mean;
    StateCov cov;
    const DetectBox box = make_box(50.f, 50.f, 0.5f, 100.f);
    kf.initiate(mean, cov, box, 1.0f);

    kf.predict(mean, cov, 1.0f);
    kf.update(mean, cov, box, 1.0f);

    EXPECT_EQ(mean, make_state(50.f, 50.f, 0.5f, 100.f, 0.f, 0.f, 0.f, 0.f));
}
