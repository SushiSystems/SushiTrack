/**
 * @file      TestTimingPolicies.cpp
 * @brief     Unit tests for FrameTimingPolicy and WallClockTimingPolicy.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      12.05.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 *
 * Coverage
 * --------
 * FrameTimingPolicy
 *   - toKfDt() always returns 1.0 regardless of input.
 *   - isExpired() returns false when frame_count <= buffer_frames.
 *   - isExpired() returns true when frame_count > buffer_frames.
 *
 * WallClockTimingPolicy
 *   - toKfDt() converts real dt=1/fps to exactly 1.0.
 *   - toKfDt() clips output at MAX_KF_DT_STEPS (5.0).
 *   - toKfDt() snaps values within 1e-4 of 1.0 to exactly 1.0.
 *   - isExpired() returns false when accumulated < timeout.
 *   - isExpired() returns true when accumulated > timeout.
 *   - fps=0 is clamped to fps=1 (no division-by-zero).
 */

#include <gtest/gtest.h>
#include "SushiTrack/TimingPolicies.h"

using namespace SushiTrack;

// ===========================================================================
// FrameTimingPolicy
// ===========================================================================

/**
 * @brief toKfDt() returns 1.0 for any raw_dt value.
 */
TEST(FrameTimingPolicyTest, ToKfDtAlwaysReturnsOne)
{
    FrameTimingPolicy p;
    EXPECT_FLOAT_EQ(p.toKfDt(0.f),    1.f);
    EXPECT_FLOAT_EQ(p.toKfDt(0.04f),  1.f);
    EXPECT_FLOAT_EQ(p.toKfDt(100.f),  1.f);
    EXPECT_FLOAT_EQ(p.toKfDt(-1.f),   1.f);
}

/**
 * @brief isExpired() returns false when frame_count equals buffer_frames
 *        (strict greater-than, not greater-than-or-equal).
 */
TEST(FrameTimingPolicyTest, IsExpiredFalseAtExactThreshold)
{
    FrameTimingPolicy p;
    EXPECT_FALSE(p.isExpired(30.f, 30));
}

/**
 * @brief isExpired() returns false when frame_count is below buffer_frames.
 */
TEST(FrameTimingPolicyTest, IsExpiredFalseBelow)
{
    FrameTimingPolicy p;
    EXPECT_FALSE(p.isExpired(29.f, 30));
}

/**
 * @brief isExpired() returns true when frame_count strictly exceeds buffer_frames.
 */
TEST(FrameTimingPolicyTest, IsExpiredTrueAbove)
{
    FrameTimingPolicy p;
    EXPECT_TRUE(p.isExpired(31.f, 30));
}

// ===========================================================================
// WallClockTimingPolicy
// ===========================================================================

/**
 * @brief toKfDt(1/fps) converts exactly one real frame to 1.0 (with snap).
 */
TEST(WallClockTimingPolicyTest, OneFpsIntervalConvertsToOne)
{
    WallClockTimingPolicy p(25);
    // 1/25 = 0.04 s; 0.04 * 25 = 1.0 — should snap to exactly 1.0.
    EXPECT_FLOAT_EQ(p.toKfDt(1.f / 25.f), 1.f);
}

/**
 * @brief toKfDt() is capped at MAX_KF_DT_STEPS (5.0).
 */
TEST(WallClockTimingPolicyTest, LargeDtIsCapped)
{
    WallClockTimingPolicy p(25);
    // 10 s gap * 25 fps = 250 frames; must be capped at 5.0.
    EXPECT_FLOAT_EQ(p.toKfDt(10.f), 5.f);
}

/**
 * @brief Values already very close to 1.0 (within 1e-4) snap to exactly 1.0.
 */
TEST(WallClockTimingPolicyTest, NearOneSnapsToPreciselyOne)
{
    WallClockTimingPolicy p(25);
    // Introduce a tiny floating-point error: 0.039999 * 25 = 0.99998 (≈1.0).
    float near_one_raw = 0.999980f / 25.f;
    EXPECT_FLOAT_EQ(p.toKfDt(near_one_raw), 1.f);
}

/**
 * @brief toKfDt() scales a half-frame interval to 0.5 at 25 fps.
 */
TEST(WallClockTimingPolicyTest, HalfFrameScalesCorrectly)
{
    WallClockTimingPolicy p(25);
    float dt = 0.5f / 25.f;   // 0.02 s
    EXPECT_NEAR(p.toKfDt(dt), 0.5f, 1e-4f);
}

/**
 * @brief isExpired() returns false when accumulated_seconds < timeout.
 */
TEST(WallClockTimingPolicyTest, IsExpiredFalseWhenUnderThreshold)
{
    WallClockTimingPolicy p(25);
    // buffer = 30 frames → timeout = 30/25 = 1.2 s
    EXPECT_FALSE(p.isExpired(1.0f, 30));
}

/**
 * @brief isExpired() returns false at exactly the timeout boundary.
 *
 * The implementation computes timeout = buffer_frames / fps_f_ at runtime.
 * Using the same expression in the test avoids a floating-point
 * representation mismatch between the 1.2f literal and the result of
 * 30.0f / 25.0f, which differ in the least-significant bit.
 */
TEST(WallClockTimingPolicyTest, IsExpiredFalseAtExactBoundary)
{
    WallClockTimingPolicy p(25);
    // Compute the timeout the same way the implementation does so both sides
    // of the comparison hold the identical float bit-pattern.
    const float timeout = static_cast<float>(30) / 25.0f;
    EXPECT_FALSE(p.isExpired(timeout, 30));
}

/**
 * @brief isExpired() returns true when accumulated_seconds > timeout.
 */
TEST(WallClockTimingPolicyTest, IsExpiredTrueWhenOverThreshold)
{
    WallClockTimingPolicy p(25);
    // timeout = 30/25 = 1.2 s; 1.201 s exceeds it.
    EXPECT_TRUE(p.isExpired(1.201f, 30));
}

/**
 * @brief Constructing with fps <= 0 clamps to 1 fps, preventing division-by-zero.
 */
TEST(WallClockTimingPolicyTest, ZeroFpsClampedToOneHz)
{
    WallClockTimingPolicy p(0);
    // With fps=1: toKfDt(1.0) = 1.0 (clamp applied after snap).
    float kf_dt = p.toKfDt(1.0f);
    EXPECT_FLOAT_EQ(kf_dt, 1.f);

    // isExpired with buffer=5, fps=1 → timeout = 5.0 s.
    EXPECT_FALSE(p.isExpired(4.9f, 5));
    EXPECT_TRUE(p.isExpired(5.1f, 5));
}
