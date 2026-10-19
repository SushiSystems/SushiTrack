/**************************************************************************/
/* TestTimingPolicies.cpp                                                 */
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
 * @file TestTimingPolicies.cpp
 * @brief Unit tests for frame and wall-clock timing policies.
 * @author Mustafa Garip
 */

#include <gtest/gtest.h>

#include <SushiTrack/tracking/frame_timing_policy.hpp>
#include <SushiTrack/tracking/wall_clock_timing_policy.hpp>

using namespace SushiTrack;

// FrameTimingPolicy

/**
 * @brief to_kf_dt() returns 1.0 for any raw_dt value.
 */
TEST(FrameTimingPolicyTest, ToKfDtAlwaysReturnsOne)
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
TEST(FrameTimingPolicyTest, IsExpiredFalseAtExactThreshold)
{
    FrameTimingPolicy p;
    EXPECT_FALSE(p.is_expired(30.f, 30));
}

/**
 * @brief is_expired() returns false when frame_count is below buffer_frames.
 */
TEST(FrameTimingPolicyTest, IsExpiredFalseBelow)
{
    FrameTimingPolicy p;
    EXPECT_FALSE(p.is_expired(29.f, 30));
}

/**
 * @brief is_expired() returns true when frame_count strictly exceeds buffer_frames.
 */
TEST(FrameTimingPolicyTest, IsExpiredTrueAbove)
{
    FrameTimingPolicy p;
    EXPECT_TRUE(p.is_expired(31.f, 30));
}

// WallClockTimingPolicy

/**
 * @brief to_kf_dt(1/fps) converts exactly one real frame to 1.0 (with snap).
 */
TEST(WallClockTimingPolicyTest, OneFpsIntervalConvertsToOne)
{
    WallClockTimingPolicy p(25);
    // 1/25 = 0.04 s; 0.04 * 25 = 1.0 — should snap to exactly 1.0.
    EXPECT_FLOAT_EQ(p.to_kf_dt(1.f / 25.f), 1.f);
}

/**
 * @brief to_kf_dt() is capped at MAX_KF_DT_STEPS (5.0).
 */
TEST(WallClockTimingPolicyTest, LargeDtIsCapped)
{
    WallClockTimingPolicy p(25);
    // 10 s gap * 25 fps = 250 frames; must be capped at 5.0.
    EXPECT_FLOAT_EQ(p.to_kf_dt(10.f), 5.f);
}

/**
 * @brief Values already very close to 1.0 (within 1e-4) snap to exactly 1.0.
 */
TEST(WallClockTimingPolicyTest, NearOneSnapsToPreciselyOne)
{
    WallClockTimingPolicy p(25);
    // Introduce a tiny floating-point error: 0.039999 * 25 = 0.99998 (≈1.0).
    float near_one_raw = 0.999980f / 25.f;
    EXPECT_FLOAT_EQ(p.to_kf_dt(near_one_raw), 1.f);
}

/**
 * @brief to_kf_dt() scales a half-frame interval to 0.5 at 25 fps.
 */
TEST(WallClockTimingPolicyTest, HalfFrameScalesCorrectly)
{
    WallClockTimingPolicy p(25);
    float dt = 0.5f / 25.f; // 0.02 s
    EXPECT_NEAR(p.to_kf_dt(dt), 0.5f, 1e-4f);
}

/**
 * @brief is_expired() returns false when accumulated_seconds < timeout.
 */
TEST(WallClockTimingPolicyTest, IsExpiredFalseWhenUnderThreshold)
{
    WallClockTimingPolicy p(25);
    // buffer = 30 frames → timeout = 30/25 = 1.2 s
    EXPECT_FALSE(p.is_expired(1.0f, 30));
}

/**
 * @brief is_expired() returns false at exactly the timeout boundary.
 *
 * The implementation computes timeout = buffer_frames / fps_f_ at runtime.
 * Using the same expression in the test avoids a floating-point
 * representation mismatch between the 1.2f literal and the result of
 * 30.0f / 25.0f, which differ in the least-significant bit.
 */
TEST(WallClockTimingPolicyTest, IsExpiredFalseAtExactBoundary)
{
    WallClockTimingPolicy p(25);
    // Compute the timeout the same way the implementation does so both sides of the comparison hold the identical float bit-pattern.
    const float timeout = static_cast<float>(30) / 25.0f;
    EXPECT_FALSE(p.is_expired(timeout, 30));
}

/**
 * @brief is_expired() returns true when accumulated_seconds > timeout.
 */
TEST(WallClockTimingPolicyTest, IsExpiredTrueWhenOverThreshold)
{
    WallClockTimingPolicy p(25);
    // timeout = 30/25 = 1.2 s; 1.201 s exceeds it.
    EXPECT_TRUE(p.is_expired(1.201f, 30));
}

/**
 * @brief Constructing with fps <= 0 clamps to 1 fps, preventing division-by-zero.
 */
TEST(WallClockTimingPolicyTest, ZeroFpsClampedToOneHz)
{
    WallClockTimingPolicy p(0);
    // With fps=1: to_kf_dt(1.0) = 1.0 (clamp applied after snap).
    float kf_dt = p.to_kf_dt(1.0f);
    EXPECT_FLOAT_EQ(kf_dt, 1.f);

    // is_expired with buffer=5, fps=1 → timeout = 5.0 s.
    EXPECT_FALSE(p.is_expired(4.9f, 5));
    EXPECT_TRUE(p.is_expired(5.1f, 5));
}

// Tier 3 edge cases

/**
 * @brief to_kf_dt() with a negative raw_dt does not return a positive value
 *        (negative intervals should not produce positive frame counts).
 */
TEST(WallClockTimingPolicyTest, NegativeDtProducesNonPositiveKfDt)
{
    WallClockTimingPolicy p(25);
    float result = p.to_kf_dt(-1.0f);
    EXPECT_LE(result, 0.f);
}

/**
 * @brief to_kf_dt() with a dt that would produce exactly MAX_KF_DT_STEPS is
 *        capped to that value.
 */
TEST(WallClockTimingPolicyTest, ExactCapBoundaryReturnsCap)
{
    WallClockTimingPolicy p(25);
    float dt_at_cap = K_MAX_KF_DT_STEPS / 25.f;
    EXPECT_FLOAT_EQ(p.to_kf_dt(dt_at_cap), K_MAX_KF_DT_STEPS);
}

/**
 * @brief is_expired() with a very small positive epsilon above the boundary
 *        returns true (boundary is open on the upper end).
 */
TEST(WallClockTimingPolicyTest, SmallEpsilonAboveBoundaryExpires)
{
    WallClockTimingPolicy p(25);
    const float timeout = static_cast<float>(30) / 25.0f;
    const float just_above = timeout + 1e-3f;
    EXPECT_TRUE(p.is_expired(just_above, 30));
}

/**
 * @brief FrameTimingPolicy::is_expired() with frame_count = 0 and buffer_frames = 0
 *        returns false (0 is not strictly greater than 0).
 */
TEST(FrameTimingPolicyTest, ZeroFrameCountAndZeroBufferNotExpired)
{
    FrameTimingPolicy p;
    EXPECT_FALSE(p.is_expired(0.f, 0));
}
