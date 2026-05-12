/**
 * @file      TestTrackerBuffers.cpp
 * @brief     Tracker-level tests for Lost and Removed buffer lifecycle.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      12.05.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 *
 * Exercises Lost → Removed transitions via the C-API update loop.
 * Tracking state integers map to SushiTrack::TrackState:
 *   Tracked=1, Lost=2, Removed=3, Tentative=4
 */

#include <gtest/gtest.h>
#include "SushiTrackC.h"
#include <vector>

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

/**
 * @brief Tracker fixture configured for time-aware buffer expiry testing.
 *
 * track_buffer=2, frame_rate=25 → buffer window ≈ 0.08 s (2 frames).
 * tentative_confirm_hits=3       → track confirmed after three consecutive hits.
 */
class TrackerBuffersTest : public ::testing::Test
{
protected:
    SushiTrackHdl     hdl_    = nullptr;
    SushiTrack_Params params_ = {};

    void SetUp() override
    {
        SushiTrack_GetDefaultParams(&params_);
        params_.time_aware               = 1;
        params_.enable_tentative         = 1;
        params_.tentative_confirm_hits   = 3;
        params_.tentative_max_age_seconds = 0.08f;
        params_.track_buffer             = 2;
        params_.frame_rate               = 25;
        params_.tentative_match_thresh   = 0.9f;
        params_.track_thresh             = 0.1f;
        params_.high_thresh              = 0.2f;

        hdl_ = SushiTrack_Create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override
    {
        SushiTrack_Destroy(hdl_);
    }

    /** @brief Returns a detection at the given top-left position. */
    SushiTrack_Object det(float x, float y) const
    {
        SushiTrack_Object o{};
        o.x = x; o.y = y;
        o.width = 50.0f; o.height = 100.0f;
        o.score = 0.9f;  o.label  = 1;
        return o;
    }

    /**
     * @brief Drives the tracker through three matched frames to reach Tracked state.
     * @param out Output buffer (must have capacity >= 1).
     */
    void confirmTrack(std::vector<SushiTrack_Track>& out)
    {
        SushiTrack_Update(hdl_, &det(100.0f, 200.0f), 1, out.data(), 10, 0.04f);
        SushiTrack_Update(hdl_, &det(102.0f, 202.0f), 1, out.data(), 10, 0.04f);
        SushiTrack_Update(hdl_, &det(104.0f, 204.0f), 1, out.data(), 10, 0.04f);
        ASSERT_EQ(out[0].state, 1) << "Precondition: track must be Tracked before buffer tests";
    }
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

/**
 * @brief A confirmed Tracked track transitions to Lost on the first missed frame
 *        and accurately initialises its accumulated lost time.
 */
TEST_F(TrackerBuffersTest, TrackedTransitionsToLostOnFirstMiss)
{
    std::vector<SushiTrack_Track> out(10);
    confirmTrack(out);

    int n = SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 2);               // Lost
    EXPECT_FLOAT_EQ(out[0].lost_time, 0.04f);
}

/**
 * @brief Lost time accumulates monotonically across consecutive missed frames.
 */
TEST_F(TrackerBuffersTest, LostTimeAccumulatesAcrossMissedFrames)
{
    std::vector<SushiTrack_Track> out(10);
    confirmTrack(out);

    SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 0.04f); // 0.04 s
    int n = SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 0.04f); // 0.08 s

    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 2);               // Still Lost
    EXPECT_FLOAT_EQ(out[0].lost_time, 0.08f);
}

/**
 * @brief A Lost track moves to Removed once accumulated lost time exceeds
 *        the track buffer window.
 */
TEST_F(TrackerBuffersTest, LostTrackRemovedAfterBufferExpiry)
{
    std::vector<SushiTrack_Track> out(10);
    confirmTrack(out);

    // Exhaust two normal frames
    SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 0.04f);
    SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 0.04f);

    // Fast-forward well past the track_buffer window (2 frames = 0.08 s)
    int n = SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 2.0f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 3);               // Removed
}
