/**************************************************************************/
/* test_frame_timing_policy.cpp                                           */
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
 * @file test_frame_timing_policy.cpp
 * @brief Tests FrameTimingPolicy.
 * @author Mustafa Garip
 */

#include <gtest/gtest.h>

#include <SushiTrack/tracking/frame_timing_policy.hpp>

using namespace SushiTrack;

/**
 * @brief to_kf_dt() returns 1.0 for any raw_dt: zero, a frame at 25 fps, a long gap and a
 *        negative interval.
 */
class Small_FrameTimingPolicyStepTest : public ::testing::TestWithParam<float>
{
};

TEST_P(Small_FrameTimingPolicyStepTest, ToKfDtAlwaysReturnsOne)
{
    const FrameTimingPolicy policy;

    EXPECT_FLOAT_EQ(policy.to_kf_dt(GetParam()), 1.f);
}

INSTANTIATE_TEST_SUITE_P(RawIntervals, Small_FrameTimingPolicyStepTest,
                         ::testing::Values(0.f, 0.04f, 100.f, -1.f));

namespace
{
    /**
     * @brief One is_expired() call and its verdict.
     */
    struct ExpiryRow
    {
            float frame_count;
            int buffer_frames;
            bool expired;
    };
} // namespace

/**
 * @brief is_expired() is true only when frame_count is strictly above buffer_frames.
 */
class Small_FrameTimingPolicyExpiryTest : public ::testing::TestWithParam<ExpiryRow>
{
};

TEST_P(Small_FrameTimingPolicyExpiryTest, IsExpiredOnlyAboveTheLimit)
{
    const FrameTimingPolicy policy;

    EXPECT_EQ(policy.is_expired(GetParam().frame_count, GetParam().buffer_frames),
              GetParam().expired);
}

INSTANTIATE_TEST_SUITE_P(AroundTheLimit, Small_FrameTimingPolicyExpiryTest,
                         ::testing::Values(
                             // 30 is not above 30
                             ExpiryRow{30.f, 30, false},
                             // 29 is below 30
                             ExpiryRow{29.f, 30, false},
                             // 31 is above 30
                             ExpiryRow{31.f, 30, true},
                             // 0 is not above a limit of 0
                             ExpiryRow{0.f, 0, false}));
