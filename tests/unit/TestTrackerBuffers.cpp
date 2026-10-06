/**************************************************************************/
/* TestTrackerBuffers.cpp                                                 */
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
 * @file TestTrackerBuffers.cpp
 * @brief Unit tests for tracker buffer management.
 * @author Mustafa Garip
 */

#include <gtest/gtest.h>
#include <SushiTrack/sushitrack_c.h>
#include <vector>

// TrackerBuffersTest — standard 0.08 s buffer window

/**
 * @brief Tracker fixture configured for time-aware buffer expiry testing.
 *
 * track_buffer=2, frame_rate=25 → buffer window ≈ 0.08 s (2 frames).
 * tentative_confirm_hits=3       → track confirmed after three consecutive hits.
 */
class TrackerBuffersTest : public ::testing::Test
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

    /**
     * @brief Drives the tracker through three matched frames to reach Tracked state.
     * @param out Output buffer (must have capacity >= 1).
     */
    void confirmTrack(std::vector<sushitrack_track_t>& out)
    {
        sushitrack_update(hdl_, &det(100.0f, 200.0f), 1, out.data(), 10, 0.04f);
        sushitrack_update(hdl_, &det(102.0f, 202.0f), 1, out.data(), 10, 0.04f);
        sushitrack_update(hdl_, &det(104.0f, 204.0f), 1, out.data(), 10, 0.04f);
        ASSERT_EQ(out[0].state, 1) << "Precondition: track must be Tracked before buffer tests";
    }
};

/**
 * @brief A confirmed Tracked track transitions to Lost on the first missed frame
 *        and accurately initialises its accumulated lost time.
 */
TEST_F(TrackerBuffersTest, TrackedTransitionsToLostOnFirstMiss)
{
    std::vector<sushitrack_track_t> out(10);
    confirmTrack(out);

    int n = sushitrack_update(hdl_, nullptr, 0, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 2);
    EXPECT_FLOAT_EQ(out[0].lost_time, 0.04f);
}

/**
 * @brief Lost time accumulates monotonically across consecutive missed frames.
 */
TEST_F(TrackerBuffersTest, LostTimeAccumulatesAcrossMissedFrames)
{
    std::vector<sushitrack_track_t> out(10);
    confirmTrack(out);

    sushitrack_update(hdl_, nullptr, 0, out.data(), 10, 0.04f);
    int n = sushitrack_update(hdl_, nullptr, 0, out.data(), 10, 0.04f);

    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 2);
    EXPECT_FLOAT_EQ(out[0].lost_time, 0.08f);
}

/**
 * @brief A Lost track moves to Removed once accumulated lost time exceeds
 *        the track buffer window.
 */
TEST_F(TrackerBuffersTest, LostTrackRemovedAfterBufferExpiry)
{
    std::vector<sushitrack_track_t> out(10);
    confirmTrack(out);

    sushitrack_update(hdl_, nullptr, 0, out.data(), 10, 0.04f);
    sushitrack_update(hdl_, nullptr, 0, out.data(), 10, 0.04f);

    int n = sushitrack_update(hdl_, nullptr, 0, out.data(), 10, 2.0f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 3);
}

/**
 * @brief A Lost track returns to Tracked state and its lost_time resets to zero
 *        when a detection is re-associated to it within the buffer window.
 */
TEST_F(TrackerBuffersTest, LostTrackRecoveredWhenReassociated)
{
    std::vector<sushitrack_track_t> out(10);
    confirmTrack(out);

    sushitrack_update(hdl_, nullptr, 0, out.data(), 10, 0.04f);
    ASSERT_EQ(out[0].state, 2) << "Precondition: track must be Lost before recovery test";

    sushitrack_object_t recovery = det(105.0f, 202.0f);
    int n = sushitrack_update(hdl_, &recovery, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 1);
    EXPECT_FLOAT_EQ(out[0].lost_time, 0.0f);
}

/**
 * @brief A track that reaches Removed state is absent from the output of the
 *        subsequent Update call.
 */
TEST_F(TrackerBuffersTest, RemovedTrackAbsentFromNextFrameOutput)
{
    std::vector<sushitrack_track_t> out(10);
    confirmTrack(out);

    sushitrack_update(hdl_, nullptr, 0, out.data(), 10, 0.04f);

    int n = sushitrack_update(hdl_, nullptr, 0, out.data(), 10, 2.0f);
    ASSERT_EQ(n, 1);
    ASSERT_EQ(out[0].state, 3) << "Precondition: track must reach Removed state";

    int n2 = sushitrack_update(hdl_, nullptr, 0, out.data(), 10, 1.0f);
    EXPECT_EQ(n2, 0);
}

/**
 * @brief Two detections at well-separated positions both reach Tracked state after
 *        three consecutive matched frames and are assigned distinct non-zero IDs.
 */
TEST_F(TrackerBuffersTest, MultipleTracksRetainIndependentIds)
{
    std::vector<sushitrack_track_t> out(10);

    sushitrack_object_t dets[2];
    dets[0] = det(100.0f, 200.0f);
    dets[1] = det(600.0f, 400.0f);

    sushitrack_update(hdl_, dets, 2, out.data(), 10, 0.04f);
    sushitrack_update(hdl_, dets, 2, out.data(), 10, 0.04f);
    int n = sushitrack_update(hdl_, dets, 2, out.data(), 10, 0.04f);

    ASSERT_EQ(n, 2);
    EXPECT_EQ(out[0].state, 1);
    EXPECT_EQ(out[1].state, 1);
    EXPECT_NE(out[0].track_id, 0);
    EXPECT_NE(out[1].track_id, 0);
    EXPECT_NE(out[0].track_id, out[1].track_id);
}

/**
 * @brief When no detections arrive after two confirmed tracks, both transition to
 *        Lost independently.  Re-presenting only the first track's detection returns
 *        that track to Tracked while the second remains Lost.
 */
TEST_F(TrackerBuffersTest, MultipleTrackIndependentLostState)
{
    std::vector<sushitrack_track_t> out(10);

    sushitrack_object_t dets[2];
    dets[0] = det(100.0f, 200.0f);
    dets[1] = det(600.0f, 400.0f);

    sushitrack_update(hdl_, dets, 2, out.data(), 10, 0.04f);
    sushitrack_update(hdl_, dets, 2, out.data(), 10, 0.04f);
    sushitrack_update(hdl_, dets, 2, out.data(), 10, 0.04f);

    int n_lost = sushitrack_update(hdl_, nullptr, 0, out.data(), 10, 0.04f);
    ASSERT_EQ(n_lost, 2);
    for (int i = 0; i < 2; ++i)
        EXPECT_EQ(out[i].state, 2);

    sushitrack_object_t single = det(100.0f, 200.0f);
    int n_mixed = sushitrack_update(hdl_, &single, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n_mixed, 2);

    int tracked_count = 0;
    int lost_count    = 0;
    for (int i = 0; i < n_mixed; ++i)
    {
        if (out[i].state == 1) ++tracked_count;
        else if (out[i].state == 2) ++lost_count;
    }
    EXPECT_EQ(tracked_count, 1);
    EXPECT_EQ(lost_count,    1);
}

// TrackerBuffersLargeBufferTest — wide 0.4 s expiry window

/**
 * @brief Tracker fixture with track_buffer=10 yielding a 0.4 s expiry window.
 *
 * track_buffer=10, frame_rate=25 → window = 10/25 = 0.4 s.
 * tentative_confirm_hits=3 → track confirmed after three consecutive hits.
 */
class TrackerBuffersLargeBufferTest : public ::testing::Test
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
        params_.track_buffer              = 10;
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

    /**
     * @brief Drives three matched frames to bring the track to Tracked state.
     * @param out Output buffer (capacity >= 1).
     */
    void confirmTrack(std::vector<sushitrack_track_t>& out)
    {
        sushitrack_update(hdl_, &det(100.0f, 200.0f), 1, out.data(), 10, 0.04f);
        sushitrack_update(hdl_, &det(102.0f, 202.0f), 1, out.data(), 10, 0.04f);
        sushitrack_update(hdl_, &det(104.0f, 204.0f), 1, out.data(), 10, 0.04f);
        ASSERT_EQ(out[0].state, 1) << "Precondition: track must be Tracked";
    }
};

/**
 * @brief A Lost track whose accumulated lost_time has not yet exceeded the wider
 *        buffer window remains in Lost state rather than transitioning to Removed.
 *
 * Five miss frames at dt=0.04 s yield lost_time=0.2 s, which is below the 0.4 s
 * window configured by track_buffer=10 at frame_rate=25.
 */
TEST_F(TrackerBuffersLargeBufferTest, DifferentBufferWindowAffectsExpiry)
{
    std::vector<sushitrack_track_t> out(10);
    confirmTrack(out);

    int n = 0;
    for (int i = 0; i < 5; ++i)
        n = sushitrack_update(hdl_, nullptr, 0, out.data(), 10, 0.04f);

    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 2);
    EXPECT_LT(out[0].lost_time, 0.4f);
}
