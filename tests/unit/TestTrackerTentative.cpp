/****************************************************************/
/* TestTrackerTentative.cpp                                     */
/* SushiTrack - https://github.com/SushiSystems/SushiTrack      */
/* Copyright (c) 2026 Sushi Systems                             */
/* Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.    */
/* Commercial use requires a licence from Sushi Systems.        */
/****************************************************************/

/**
 * @file TestTrackerTentative.cpp
 * @brief Unit tests for tentative track lifecycle validation.
 * @author Mustafa Garip
 */

#include <gtest/gtest.h>
#include <SushiTrack/sushitrack_c.h>
#include <vector>

// TrackerTentativeTest

/**
 * @brief Tracker fixture configured for time-aware tentative testing.
 *
 * track_buffer=2, frame_rate=25 → one frame interval = 0.04 s.
 * time_aware=1 → age-out uses the wall-clock seconds budget (tentative_max_age_seconds).
 * tentative_max_age_seconds=0.08 → track expires once accumulated lost_time exceeds 0.08 s.
 * tentative_confirm_hits=3       → promotion requires three consecutive hits.
 */
class TrackerTentativeTest : public ::testing::Test
{
protected:
    sushitrack_tracker_t     hdl_    = nullptr;
    sushitrack_params_t params_ = {};

    void SetUp() override
    {
        sushitrack_get_default_params(&params_);
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

        hdl_ = sushitrack_create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override { sushitrack_destroy(hdl_); }

    /** @brief Returns a detection with the given top-left position. */
    sushitrack_object_t det(float x, float y) const
    {
        sushitrack_object_t o{};
        o.x = x; o.y = y;
        o.width = 50.0f; o.height = 100.0f;
        o.score = 0.9f;  o.label  = 1;
        return o;
    }
};

/**
 * @brief In time-aware mode a tentative ages out on the wall-clock seconds budget.
 *
 * Under the fixture's time_aware=1, age-out tracks real elapsed time: each missed frame
 * adds its dt to lost_time and removal happens once it exceeds tentative_max_age_seconds.
 * The larger dt on the second miss is what tips it past 0.08 s, so removal occurs after
 */
TEST_F(TrackerTentativeTest, AgesOutViaSecondsBudgetWhenTimeAware)
{
    std::vector<sushitrack_track_t> out(10);
    sushitrack_object_t d0 = det(100.0f, 200.0f);

    int n = sushitrack_update(hdl_, &d0, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state,        4);
    EXPECT_EQ(out[0].is_confirmed, 0);
    EXPECT_EQ(out[0].hits,         1);

    n = sushitrack_update(hdl_, nullptr, 0, out.data(), 10, 0.04f); // lost_time 0.04
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 4);
    EXPECT_FLOAT_EQ(out[0].lost_time, 0.04f);

    n = sushitrack_update(hdl_, nullptr, 0, out.data(), 10, 0.05f); // lost_time 0.09 > 0.08
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 3);
    EXPECT_FLOAT_EQ(out[0].lost_time, 0.09f);
}

/**
 * @brief In frame-counted mode a tentative ages out on a fixed missed-frame budget.
 *
 * With time_aware=0 the per-frame dt is ignored (effective_dt = 1/frame_rate) and age-out
 * uses tentative_max_miss_frames=2, so strictness is constant regardless of frame rate.
 * The varying dt passed to Update has no effect on the outcome; removal happens on the
 */
TEST(TrackerTentativeFrameBudgetTest, AgesOutViaFrameBudgetWhenFrameAware)
{
    sushitrack_params_t params = {};
    sushitrack_get_default_params(&params);
    params.time_aware                = 0;
    params.enable_tentative          = 1;
    params.tentative_confirm_hits    = 3;
    params.tentative_max_miss_frames = 2;
    params.frame_rate                = 25;
    params.tentative_match_thresh    = 0.9f;
    params.track_thresh              = 0.1f;
    params.high_thresh               = 0.2f;
    params.tentative_fast_thresh     = 1.0f;

    sushitrack_tracker_t hdl = sushitrack_create(&params);
    ASSERT_NE(hdl, nullptr);

    std::vector<sushitrack_track_t> out(10);
    sushitrack_object_t o{};
    o.x = 100.0f; o.y = 200.0f; o.width = 50.0f; o.height = 100.0f; o.score = 0.9f; o.label = 1;

    int n = sushitrack_update(hdl, &o, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 4);

    n = sushitrack_update(hdl, nullptr, 0, out.data(), 10, 0.05f); // 1st miss, dt ignored
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 4);

    n = sushitrack_update(hdl, nullptr, 0, out.data(), 10, 0.50f); // 2nd miss, dt ignored
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 4);

    n = sushitrack_update(hdl, nullptr, 0, out.data(), 10, 0.01f); // 3rd miss, removed
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 3);

    sushitrack_destroy(hdl);
}

/**
 * @brief Tentative track is promoted to Tracked after reaching the hit threshold.
 *
 * Three consecutive matched frames must flip state to Tracked (1) and set
 * is_confirmed, with hit count equal to tentative_confirm_hits.
 */
TEST_F(TrackerTentativeTest, PromotedToTrackedAtHitThreshold)
{
    std::vector<sushitrack_track_t> out(10);

    sushitrack_update(hdl_, &det(100.0f, 200.0f), 1, out.data(), 10, 0.04f);
    sushitrack_update(hdl_, &det(102.0f, 202.0f), 1, out.data(), 10, 0.04f);

    int n = sushitrack_update(hdl_, &det(104.0f, 204.0f), 1, out.data(), 10, 0.04f);
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
    std::vector<sushitrack_track_t> out(10);
    sushitrack_object_t d0 = det(100.0f, 200.0f);

    int n = sushitrack_update(hdl_, &d0, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 4);
    EXPECT_EQ(out[0].hits,  1);
    EXPECT_FLOAT_EQ(out[0].lost_time, 0.0f);

    n = sushitrack_update(hdl_, nullptr, 0, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 4);
    EXPECT_FLOAT_EQ(out[0].lost_time, 0.04f);

    n = sushitrack_update(hdl_, &d0, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 4);
    EXPECT_EQ(out[0].hits,  2);
    EXPECT_FLOAT_EQ(out[0].lost_time, 0.0f);
}

/**
 * @brief A miss whose dt lands exactly on the seconds budget does not expire the track.
 *
 * Under time_aware=1 the boundary is exclusive: lost_time reaching tentative_max_age_seconds
 * (0.08 s) exactly keeps the track, which must remain non-Tracked rather than be removed.
 *
 */
TEST_F(TrackerTentativeTest, SingleMissDoesNotExpire)
{
    std::vector<sushitrack_track_t> out(10);
    sushitrack_object_t d0 = det(100.0f, 200.0f);

    sushitrack_update(hdl_, &d0, 1, out.data(), 10, 0.04f);

    int n = sushitrack_update(hdl_, nullptr, 0, out.data(), 10, 0.08f);
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
    std::vector<sushitrack_track_t> out(10);

    sushitrack_object_t both[2];
    both[0] = det(100.0f, 200.0f);
    both[1] = det(700.0f, 500.0f);

    int n = sushitrack_update(hdl_, both, 2, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 2);

    sushitrack_object_t first = det(100.0f, 200.0f);
    n = sushitrack_update(hdl_, &first, 1, out.data(), 10, 0.04f);
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
    std::vector<sushitrack_track_t> out(10);

    sushitrack_update(hdl_, &det(100.0f, 200.0f), 1, out.data(), 10, 0.04f);
    sushitrack_update(hdl_, &det(102.0f, 202.0f), 1, out.data(), 10, 0.04f);
    int n = sushitrack_update(hdl_, &det(104.0f, 204.0f), 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 1);
    int64_t promoted_id = out[0].track_id;

    n = sushitrack_update(hdl_, &det(106.0f, 206.0f), 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].track_id, promoted_id);
    EXPECT_EQ(out[0].state,    1);
    EXPECT_EQ(out[0].hits,     4);
}

// TrackerTentativeInitSuppressTest — tentative_init_suppress_iou (#8)

namespace
{
    /** @brief Builds params with tentative enabled and the given init-suppression IoU. */
    sushitrack_params_t suppressParams(float suppress_iou)
    {
        sushitrack_params_t p = {};
        sushitrack_get_default_params(&p);
        p.time_aware                 = 1;
        p.enable_tentative           = 1;
        p.tentative_confirm_hits     = 3;
        p.tentative_fast_thresh      = 1.0f;
        p.tentative_match_thresh     = 0.9f;
        p.track_thresh               = 0.1f;
        p.high_thresh                = 0.2f;
        p.frame_rate                 = 25;
        p.tentative_init_suppress_iou = suppress_iou;
        return p;
    }

    /** @brief A 50x100 detection at (x, y) with score 0.9. */
    sushitrack_object_t obj(float x, float y)
    {
        sushitrack_object_t o{};
        o.x = x; o.y = y; o.width = 50.0f; o.height = 100.0f; o.score = 0.9f; o.label = 1;
        return o;
    }

    /** @brief Confirms a single track over three matched frames at (100,200)->(104,204). */
    void confirmTrack(sushitrack_tracker_t hdl, std::vector<sushitrack_track_t>& out)
    {
        sushitrack_update(hdl, &obj(100.0f, 200.0f), 1, out.data(), 10, 0.04f);
        sushitrack_update(hdl, &obj(102.0f, 202.0f), 1, out.data(), 10, 0.04f);
        int n = sushitrack_update(hdl, &obj(104.0f, 204.0f), 1, out.data(), 10, 0.04f);
        ASSERT_EQ(n, 1);
        ASSERT_EQ(out[0].state, 1);
    }
}

/**
 * @brief A detection overlapping a confirmed track does not spawn a new tentative.
 *
 * After confirming one track, a frame carries two detections: one on the track (matches
 * it) and one 5 px away that stays unmatched but overlaps the confirmed box well above
 * the suppression gate. With suppression on (default 0.7) the unmatched detection is
 * dropped, so only the confirmed track is output.
 */
TEST(TrackerTentativeInitSuppressTest, OverlappingDetectionDoesNotInitNewTentative)
{
    sushitrack_params_t params = suppressParams(0.7f);
    sushitrack_tracker_t hdl = sushitrack_create(&params);
    ASSERT_NE(hdl, nullptr);

    std::vector<sushitrack_track_t> out(10);
    confirmTrack(hdl, out);

    sushitrack_object_t pair[2] = { obj(106.0f, 206.0f), obj(111.0f, 206.0f) };
    int n = sushitrack_update(hdl, pair, 2, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 1);

    sushitrack_destroy(hdl);
}

/**
 * @brief Control: with suppression effectively disabled the overlapping detection inits
 *        a new tentative, proving the gate is what removes it in the test above.
 *
 * tentative_init_suppress_iou=2.0 can never be exceeded by an IoU in [0,1], so no
 * detection is ever suppressed and the unmatched overlapping detection becomes a
 * Tentative track alongside the confirmed one.
 */
TEST(TrackerTentativeInitSuppressTest, OverlappingDetectionInitsTentativeWhenGateDisabled)
{
    sushitrack_params_t params = suppressParams(2.0f);
    sushitrack_tracker_t hdl = sushitrack_create(&params);
    ASSERT_NE(hdl, nullptr);

    std::vector<sushitrack_track_t> out(10);
    confirmTrack(hdl, out);

    sushitrack_object_t pair[2] = { obj(106.0f, 206.0f), obj(111.0f, 206.0f) };
    int n = sushitrack_update(hdl, pair, 2, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 2);

    int confirmed = 0, tentative = 0;
    for (int i = 0; i < n; ++i)
    {
        if (out[i].state == 1) ++confirmed;
        else if (out[i].state == 4) ++tentative;
    }
    EXPECT_EQ(confirmed, 1);
    EXPECT_EQ(tentative, 1);

    sushitrack_destroy(hdl);
}

// TrackerTentativeDisabledTest — enable_tentative=0

/**
 * @brief Tracker fixture with tentative mode disabled.
 *
 * All parameters match TrackerTentativeTest except enable_tentative=0.
 * New tracks must enter Tracked state immediately on the first detection.
 */
class TrackerTentativeDisabledTest : public ::testing::Test
{
protected:
    sushitrack_tracker_t     hdl_    = nullptr;
    sushitrack_params_t params_ = {};

    void SetUp() override
    {
        sushitrack_get_default_params(&params_);
        params_.time_aware                = 1;
        params_.enable_tentative          = 0;
        params_.tentative_confirm_hits    = 3;
        params_.tentative_max_age_seconds = 0.08f;
        params_.track_buffer              = 2;
        params_.frame_rate                = 25;
        params_.tentative_match_thresh    = 0.9f;
        params_.track_thresh              = 0.1f;
        params_.high_thresh               = 0.2f;

        hdl_ = sushitrack_create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override { sushitrack_destroy(hdl_); }

    /** @brief Returns a detection at the given top-left position. */
    sushitrack_object_t det(float x, float y) const
    {
        sushitrack_object_t o{};
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
    std::vector<sushitrack_track_t> out(10);
    sushitrack_object_t d0 = det(100.0f, 200.0f);

    int n = sushitrack_update(hdl_, &d0, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state,        1);
    EXPECT_EQ(out[0].is_confirmed, 1);
}
