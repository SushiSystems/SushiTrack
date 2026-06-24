/**
 * @file      TestTrackerBuffers.cpp
 * @brief     Tracker-level tests for Tracked/Lost/Removed lifecycle and multi-track independence.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      12.05.2026
 * @version   v1.1.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 *
 * Coverage
 * --------
 * TrackerBuffersTest (track_buffer=2, frame_rate=25, window=0.08 s)
 *   - Tracked track transitions to Lost on the first missed frame.
 *   - Lost time accumulates monotonically across consecutive missed frames.
 *   - Lost track transitions to Removed once accumulated time exceeds the window.
 *   - Lost track is recovered to Tracked when a matching detection arrives.
 *   - Removed track is absent from the subsequent Update output.
 *   - Two tracks at separated positions confirm independently with distinct IDs.
 *   - Two confirmed tracks go Lost independently; only the re-observed one recovers.
 *
 * TrackerBuffersLargeBufferTest (track_buffer=10, frame_rate=25, window=0.4 s)
 *   - Five miss frames (0.2 s) keep the track Lost, not Removed, under a wider window.
 *
 * Tracking state integers map to SushiTrack::TrackState:
 *   Tracked=1, Lost=2, Removed=3, Tentative=4
 */

#include <gtest/gtest.h>
#include "SushiTrackC.h"
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
    EXPECT_EQ(out[0].state, 2);
    EXPECT_FLOAT_EQ(out[0].lost_time, 0.04f);
}

/**
 * @brief Lost time accumulates monotonically across consecutive missed frames.
 */
TEST_F(TrackerBuffersTest, LostTimeAccumulatesAcrossMissedFrames)
{
    std::vector<SushiTrack_Track> out(10);
    confirmTrack(out);

    SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 0.04f);
    int n = SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 0.04f);

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
    std::vector<SushiTrack_Track> out(10);
    confirmTrack(out);

    SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 0.04f);
    SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 0.04f);

    int n = SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 2.0f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 3);
}

/**
 * @brief A Lost track returns to Tracked state and its lost_time resets to zero
 *        when a detection is re-associated to it within the buffer window.
 */
TEST_F(TrackerBuffersTest, LostTrackRecoveredWhenReassociated)
{
    std::vector<SushiTrack_Track> out(10);
    confirmTrack(out);

    SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 0.04f);
    ASSERT_EQ(out[0].state, 2) << "Precondition: track must be Lost before recovery test";

    SushiTrack_Object recovery = det(105.0f, 202.0f);
    int n = SushiTrack_Update(hdl_, &recovery, 1, out.data(), 10, 0.04f);
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
    std::vector<SushiTrack_Track> out(10);
    confirmTrack(out);

    SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 0.04f);

    int n = SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 2.0f);
    ASSERT_EQ(n, 1);
    ASSERT_EQ(out[0].state, 3) << "Precondition: track must reach Removed state";

    int n2 = SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 1.0f);
    EXPECT_EQ(n2, 0);
}

/**
 * @brief Two detections at well-separated positions both reach Tracked state after
 *        three consecutive matched frames and are assigned distinct non-zero IDs.
 */
TEST_F(TrackerBuffersTest, MultipleTracksRetainIndependentIds)
{
    std::vector<SushiTrack_Track> out(10);

    SushiTrack_Object dets[2];
    dets[0] = det(100.0f, 200.0f);
    dets[1] = det(600.0f, 400.0f);

    SushiTrack_Update(hdl_, dets, 2, out.data(), 10, 0.04f);
    SushiTrack_Update(hdl_, dets, 2, out.data(), 10, 0.04f);
    int n = SushiTrack_Update(hdl_, dets, 2, out.data(), 10, 0.04f);

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
    std::vector<SushiTrack_Track> out(10);

    SushiTrack_Object dets[2];
    dets[0] = det(100.0f, 200.0f);
    dets[1] = det(600.0f, 400.0f);

    SushiTrack_Update(hdl_, dets, 2, out.data(), 10, 0.04f);
    SushiTrack_Update(hdl_, dets, 2, out.data(), 10, 0.04f);
    SushiTrack_Update(hdl_, dets, 2, out.data(), 10, 0.04f);

    int n_lost = SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 0.04f);
    ASSERT_EQ(n_lost, 2);
    for (int i = 0; i < 2; ++i)
        EXPECT_EQ(out[i].state, 2);

    SushiTrack_Object single = det(100.0f, 200.0f);
    int n_mixed = SushiTrack_Update(hdl_, &single, 1, out.data(), 10, 0.04f);
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
    SushiTrackHdl     hdl_    = nullptr;
    SushiTrack_Params params_ = {};

    void SetUp() override
    {
        SushiTrack_GetDefaultParams(&params_);
        params_.time_aware                = 1;
        params_.enable_tentative          = 1;
        params_.tentative_confirm_hits    = 3;
        params_.tentative_max_age_seconds = 0.08f;
        params_.track_buffer              = 10;
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

    /**
     * @brief Drives three matched frames to bring the track to Tracked state.
     * @param out Output buffer (capacity >= 1).
     */
    void confirmTrack(std::vector<SushiTrack_Track>& out)
    {
        SushiTrack_Update(hdl_, &det(100.0f, 200.0f), 1, out.data(), 10, 0.04f);
        SushiTrack_Update(hdl_, &det(102.0f, 202.0f), 1, out.data(), 10, 0.04f);
        SushiTrack_Update(hdl_, &det(104.0f, 204.0f), 1, out.data(), 10, 0.04f);
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
    std::vector<SushiTrack_Track> out(10);
    confirmTrack(out);

    int n = 0;
    for (int i = 0; i < 5; ++i)
        n = SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 0.04f);

    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 2);
    EXPECT_LT(out[0].lost_time, 0.4f);
}
