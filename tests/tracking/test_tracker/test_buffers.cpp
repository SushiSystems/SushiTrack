/**************************************************************************/
/* test_buffers.cpp                                                       */
/**************************************************************************/
/*                          This file is part of:                         */
/*                               SushiTrack                               */
/*               https://github.com/SushiSystems/SushiTrack               */
/*                         https://sushisystems.io                        */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License"); you    */
/* may not use this file except in compliance with the License. You may   */
/* obtain a copy of the License at                                        */
/*                                                                        */
/*     https://www.apache.org/licenses/LICENSE-2.0                        */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/**************************************************************************/

/**
 * @file test_buffers.cpp
 * @brief Tests the tracker's lost and removed buffers.
 * @author Mustafa Garip
 */

#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/c_api/sushitrack_c.h>

#include <c_detection_builders.hpp>

using CDetections::make_det;
using CDetections::update_frame;

namespace
{
    /** @brief Returns the last of the first @p n tracks of @p out whose x is below @p x, or null. */
    const sushitrack_track_t* track_left_of(const std::vector<sushitrack_track_t>& out, int n,
                                            float x)
    {
        const sushitrack_track_t* found = nullptr;
        for (int i = 0; i < n; ++i)
        {
            if (out[static_cast<std::size_t>(i)].x < x)
                found = &out[static_cast<std::size_t>(i)];
        }
        return found;
    }

    /** @brief Returns the last of the first @p n tracks of @p out whose x is above @p x, or null. */
    const sushitrack_track_t* track_right_of(const std::vector<sushitrack_track_t>& out, int n,
                                             float x)
    {
        const sushitrack_track_t* found = nullptr;
        for (int i = 0; i < n; ++i)
        {
            if (out[static_cast<std::size_t>(i)].x > x)
                found = &out[static_cast<std::size_t>(i)];
        }
        return found;
    }
} // namespace

/**
 * @brief Tracker fixture configured for time-aware buffer expiry testing.
 *
 * track_buffer=2, frame_rate=25 → buffer window ≈ 0.08 s (2 frames); start() builds a tracker
 * with another track_buffer.
 * tentative_confirm_hits=3       → track confirmed after three consecutive hits.
 */
class Small_TrackerBuffersTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};

        void SetUp() override { start(2); }

        void TearDown() override { sushitrack_destroy(hdl_); }

        /** @brief Replaces the tracker with a new one whose lost buffer is @p track_buffer frames. */
        void start(int track_buffer)
        {
            sushitrack_destroy(hdl_);
            sushitrack_get_default_params(&params_);
            params_.time_aware = 1;
            params_.enable_tentative = 1;
            params_.tentative_confirm_hits = 3;
            params_.tentative_max_age_seconds = 0.08f;
            params_.track_buffer = track_buffer;
            params_.frame_rate = 25;
            params_.tentative_match_thresh = 0.9f;
            params_.track_thresh = 0.1f;
            params_.high_thresh = 0.2f;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);
        }

        /**
         * @brief Drives the tracker through three matched frames to reach Tracked state.
         * @param out Output buffer (must have capacity >= 1).
         */
        void confirm_track(std::vector<sushitrack_track_t>& out)
        {
            update_frame(hdl_, make_det(100.0f, 200.0f), out.data(), 10, 0.04f);
            update_frame(hdl_, make_det(102.0f, 202.0f), out.data(), 10, 0.04f);
            update_frame(hdl_, make_det(104.0f, 204.0f), out.data(), 10, 0.04f);
            ASSERT_EQ(out[0].state, 1) << "Precondition: track must be Tracked before buffer tests";
        }

        /**
         * @brief Drives two tracks, one at x = 100 and one at x = 600, through three matched
         *        frames; the tracker holds two Tracked tracks afterwards.
         */
        void confirm_two_tracks(std::vector<sushitrack_track_t>& out)
        {
            const sushitrack_object_t detections[2] = {make_det(100.0f, 200.0f),
                                                       make_det(600.0f, 400.0f)};
            update_frame(hdl_, detections, 2, out.data(), 10, 0.04f);
            update_frame(hdl_, detections, 2, out.data(), 10, 0.04f);
            update_frame(hdl_, detections, 2, out.data(), 10, 0.04f);
        }

        /** @brief Feeds @p count frames without detections of 0.04 s and returns the last count. */
        int feed_empty_frames(std::vector<sushitrack_track_t>& out, int count)
        {
            int tracks = 0;
            for (int frame = 0; frame < count; ++frame)
                tracks = update_frame(hdl_, nullptr, 0, out.data(), 10, 0.04f);
            return tracks;
        }
};

namespace
{
    /**
     * @brief The number of missed frames after a confirmed track and the lost time they add up to.
     */
    struct MissRow
    {
            int misses;
            float lost_time;
    };
} // namespace

/**
 * @brief The fixture, run over the number of missed frames.
 */
class Small_TrackerBuffersMissTest : public Small_TrackerBuffersTest,
                                     public ::testing::WithParamInterface<MissRow>
{
};

/**
 * @brief A confirmed Tracked track is Lost after its first missed frame and after the second,
 *        and its lost time is the sum of the missed frames' intervals.
 *
 * Each missed frame is 0.04 s, so one miss is 0.04 s and two are 0.04 + 0.04 = 0.08 s, both
 * exact in float. The buffer window is 2 frames, 0.08 s, which the lost time does not pass.
 */
TEST_P(Small_TrackerBuffersMissTest, LostTimeAddsUpTheMissedFrames)
{
    std::vector<sushitrack_track_t> out(10);
    confirm_track(out);

    const int n = feed_empty_frames(out, GetParam().misses);

    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 2);
    EXPECT_FLOAT_EQ(out[0].lost_time, GetParam().lost_time);
}

INSTANTIATE_TEST_SUITE_P(MissedFrames, Small_TrackerBuffersMissTest,
                         ::testing::Values(MissRow{1, 0.04f}, MissRow{2, 0.08f}));

/**
 * @brief A Lost track moves to Removed once accumulated lost time exceeds
 *        the track buffer window.
 */
TEST_F(Small_TrackerBuffersTest, LostTrackRemovedAfterBufferExpiry)
{
    std::vector<sushitrack_track_t> out(10);
    confirm_track(out);

    feed_empty_frames(out, 2);

    int n = update_frame(hdl_, nullptr, 0, out.data(), 10, 2.0f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 3);
}

/**
 * @brief A Lost track returns to Tracked state and its lost_time resets to zero
 *        when a detection is re-associated to it within the buffer window.
 */
TEST_F(Small_TrackerBuffersTest, LostTrackRecoveredWhenReassociated)
{
    std::vector<sushitrack_track_t> out(10);
    confirm_track(out);

    feed_empty_frames(out, 1);
    ASSERT_EQ(out[0].state, 2) << "Precondition: track must be Lost before recovery test";

    sushitrack_object_t recovery = make_det(105.0f, 202.0f);
    int n = update_frame(hdl_, &recovery, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 1);
    EXPECT_FLOAT_EQ(out[0].lost_time, 0.0f);
}

/**
 * @brief A track that reaches Removed state is absent from the output of the
 *        subsequent Update call.
 */
TEST_F(Small_TrackerBuffersTest, RemovedTrackAbsentFromNextFrameOutput)
{
    std::vector<sushitrack_track_t> out(10);
    confirm_track(out);

    feed_empty_frames(out, 1);

    int n = update_frame(hdl_, nullptr, 0, out.data(), 10, 2.0f);
    ASSERT_EQ(n, 1);
    ASSERT_EQ(out[0].state, 3) << "Precondition: track must reach Removed state";

    int n2 = update_frame(hdl_, nullptr, 0, out.data(), 10, 1.0f);
    EXPECT_EQ(n2, 0);
}

/**
 * @brief When no detections arrive after two confirmed tracks, both go Lost.
 */
TEST_F(Small_TrackerBuffersTest, TwoTracksBothGoLostWhenNoDetectionArrives)
{
    std::vector<sushitrack_track_t> out(10);
    confirm_two_tracks(out);

    const int n = feed_empty_frames(out, 1);

    ASSERT_EQ(n, 2);
    ASSERT_NE(track_left_of(out, n, 300.0f), nullptr);
    ASSERT_NE(track_right_of(out, n, 300.0f), nullptr);
    EXPECT_EQ(track_left_of(out, n, 300.0f)->state, 2);
    EXPECT_EQ(track_right_of(out, n, 300.0f)->state, 2);
}

/**
 * @brief After both tracks went Lost, presenting only the first track's detection returns that
 *        track to Tracked while the second stays Lost.
 */
TEST_F(Small_TrackerBuffersTest, OnlyThePresentedLostTrackRecovers)
{
    std::vector<sushitrack_track_t> out(10);
    confirm_two_tracks(out);
    feed_empty_frames(out, 1);
    const sushitrack_object_t single = make_det(100.0f, 200.0f);

    const int n = update_frame(hdl_, &single, 1, out.data(), 10, 0.04f);

    ASSERT_EQ(n, 2);
    ASSERT_NE(track_left_of(out, n, 300.0f), nullptr);
    ASSERT_NE(track_right_of(out, n, 300.0f), nullptr);
    EXPECT_EQ(track_left_of(out, n, 300.0f)->state, 1);
    EXPECT_EQ(track_right_of(out, n, 300.0f)->state, 2);
}

namespace
{
    /**
     * @brief A lost buffer, the missed frames of 0.04 s that follow a confirmed track, and the
     *        state the track reports after them.
     */
    struct WindowRow
    {
            int track_buffer;
            int misses;
            int state;
    };
} // namespace

/**
 * @brief The fixture, run over the lost buffer and the number of missed frames.
 */
class Small_TrackerBufferWindowTest : public Small_TrackerBuffersTest,
                                      public ::testing::WithParamInterface<WindowRow>
{
};

/**
 * @brief A Lost track is Removed once its lost time passes track_buffer / frame_rate seconds.
 *
 * At 25 fps a buffer of 2 frames is 0.08 s: 2 misses are 0.08 s, which does not pass it (Lost),
 * and 3 misses are 0.12 s, which does (Removed). A buffer of 10 frames is 0.4 s: 5 misses are
 * 0.2 s (Lost) and 11 misses are 0.44 s (Removed).
 */
TEST_P(Small_TrackerBufferWindowTest, LostTrackIsRemovedOncePastTheWindow)
{
    start(GetParam().track_buffer);
    std::vector<sushitrack_track_t> out(10);
    confirm_track(out);

    const int n = feed_empty_frames(out, GetParam().misses);

    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, GetParam().state);
}

INSTANTIATE_TEST_SUITE_P(Windows, Small_TrackerBufferWindowTest,
                         ::testing::Values(WindowRow{2, 2, 2}, WindowRow{2, 3, 3},
                                           WindowRow{10, 5, 2}, WindowRow{10, 11, 3}));
