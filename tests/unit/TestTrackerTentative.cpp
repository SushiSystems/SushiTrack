/**
 * @file      TestTrackerTentative.cpp
 * @brief     Tracker-level tests for tentative track age-out and promotion logic.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      12.05.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 *
 * Exercises the tentative buffer via the C-API update loop.
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
 * @brief Tracker fixture configured for time-aware tentative testing.
 *
 * track_buffer=2, frame_rate=25 → one frame interval = 0.04 s.
 * tentative_max_age_seconds=0.08 → track expires after 2 missed frames.
 * tentative_confirm_hits=3       → promotion requires three consecutive hits.
 */
class TrackerTentativeTest : public ::testing::Test
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

    /** @brief Returns a detection with the given top-left position. */
    SushiTrack_Object det(float x, float y) const
    {
        SushiTrack_Object o{};
        o.x = x; o.y = y;
        o.width = 50.0f; o.height = 100.0f;
        o.score = 0.9f;  o.label  = 1;
        return o;
    }
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

/**
 * @brief Tentative track must expire via accumulated lost time.
 *
 * Frame 1 — detection creates a Tentative track (hits=1, lost_time=0).
 * Frame 2 — no detection: lost_time accumulates to 0.04 s, state stays Tentative.
 * Frame 3 — no detection: lost_time exceeds max_age (0.08 s), track becomes Removed.
 */
TEST_F(TrackerTentativeTest, AgesOutViaPolicyWhenMissed)
{
    std::vector<SushiTrack_Track> out(10);
    SushiTrack_Object d0 = det(100.0f, 200.0f);

    // Frame 1 — initialisation
    int n = SushiTrack_Update(hdl_, &d0, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state,        4);    // Tentative
    EXPECT_EQ(out[0].is_confirmed, 0);
    EXPECT_FLOAT_EQ(out[0].lost_time, 0.0f);
    EXPECT_EQ(out[0].hits,         1);

    // Frame 2 — first miss
    n = SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state,  4);          // Still Tentative
    EXPECT_FLOAT_EQ(out[0].lost_time, 0.04f);

    // Frame 3 — second miss: total lost_time = 0.09 s > 0.08 s limit
    n = SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 0.05f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state,  3);          // Removed
    EXPECT_FLOAT_EQ(out[0].lost_time, 0.09f);
}

/**
 * @brief Tentative track is promoted to Tracked after reaching the hit threshold.
 *
 * Three consecutive matched frames must flip state to Tracked (1) and set
 * is_confirmed, with hit count equal to tentative_confirm_hits.
 */
TEST_F(TrackerTentativeTest, PromotedToTrackedAtHitThreshold)
{
    std::vector<SushiTrack_Track> out(10);

    SushiTrack_Update(hdl_, &det(100.0f, 200.0f), 1, out.data(), 10, 0.04f); // hit 1
    SushiTrack_Update(hdl_, &det(102.0f, 202.0f), 1, out.data(), 10, 0.04f); // hit 2

    int n = SushiTrack_Update(hdl_, &det(104.0f, 204.0f), 1, out.data(), 10, 0.04f); // hit 3

    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state,        1);    // Tracked
    EXPECT_EQ(out[0].is_confirmed, 1);
    EXPECT_EQ(out[0].hits,         3);
}
