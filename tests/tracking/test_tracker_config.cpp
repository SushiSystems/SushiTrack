/**************************************************************************/
/* test_tracker_config.cpp                                                */
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
 * @file test_tracker_config.cpp
 * @brief Tests that the TrackerConfig defaults describe a tracker that can run.
 * @author Mustafa Garip
 */

#include <cmath>

#include <gtest/gtest.h>

#include <SushiTrack/tracking/tracker_config.hpp>

#include <support/tolerance.hpp>

using namespace SushiTrack;

namespace
{
    /** @brief A default field that is a fraction, by name and by member. */
    struct FractionRow
    {
            const char* name;
            float TrackerConfig::*field;
    };
} // namespace

/**
 * @brief The default frame rate and track buffer are positive.
 */
TEST(Small_TrackerConfigTest, DefaultFrameRateAndBufferArePositive)
{
    const TrackerConfig config;

    EXPECT_GT(config.frame_rate, 0);
    EXPECT_GT(config.track_buffer, 0);
}

/**
 * @brief A detection that starts a track is a high-confidence one: track_thresh is below
 *        high_thresh.
 */
TEST(Small_TrackerConfigTest, DefaultTrackThreshIsBelowHighThresh)
{
    const TrackerConfig config;

    EXPECT_LT(config.track_thresh, config.high_thresh);
}

/**
 * @brief The fractions of the config, run over each field.
 */
class Small_TrackerConfigFractionTest : public ::testing::TestWithParam<FractionRow>
{
};

/**
 * @brief Each default threshold and momentum is a fraction in (0, 1].
 */
TEST_P(Small_TrackerConfigFractionTest, DefaultIsAFraction)
{
    const TrackerConfig config;
    const float value = config.*(GetParam().field);

    EXPECT_GT(value, 0.0f) << GetParam().name;
    EXPECT_LE(value, 1.0f) << GetParam().name;
}

INSTANTIATE_TEST_SUITE_P(
    Fractions, Small_TrackerConfigFractionTest,
    ::testing::Values(FractionRow{"match_thresh", &TrackerConfig::match_thresh},
                      FractionRow{"iou_match_thresh", &TrackerConfig::iou_match_thresh},
                      FractionRow{"unconfirmed_iou_thresh", &TrackerConfig::unconfirmed_iou_thresh},
                      FractionRow{"duplicate_iou_thresh", &TrackerConfig::duplicate_iou_thresh},
                      FractionRow{"feature_momentum", &TrackerConfig::feature_momentum}));

/**
 * @brief The default gate is the 95% chi-square bound for four degrees of freedom.
 *
 * For four degrees of freedom the survival function is S(x) = exp(-x/2) * (1 + x/2), so the gate
 * must leave S = 0.05. The constant is typed to five significant digits, so x is off by at most
 * 0.5e-4 plus the float roundoff u * x, and S moves by at most |S'(x)| times that, with
 * |S'(x)| = (x / 4) * exp(-x/2).
 */
TEST(Small_TrackerConfigTest, DefaultGateIsTheFourDegreeChiSquareBound)
{
    const double gate = TrackerConfig{}.gating_thresh;
    const double survival = std::exp(-gate / 2.0) * (1.0 + gate / 2.0);
    const double slope = (gate / 4.0) * std::exp(-gate / 2.0);
    const double bound = slope * (0.5e-4 + TestSupport::unit_roundoff<float>() * gate);

    EXPECT_TRUE(TestSupport::is_within(TestSupport::measure_error(survival, 0.05, bound)));
}
