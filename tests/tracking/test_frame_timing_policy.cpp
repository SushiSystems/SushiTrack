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
 * @file test_frame_timing_policy.cpp
 * @brief Tests FrameTimingPolicy.
 * @author Mustafa Garip
 */

#include <gtest/gtest.h>

#include <SushiTrack/tracking/frame_timing_policy.hpp>

using namespace SushiTrack;

// FrameTimingPolicy

/**
 * @brief to_kf_dt() returns 1.0 for any raw_dt value.
 */
TEST(Small_FrameTimingPolicyTest, ToKfDtAlwaysReturnsOne)
{
    FrameTimingPolicy p;
    EXPECT_FLOAT_EQ(p.to_kf_dt(0.f), 1.f);
    EXPECT_FLOAT_EQ(p.to_kf_dt(0.04f), 1.f);
    EXPECT_FLOAT_EQ(p.to_kf_dt(100.f), 1.f);
    EXPECT_FLOAT_EQ(p.to_kf_dt(-1.f), 1.f);
}

/**
 * @brief is_expired() returns false when frame_count equals buffer_frames
 *        (strict greater-than, not greater-than-or-equal).
 */
TEST(Small_FrameTimingPolicyTest, IsExpiredFalseAtExactThreshold)
{
    FrameTimingPolicy p;
    EXPECT_FALSE(p.is_expired(30.f, 30));
}

/**
 * @brief is_expired() returns false when frame_count is below buffer_frames.
 */
TEST(Small_FrameTimingPolicyTest, IsExpiredFalseBelow)
{
    FrameTimingPolicy p;
    EXPECT_FALSE(p.is_expired(29.f, 30));
}

/**
 * @brief is_expired() returns true when frame_count strictly exceeds buffer_frames.
 */
TEST(Small_FrameTimingPolicyTest, IsExpiredTrueAbove)
{
    FrameTimingPolicy p;
    EXPECT_TRUE(p.is_expired(31.f, 30));
}

/**
 * @brief FrameTimingPolicy::is_expired() with frame_count = 0 and buffer_frames = 0
 *        returns false (0 is not strictly greater than 0).
 */
TEST(Small_FrameTimingPolicyTest, ZeroFrameCountAndZeroBufferNotExpired)
{
    FrameTimingPolicy p;
    EXPECT_FALSE(p.is_expired(0.f, 0));
}
