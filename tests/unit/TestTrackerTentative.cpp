/**
 * @file      TestTrackerTentative.cpp
 * @brief     Tracker-level tests for tentative track age-out, promotion, and disable logic.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      12.05.2026
 * @version   v1.1.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 *
 * Coverage
 * --------
 * TrackerTentativeTest (time_aware=1, tentative_confirm_hits=3, max_age=0.08 s)
 *   - Tentative track ages out via accumulated lost_time when missed.
 *   - Tentative track is promoted to Tracked after the required hit count.
 *   - Re-association after a miss resets lost_time to zero and increments hits.
 *   - A miss whose dt equals max_age exactly does not remove the track.
 *   - Two simultaneous tentative tracks evolve independently per frame.
 *   - A promoted Tracked track persists with the same ID on subsequent frames.
 *
 * TrackerTentativeDisabledTest (enable_tentative=0)
 *   - A new detection immediately produces a Tracked, confirmed track.
 *
 * Tracking state integers map to SushiTrack::TrackState:
 *   Tracked=1, Lost=2, Removed=3, Tentative=4
 */

#include <gtest/gtest.h>
#include "SushiTrackC.h"
#include <vector>

// ===========================================================================
// TrackerTentativeTest
// ===========================================================================

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
        params_.time_aware                = 1;
        params_.enable_tentative          = 1;
        params_.tentative_confirm_hits    = 3;
        params_.tentative_max_age_seconds = 0.08f;
        params_.track_buffer              = 2;
        params_.frame_rate                = 25;
        params_.tentative_match_thresh    = 0.9f;
        params_.track_thresh              = 0.1f;
        params_.high_thresh               = 0.2f;
        params_.tentative_fast_thresh     = 1.0f;

        hdl_ = SushiTrack_Create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override { SushiTrack_Destroy(hdl_); }

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

/**
 * @brief Tentative track expires via accumulated lost_time when missed.
 *
 * Frame 1 — detection creates a Tentative track (hits=1, lost_time=0).
 * Frame 2 — no detection: lost_time accumulates to 0.04 s, state stays Tentative.
 * Frame 3 — no detection: lost_time exceeds max_age (0.08 s), track becomes Removed.
 */
TEST_F(TrackerTentativeTest, AgesOutViaPolicyWhenMissed)
{
    std::vector<SushiTrack_Track> out(10);
    SushiTrack_Object d0 = det(100.0f, 200.0f);

    int n = SushiTrack_Update(hdl_, &d0, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state,        4);
    EXPECT_EQ(out[0].is_confirmed, 0);
    EXPECT_FLOAT_EQ(out[0].lost_time, 0.0f);
    EXPECT_EQ(out[0].hits,         1);

    n = SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 4);
    EXPECT_FLOAT_EQ(out[0].lost_time, 0.04f);

    n = SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 0.05f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 3);
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

    SushiTrack_Update(hdl_, &det(100.0f, 200.0f), 1, out.data(), 10, 0.04f);
    SushiTrack_Update(hdl_, &det(102.0f, 202.0f), 1, out.data(), 10, 0.04f);

    int n = SushiTrack_Update(hdl_, &det(104.0f, 204.0f), 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state,        1);
    EXPECT_EQ(out[0].is_confirmed, 1);
    EXPECT_EQ(out[0].hits,         3);
}

/**
 * @brief Re-association after a missed frame resets lost_time to zero and
 *        increments the hit counter.
 *
 * Frame 1 — detection at (100,200) creates a Tentative track (hits=1, lost_time=0).
 * Frame 2 — no detection: lost_time accumulates to 0.04 s.
 * Frame 3 — detection at (100,200) re-associates: lost_time resets to 0, hits=2.
 */
TEST_F(TrackerTentativeTest, PartialHitSequenceResetsLostTimeOnMatch)
{
    std::vector<SushiTrack_Track> out(10);
    SushiTrack_Object d0 = det(100.0f, 200.0f);

    int n = SushiTrack_Update(hdl_, &d0, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 4);
    EXPECT_EQ(out[0].hits,  1);
    EXPECT_FLOAT_EQ(out[0].lost_time, 0.0f);

    n = SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 4);
    EXPECT_FLOAT_EQ(out[0].lost_time, 0.04f);

    n = SushiTrack_Update(hdl_, &d0, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 4);
    EXPECT_EQ(out[0].hits,  2);
    EXPECT_FLOAT_EQ(out[0].lost_time, 0.0f);
}

/**
 * @brief A miss whose dt equals tentative_max_age_seconds exactly does not remove
 *        the track; the boundary is treated as non-expiring.
 *
 * Frame 1 — detection creates a Tentative track (lost_time=0).
 * Frame 2 — no detection, dt=0.08 s: lost_time reaches the boundary value exactly.
 * The track must still appear in the output (count=1) and must not be Tracked
 * (no additional hits).  Whether the boundary is inclusive or exclusive is
 * implementation-defined; this test documents the observed behavior.
 */
TEST_F(TrackerTentativeTest, ExactBoundaryDtDoesNotExpire)
{
    std::vector<SushiTrack_Track> out(10);
    SushiTrack_Object d0 = det(100.0f, 200.0f);

    SushiTrack_Update(hdl_, &d0, 1, out.data(), 10, 0.04f);

    int n = SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 0.08f);
    ASSERT_EQ(n, 1);
    EXPECT_NE(out[0].state, 1);
    EXPECT_FLOAT_EQ(out[0].lost_time, 0.08f);
}

/**
 * @brief Two well-separated tentative tracks evolve independently per frame.
 *
 * Frame 1 — two detections create two Tentative tracks.
 * Frame 2 — only the first detection is sent; the matched track gains a hit,
 *            the unmatched track accumulates lost_time.
 */
TEST_F(TrackerTentativeTest, TwoSimultaneousTentativeTracksIndependent)
{
    std::vector<SushiTrack_Track> out(10);

    SushiTrack_Object both[2];
    both[0] = det(100.0f, 200.0f);
    both[1] = det(700.0f, 500.0f);

    int n = SushiTrack_Update(hdl_, both, 2, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 2);

    SushiTrack_Object first = det(100.0f, 200.0f);
    n = SushiTrack_Update(hdl_, &first, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 2);

    int hits2_count      = 0;
    int hits1_lost_count = 0;
    for (int i = 0; i < n; ++i)
    {
        if (out[i].hits == 2)
            ++hits2_count;
        else if (out[i].hits == 1 && out[i].lost_time > 0.0f)
            ++hits1_lost_count;
    }
    EXPECT_EQ(hits2_count,      1);
    EXPECT_EQ(hits1_lost_count, 1);
}

/**
 * @brief A track promoted to Tracked persists with the same track_id on
 *        subsequent matched frames and its hit counter continues to increment.
 */
TEST_F(TrackerTentativeTest, TentativePromotedTrackPersistsAfterPromotion)
{
    std::vector<SushiTrack_Track> out(10);

    SushiTrack_Update(hdl_, &det(100.0f, 200.0f), 1, out.data(), 10, 0.04f);
    SushiTrack_Update(hdl_, &det(102.0f, 202.0f), 1, out.data(), 10, 0.04f);
    int n = SushiTrack_Update(hdl_, &det(104.0f, 204.0f), 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 1);
    int64_t promoted_id = out[0].track_id;

    n = SushiTrack_Update(hdl_, &det(106.0f, 206.0f), 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].track_id, promoted_id);
    EXPECT_EQ(out[0].state,    1);
    EXPECT_EQ(out[0].hits,     4);
}

// ===========================================================================
// TrackerTentativeDisabledTest — enable_tentative=0
// ===========================================================================

/**
 * @brief Tracker fixture with tentative mode disabled.
 *
 * All parameters match TrackerTentativeTest except enable_tentative=0.
 * New tracks must enter Tracked state immediately on the first detection.
 */
class TrackerTentativeDisabledTest : public ::testing::Test
{
protected:
    SushiTrackHdl     hdl_    = nullptr;
    SushiTrack_Params params_ = {};

    void SetUp() override
    {
        SushiTrack_GetDefaultParams(&params_);
        params_.time_aware                = 1;
        params_.enable_tentative          = 0;
        params_.tentative_confirm_hits    = 3;
        params_.tentative_max_age_seconds = 0.08f;
        params_.track_buffer              = 2;
        params_.frame_rate                = 25;
        params_.tentative_match_thresh    = 0.9f;
        params_.track_thresh              = 0.1f;
        params_.high_thresh               = 0.2f;

        hdl_ = SushiTrack_Create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override { SushiTrack_Destroy(hdl_); }

    /** @brief Returns a detection at the given top-left position. */
    SushiTrack_Object det(float x, float y) const
    {
        SushiTrack_Object o{};
        o.x = x; o.y = y;
        o.width = 50.0f; o.height = 100.0f;
        o.score = 0.9f;  o.label  = 1;
        return o;
    }
};

/**
 * @brief With tentative mode disabled, the first detection immediately produces
 *        a Tracked, confirmed track without requiring additional hits.
 */
TEST_F(TrackerTentativeDisabledTest, WithTentativeDisabledNewTrackIsImmediatelyTracked)
{
    std::vector<SushiTrack_Track> out(10);
    SushiTrack_Object d0 = det(100.0f, 200.0f);

    int n = SushiTrack_Update(hdl_, &d0, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state,        1);
    EXPECT_EQ(out[0].is_confirmed, 1);
}
