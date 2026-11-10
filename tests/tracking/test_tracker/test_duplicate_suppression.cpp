/**************************************************************************/
/* test_duplicate_suppression.cpp                                         */
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
 * @file test_duplicate_suppression.cpp
 * @brief Tests the tracker's duplicate suppression.
 * @author Mustafa Garip
 */

#include <algorithm>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/c_api/sushitrack_c.h>

#include <c_detection_builders.hpp>
#include <captured_log.hpp>

using namespace CDetections;

namespace
{
    /**
     * @brief IoU of two TLWH output tracks (inclusive of the +1 convention is
     *        unnecessary here — a coarse overlap measure suffices for the invariant).
     */
    float track_io_u(const sushitrack_track_t& a, const sushitrack_track_t& b)
    {
        const float ax2 = a.x + a.width, ay2 = a.y + a.height;
        const float bx2 = b.x + b.width, by2 = b.y + b.height;
        const float ix1 = std::max(a.x, b.x), iy1 = std::max(a.y, b.y);
        const float ix2 = std::min(ax2, bx2), iy2 = std::min(ay2, by2);
        const float iw = std::max(0.0f, ix2 - ix1), ih = std::max(0.0f, iy2 - iy1);
        const float inter = iw * ih;
        const float uni = a.width * a.height + b.width * b.height - inter;
        return uni > 0.0f ? inter / uni : 0.0f;
    }

    /**
     * @brief Returns whether two Tracked tracks among the first @p n of @p out overlap above 0.85.
     */
    bool has_overlapping_tracked_pair(const std::vector<sushitrack_track_t>& out, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            for (int j = i + 1; j < n; ++j)
            {
                const sushitrack_track_t& a = out[static_cast<std::size_t>(i)];
                const sushitrack_track_t& b = out[static_cast<std::size_t>(j)];
                if (a.state == K_TRACKED && b.state == K_TRACKED && track_io_u(a, b) > 0.85f)
                    return true;
            }
        }
        return false;
    }

    /**
     * @brief What feed_singles() saw: the Tracked count of the last frame and how many frames
     *        held two overlapping Tracked tracks.
     */
    struct SingleFrames
    {
            int final_tracked = 0;
            int overlapping_frames = 0;
    };

    /**
     * @brief Suppression fixture: core path, tentative off so an unmatched
     *        high-confidence detection seeds a track immediately.
     */
    class Small_TrackerDuplicateSuppressionBase : public ::testing::Test
    {
        protected:
            sushitrack_tracker_t hdl_ = nullptr;
            sushitrack_params_t params_ = {};

            void SetUp() override
            {
                sushitrack_get_default_params(&params_);
                params_.time_aware = 0;
                params_.enable_tentative = 0;
                params_.primary_assoc_mode = 1;
                params_.frame_rate = 25;
                params_.track_buffer = 30;
                params_.track_thresh = 0.2f;
                params_.high_thresh = 0.5f;
                params_.match_thresh = 0.9f;
                params_.duplicate_iou_thresh = 0.15f; // suppress when IoU > 0.85

                hdl_ = sushitrack_create(&params_);
                ASSERT_NE(hdl_, nullptr);
            }

            void TearDown() override
            {
                if (hdl_)
                    sushitrack_destroy(hdl_);
            }

            /**
             * @brief Feeds a single detection at (@p x, @p y) for @p frames frames of 0.04 s.
             */
            SingleFrames feed_singles(std::vector<sushitrack_track_t>& out, float x, float y,
                                      int frames)
            {
                SingleFrames seen;
                for (int frame = 0; frame < frames; ++frame)
                {
                    const sushitrack_object_t detection = make_det(x, y);
                    const int n = update_frame(hdl_, &detection, 1, out.data(), 8, 0.04f);
                    seen.final_tracked = count_tracked(out, n);
                    if (has_overlapping_tracked_pair(out, n))
                        ++seen.overlapping_frames;
                }
                return seen;
            }

            /**
             * @brief Feeds two detections, one at (@p x, @p y) and one 3 px to its right (IoU
             *        above 0.85), for one frame of 0.04 s and returns the track count.
             */
            int feed_twins(std::vector<sushitrack_track_t>& out, float x, float y)
            {
                const sushitrack_object_t twins[2] = {make_det(x, y), make_det(x + 3.0f, y)};
                return update_frame(hdl_, twins, 2, out.data(), 8, 0.04f);
            }
    };

    /**
     * @brief The place of a box, the frames it is fed alone before its twin appears and the
     *        frames after.
     */
    struct TwinRow
    {
            float x;
            float y;
            int settle_frames;
            int tail_frames;
    };
} // namespace

/**
 * @brief The fixture, run over a twin that appears on an established track and a twin present
 *        from the first frame.
 */
class Small_TrackerDuplicateSuppressionTest : public Small_TrackerDuplicateSuppressionBase,
                                              public ::testing::WithParamInterface<TwinRow>
{
};

/**
 * @brief A near-coincident second detection (3 px away, IoU above 0.85) does not leave two
 *        identities for one object: the scene converges to one Tracked track and no frame
 *        of the single-box phases holds two overlapping Tracked tracks.
 */
TEST_P(Small_TrackerDuplicateSuppressionTest, DuplicateDetectionsCollapseToOneIdentity)
{
    std::vector<sushitrack_track_t> out(8);
    const TwinRow row = GetParam();
    const SingleFrames settle = feed_singles(out, row.x, row.y, row.settle_frames);
    ASSERT_GE(feed_twins(out, row.x, row.y), 1);

    const SingleFrames tail = feed_singles(out, row.x, row.y, row.tail_frames);

    EXPECT_EQ(settle.overlapping_frames + tail.overlapping_frames, 0);
    EXPECT_EQ(tail.final_tracked, 1) << "the twin left a lasting second identity";
}

INSTANTIATE_TEST_SUITE_P(
    Twins, Small_TrackerDuplicateSuppressionTest,
    ::testing::Values(
        // a track settled over 6 frames meets a twin for one frame, then 4 single frames
        TwinRow{100.0f, 200.0f, 6, 4},
        // the twin is there from the first frame, then 6 single frames
        TwinRow{250.0f, 250.0f, 0, 6}));

/**
 * @brief A duplicate drop writes one DEBUG naming the dropped track and the kept one.
 *
 * Two coincident detections on the first frame confirm two tracks at once; the twin that
 * misses the next frame turns Lost and is dropped as a duplicate of the Tracked one. Which of
 * the two is dropped is not fixed, so either order of the two ids passes.
 */
class Small_TrackerDuplicateLogTest : public Small_TrackerDuplicateSuppressionBase
{
};

TEST_F(Small_TrackerDuplicateLogTest, DuplicateDropLogsBothTrackIds)
{
    SushiTrack::CapturedLog log(SushiLog::LogLevel::Debug);
    std::vector<sushitrack_track_t> out(8);
    ASSERT_EQ(feed_twins(out, 100.0f, 200.0f), 2);
    const std::string first = std::to_string(out[0].track_id);
    const std::string second = std::to_string(out[1].track_id);

    feed_singles(out, 100.0f, 200.0f, 4);

    const std::string dropped = " dropped as a duplicate of track ";
    EXPECT_GE(log.count("track " + first + dropped + second) +
                  log.count("track " + second + dropped + first),
              1);
    EXPECT_EQ(log.level_of("dropped as a duplicate of track"), SushiLog::LogLevel::Debug);
}
