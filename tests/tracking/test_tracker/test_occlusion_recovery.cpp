/**************************************************************************/
/* test_occlusion_recovery.cpp                                            */
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
 * @file test_occlusion_recovery.cpp
 * @brief Tests the tracker's recovery after occlusion.
 * @author Mustafa Garip
 */

#include <cstddef>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/c_api/sushitrack_c.h>

#include <c_detection_builders.hpp>

using namespace CDetections;

namespace
{
    /** @brief The id of a confirmed moving track and where its last detection was. */
    struct ConfirmedTrack
    {
            int64_t id = -1;
            float x = 0.0f;
            float y = 0.0f;
    };

    /** @brief The number of frames an object is hidden. */
    struct GapRow
    {
            int frames;
    };

    /** @brief Lane split of the two-lane scene: the top object runs above this y. */
    constexpr float K_LANE_SPLIT = 250.0f;
} // namespace

/**
 * @brief Core-path fixture: frame-based timing, no advanced features.
 *
 * The track buffer is set per test, so that the within-buffer and beyond-buffer cases differ
 * only in the length of the occlusion gap.
 */
class Small_TrackerOcclusionRecoveryTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};
        std::vector<sushitrack_track_t> out_ = std::vector<sushitrack_track_t>(8);

        void configure(int track_buffer)
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 0;
            params_.enable_tentative = 0;
            params_.primary_assoc_mode = 1;
            params_.frame_rate = 25;
            params_.track_buffer = track_buffer;
            params_.track_thresh = 0.2f;
            params_.high_thresh = 0.5f;
            params_.match_thresh = 0.9f;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override
        {
            if (hdl_)
                sushitrack_destroy(hdl_);
        }

        /**
         * @brief Feeds @p frames frames of a box moving (@p vx, 0) px per frame from
         *        (@p x0, @p y0) and returns the id of the track and the last position.
         */
        ConfirmedTrack confirm_moving_track(float x0, float y0, float vx, int frames)
        {
            ConfirmedTrack track;
            for (int i = 0; i < frames; ++i)
            {
                track.x = x0 + vx * static_cast<float>(i);
                track.y = y0;
                const sushitrack_object_t detection = make_det(track.x, track.y);
                const int n = update_frame(hdl_, detection, out_.data(), 8, 0.04f);
                EXPECT_GE(n, 1);
                if (n >= 1)
                    track.id = out_[0].track_id;
            }
            return track;
        }

        /**
         * @brief Feeds @p frames empty frames of 0.04 s.
         */
        void hide(int frames)
        {
            for (int i = 0; i < frames; ++i)
                update_frame(hdl_, nullptr, 0, out_.data(), 8, 0.04f);
        }
};

/**
 * @brief The occlusion fixture, run over gaps well inside the buffer.
 */
class Small_TrackerShortOcclusionTest : public Small_TrackerOcclusionRecoveryTest,
                                        public ::testing::WithParamInterface<GapRow>
{
};

/**
 * @brief An occlusion well inside the buffer of 30 frames is bridged: the object is re-acquired
 *        under its original id when it reappears on its extrapolated trajectory.
 *
 * The Kalman prediction keeps the position velocity of a Lost track, so the predicted box stays
 * on the trajectory and the reappearing detection, placed gap + 1 steps of 6 px on, overlaps it.
 */
TEST_P(Small_TrackerShortOcclusionTest, ObjectIsRecoveredUnderItsOriginalId)
{
    configure(30);
    const ConfirmedTrack track = confirm_moving_track(100.0f, 200.0f, 6.0f, 6);
    ASSERT_NE(track.id, -1);
    const int gap = GetParam().frames;
    hide(gap);

    const sushitrack_object_t back =
        make_det(track.x + 6.0f * static_cast<float>(gap + 1), track.y);
    const int n = update_frame(hdl_, back, out_.data(), 8, 0.04f);

    const sushitrack_track_t* recovered = find_by_id(out_, n, track.id);
    ASSERT_NE(recovered, nullptr)
        << "Object lost its identity across a short occlusion within the buffer.";
    EXPECT_EQ(recovered->state, K_TRACKED);
    EXPECT_TRUE(geometry_is_sane(*recovered));
}

INSTANTIATE_TEST_SUITE_P(Gaps, Small_TrackerShortOcclusionTest,
                         ::testing::Values(GapRow{1}, GapRow{3}, GapRow{10}));

/**
 * @brief An object that reappears after the buffer has retired its track gets a brand-new id,
 *        and the stale id is gone.
 *
 * With a buffer of 5 frames, a gap of 20 removes the lost track in cleanup_lost; the retire
 * itself is checked in test_buffers.cpp. The detection at x 600 matches nothing, so it seeds a
 * track of its own, the only one in the output.
 */
TEST_F(Small_TrackerOcclusionRecoveryTest, LongOcclusionReassignsANewId)
{
    configure(5);
    const ConfirmedTrack track = confirm_moving_track(100.0f, 200.0f, 5.0f, 6);
    ASSERT_NE(track.id, -1);
    hide(20);

    const sushitrack_object_t back = make_det(600.0f, 200.0f);
    const int n = update_frame(hdl_, back, out_.data(), 8, 0.04f);

    ASSERT_EQ(n, 1) << "The reappeared object produced no fresh track, or the stale one stayed.";
    EXPECT_NE(out_[0].track_id, track.id);
    EXPECT_TRUE(geometry_is_sane(out_[0]));
}

/**
 * @brief Fixture of two objects in lanes 300 px apart (y 100 and y 400), both moving right at
 *        5 px per frame, seeded over six frames with a buffer of 30.
 */
class Small_TrackerTwoLaneOcclusionTest : public Small_TrackerOcclusionRecoveryTest
{
    protected:
        int64_t id_top_ = -1;
        int64_t id_bottom_ = -1;

        void SetUp() override
        {
            configure(30);
            ASSERT_NE(hdl_, nullptr);
            seed_lanes();
        }

        /**
         * @brief Feeds both objects at frame @p frame and returns the track count.
         */
        int feed_both(int frame)
        {
            const sushitrack_object_t pair[2] = {
                make_det(100.0f + 5.0f * static_cast<float>(frame), 100.0f),
                make_det(100.0f + 5.0f * static_cast<float>(frame), 400.0f)};
            return update_frame(hdl_, pair, 2, out_.data(), 8, 0.04f);
        }

        /**
         * @brief Feeds the bottom object alone at frame @p frame and returns the track count.
         */
        int feed_bottom(int frame)
        {
            const sushitrack_object_t bottom =
                make_det(100.0f + 5.0f * static_cast<float>(frame), 400.0f);
            return update_frame(hdl_, bottom, out_.data(), 8, 0.04f);
        }

        /**
         * @brief Feeds the bottom object alone for frames 6 and 7 and returns how many of them
         *        did not show it as a Tracked track under its id.
         */
        int hide_top_for_two_frames()
        {
            int frames_without_bottom = 0;
            for (int frame = 6; frame < 8; ++frame)
            {
                const int n = feed_bottom(frame);
                const sushitrack_track_t* bottom = find_by_id(out_, n, id_bottom_);
                if (bottom == nullptr || bottom->state != K_TRACKED)
                    ++frames_without_bottom;
            }
            return frames_without_bottom;
        }

    private:
        void seed_lanes()
        {
            for (int frame = 0; frame < 6; ++frame)
            {
                const int n = feed_both(frame);
                ASSERT_GE(n, 2);
                for (int k = 0; k < n; ++k)
                {
                    const sushitrack_track_t& track = out_[static_cast<std::size_t>(k)];
                    (track.y < K_LANE_SPLIT ? id_top_ : id_bottom_) = track.track_id;
                }
            }
            ASSERT_NE(id_top_, -1);
            ASSERT_NE(id_bottom_, -1);
            ASSERT_NE(id_top_, id_bottom_);
        }
};

/**
 * @brief The visible object keeps its id and its Tracked state while its neighbour is hidden
 *        for two frames.
 */
TEST_F(Small_TrackerTwoLaneOcclusionTest, VisibleObjectKeepsItsIdWhileTheOtherIsHidden)
{
    EXPECT_EQ(hide_top_for_two_frames(), 0);
}

/**
 * @brief The hidden object recovers its own id, in its own lane, and the visible one keeps
 *        its lane: neither id moves onto the other detection.
 */
TEST_F(Small_TrackerTwoLaneOcclusionTest, HiddenObjectRecoversItsOwnIdInItsOwnLane)
{
    ASSERT_EQ(hide_top_for_two_frames(), 0);

    const int n = feed_both(8);

    ASSERT_GE(n, 2);
    const sushitrack_track_t* top = find_by_id(out_, n, id_top_);
    const sushitrack_track_t* bottom = find_by_id(out_, n, id_bottom_);
    ASSERT_NE(bottom, nullptr) << "The continuously visible object lost its identity.";
    ASSERT_NE(top, nullptr) << "The occluded object failed to recover its identity.";
    EXPECT_LT(top->y, K_LANE_SPLIT) << "The recovered identity migrated into the wrong lane.";
    EXPECT_GT(bottom->y, K_LANE_SPLIT);
}
