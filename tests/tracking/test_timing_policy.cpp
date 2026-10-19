/**************************************************************************/
/* test_timing_policy.cpp                                                 */
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
 * @file test_timing_policy.cpp
 * @brief Tests the TimingPolicy contract that both policies keep.
 * @author Mustafa Garip
 */

#include <memory>

#include <gtest/gtest.h>

#include <SushiTrack/tracking/frame_timing_policy.hpp>
#include <SushiTrack/tracking/timing_policy.hpp>
#include <SushiTrack/tracking/wall_clock_timing_policy.hpp>

using namespace SushiTrack;

namespace
{
    /** @brief Runs the frame policy (false) and the wall-clock policy at 25 fps (true). */
    class Small_TimingPolicyTest : public ::testing::TestWithParam<bool>
    {
        protected:
            /// @brief Returns the policy under test.
            [[nodiscard]] std::unique_ptr<TimingPolicy> policy() const
            {
                if (GetParam())
                    return std::make_unique<WallClockTimingPolicy>(25);
                return std::make_unique<FrameTimingPolicy>();
            }
    };
} // namespace

/**
 * @brief One frame interval is one Kalman step.
 */
TEST_P(Small_TimingPolicyTest, OneFrameIsOneStep)
{
    const float one_frame = GetParam() ? 1.f / 25.f : 1.f;
    EXPECT_NEAR(policy()->to_kf_dt(one_frame), 1.f, 1e-4f);
}

/**
 * @brief Nothing has expired at zero, and everything has expired far past the limit.
 */
TEST_P(Small_TimingPolicyTest, ExpiresOnlyPastTheLimit)
{
    const std::unique_ptr<TimingPolicy> timing = policy();
    const float far_past = GetParam() ? 100.f : 1000.f;

    EXPECT_FALSE(timing->is_expired(0.f, 30));
    EXPECT_TRUE(timing->is_expired(far_past, 30));
}

INSTANTIATE_TEST_SUITE_P(BothPolicies, Small_TimingPolicyTest, ::testing::Bool());
