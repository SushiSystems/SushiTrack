/**************************************************************************/
/* test_imm_filter.cpp                                                    */
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
 * @file test_imm_filter.cpp
 * @brief Tests IMMFilter, the four-mode interacting multiple model estimator.
 * @author Mustafa Garip
 */

#include <algorithm>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/filtering/imm_filter.hpp>
#include <SushiTrack/filtering/kalman_types.hpp>

#include <measurement_builders.hpp>
#include <support/tolerance.hpp>

using namespace SushiTrack;

namespace
{
    /**
     * @brief Runs one predict and one update per entry of @p xs, each update with a box whose x is
     *        that entry.
     */
    void feed(IMMFilter& filter, KalmanTypes::StateMean& mean, KalmanTypes::StateCov& covariance,
              const std::vector<float>& xs)
    {
        for (const float x : xs)
        {
            filter.predict(mean, covariance, 1.0f);
            filter.update(mean, covariance, make_box(x, 200.f, 50.f, 80.f), 0.9f);
        }
    }
} // namespace

/**
 * @brief A started filter holds the constant-velocity prior: manoeuvre 0.2 (CA and CT), stationary
 *        0.1 (NCP).
 *
 * CA, NCP and CT start at 0.10f; 0.10f + 0.10f is exactly 0.2f, since doubling is exact.
 */
TEST(Small_IMMFilterTest, StartsOnTheConstantVelocityPrior)
{
    IMMFilter filter;
    KalmanTypes::StateMean mean;
    KalmanTypes::StateCov covariance;
    filter.initiate(mean, covariance, make_box(100.f, 200.f, 50.f, 80.f), 0.9f);

    EXPECT_FLOAT_EQ(filter.maneuver_probability(), 0.2f);
    EXPECT_FLOAT_EQ(filter.stationary_probability(), 0.1f);
}

/**
 * @brief The mode probabilities stay a distribution through predict and update, and the state
 *        stays finite.
 *
 * Four products divided by their sum: the sum carries 3 roundings, each division 1, and adding
 * three of the four 2 more, so the excess over 1 is at most 6u to first order, u the float unit
 * roundoff. The helper's threshold of 16 also covers the reset to the mixed prior.
 */
TEST(Small_IMMFilterTest, ModeProbabilitiesStayWithinZeroAndOne)
{
    IMMFilter filter;
    KalmanTypes::StateMean mean;
    KalmanTypes::StateCov covariance;
    filter.initiate(mean, covariance, make_box(100.f, 200.f, 50.f, 80.f), 0.9f);
    feed(filter, mean, covariance,
         {104.f, 108.f, 112.f, 116.f, 120.f, 124.f, 128.f, 132.f, 136.f, 140.f});

    const float manoeuvre = filter.maneuver_probability();
    const float stationary = filter.stationary_probability();
    EXPECT_GE(manoeuvre, 0.f);
    EXPECT_GE(stationary, 0.f);
    const double excess = std::max(0.0, static_cast<double>(manoeuvre) + stationary - 1.0);
    EXPECT_TRUE(TestSupport::is_within(
        TestSupport::measure_error(excess, 0.0, 6.0 * TestSupport::unit_roundoff<float>())));
    EXPECT_TRUE(mean.allFinite());
}

/**
 * @brief restore_checkpoint brings back the mode probabilities save_checkpoint took.
 *
 * The restore copies the saved probabilities and derives the manoeuvre sum from them with the
 * same addition, so the value is bit for bit the saved one.
 */
TEST(Small_IMMFilterTest, RestoreCheckpointBringsBackTheModeProbabilities)
{
    IMMFilter filter;
    KalmanTypes::StateMean mean;
    KalmanTypes::StateCov covariance;
    filter.initiate(mean, covariance, make_box(100.f, 200.f, 50.f, 80.f), 0.9f);
    filter.save_checkpoint();
    const float saved = filter.maneuver_probability();

    // x = 100 + 40 f^2 for f = 1..5
    feed(filter, mean, covariance, {140.f, 260.f, 460.f, 740.f, 1100.f});
    filter.restore_checkpoint();

    EXPECT_FLOAT_EQ(filter.maneuver_probability(), saved);
}
