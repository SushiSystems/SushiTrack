/**************************************************************************/
/* test_wall_clock_timing_policy.cpp                                      */
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
 * @file test_wall_clock_timing_policy.cpp
 * @brief Tests WallClockTimingPolicy.
 * @author Mustafa Garip
 */

#include <gtest/gtest.h>

#include <SushiTrack/tracking/wall_clock_timing_policy.hpp>

using namespace SushiTrack;

namespace
{
    /**
     * @brief One to_kf_dt() call at a frame rate and the step it must give.
     */
    struct StepRow
    {
            int fps;
            float raw_dt;
            float step;
    };

    /**
     * @brief One is_expired() call at a frame rate and its verdict.
     */
    struct ExpiryRow
    {
            int fps;
            float accumulated_seconds;
            int buffer_frames;
            bool expired;
    };
} // namespace

/**
 * @brief to_kf_dt() converts seconds to frame steps, snaps a value within 1e-4 of one to
 *        exactly one, and caps the step at K_MAX_KF_DT_STEPS.
 */
class Small_WallClockTimingPolicyStepTest : public ::testing::TestWithParam<StepRow>
{
};

TEST_P(Small_WallClockTimingPolicyStepTest, ToKfDtConvertsSecondsToFrameSteps)
{
    const WallClockTimingPolicy policy(GetParam().fps);

    EXPECT_FLOAT_EQ(policy.to_kf_dt(GetParam().raw_dt), GetParam().step);
}

INSTANTIATE_TEST_SUITE_P(
    Intervals, Small_WallClockTimingPolicyStepTest,
    ::testing::Values(
        // one frame at 25 fps: 0.04 * 25 = 1, snapped to exactly 1
        StepRow{25, 1.f / 25.f, 1.f},
        // 0.99998 frames is within 1e-4 of one and snaps to 1
        StepRow{25, 0.999980f / 25.f, 1.f},
        // half a frame at 16 fps: both 1 / 32 and 16 are exact, so the step is 0.5
        StepRow{16, 1.f / 32.f, 0.5f},
        // a 10 s gap is 250 frames, capped at 5
        StepRow{25, 10.f, 5.f},
        // the interval that is exactly the cap: 5 / 25 s gives 5
        StepRow{25, K_MAX_KF_DT_STEPS / 25.f, K_MAX_KF_DT_STEPS},
        // a frame rate of 0 is treated as 1: one second is one frame
        StepRow{0, 1.f, 1.f}));

/**
 * @brief is_expired() is true only when the accumulated seconds pass buffer_frames / fps by
 *        more than 1e-4.
 */
class Small_WallClockTimingPolicyExpiryTest : public ::testing::TestWithParam<ExpiryRow>
{
};

TEST_P(Small_WallClockTimingPolicyExpiryTest, IsExpiredOnlyPastTheTimeout)
{
    const WallClockTimingPolicy policy(GetParam().fps);

    EXPECT_EQ(policy.is_expired(GetParam().accumulated_seconds, GetParam().buffer_frames),
              GetParam().expired);
}

INSTANTIATE_TEST_SUITE_P(
    AroundTheTimeout, Small_WallClockTimingPolicyExpiryTest,
    ::testing::Values(
        // 30 frames at 25 fps is 1.2 s; 1.0 s is under it
        ExpiryRow{25, 1.0f, 30, false},
        // 1.2 s is the timeout itself and does not pass it
        ExpiryRow{25, 1.2f, 30, false},
        // 1.201 s is 1e-3 over the timeout, more than the 1e-4 slack
        ExpiryRow{25, 1.201f, 30, true},
        // a frame rate of 0 is treated as 1: 5 frames are 5 s
        ExpiryRow{0, 4.9f, 5, false},
        ExpiryRow{0, 5.1f, 5, true}));

/**
 * @brief to_kf_dt() with a negative raw_dt does not return a positive value
 *        (negative intervals should not produce positive frame counts).
 */
TEST(Small_WallClockTimingPolicyTest, NegativeDtProducesNonPositiveKfDt)
{
    WallClockTimingPolicy p(25);
    float result = p.to_kf_dt(-1.0f);
    EXPECT_LE(result, 0.f);
}
