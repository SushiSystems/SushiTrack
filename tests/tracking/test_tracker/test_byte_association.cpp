/**************************************************************************/
/* test_byte_association.cpp                                              */
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
 * @file test_byte_association.cpp
 * @brief Tests the tracker's two-stage ByteTrack association.
 * @author Mustafa Garip
 */

#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/c_api/sushitrack_c.h>

#include <c_detection_builders.hpp>

using namespace CDetections;

/**
 * @brief Tracker fixture for the two-stage association: scores below 0.5 are the low pool, scores
 *        from 0.5 to 0.6 the high pool that may only continue a track, and 0.6 and above may
 *        also seed one.
 */
class Small_TrackerByteAssociationTest : public ::testing::Test
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
            // High/low split at 0.5; new tracks require >= 0.6.
            params_.track_thresh = 0.5f;
            params_.high_thresh = 0.6f;
            params_.match_thresh = 0.9f;
            params_.iou_match_thresh = 0.5f;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override
        {
            if (hdl_)
                sushitrack_destroy(hdl_);
        }

        /**
         * @brief Confirms a track on four high-score frames moving 4 px per frame, then follows
         *        it for three frames of score 0.35 (the low pool); returns its id.
         */
        int64_t sustain_with_low_scores(std::vector<sushitrack_track_t>& out, int& tracks)
        {
            int64_t id = -1;
            for (int i = 0; i < 4; ++i)
            {
                const sushitrack_object_t high = make_det(100.0f + 4.0f * i, 200.0f, 0.9f);
                if (update_frame(hdl_, &high, 1, out.data(), 8, 0.04f) < 1)
                    return -1;
                id = out[0].track_id;
            }
            for (int i = 0; i < 3; ++i)
            {
                const sushitrack_object_t low = make_det(116.0f + 4.0f * i, 200.0f, 0.35f);
                tracks = update_frame(hdl_, &low, 1, out.data(), 8, 0.04f);
            }
            return id;
        }

        /** @brief Feeds @p detection for @p frames frames and returns the last frame's count. */
        int feed_same_box(const sushitrack_object_t& detection, int frames,
                          std::vector<sushitrack_track_t>& out)
        {
            int tracks = 0;
            for (int frame = 0; frame < frames; ++frame)
                tracks = update_frame(hdl_, &detection, 1, out.data(), 8, 0.04f);
            return tracks;
        }
};

/**
 * @brief A confirmed track survives three frames of low-confidence detections on its path, via
 *        the secondary association stage, under its own id and still Tracked.
 */
TEST_F(Small_TrackerByteAssociationTest, LowConfidenceDetectionsSustainTheTrack)
{
    std::vector<sushitrack_track_t> out(8);
    int tracks = 0;
    const int64_t id = sustain_with_low_scores(out, tracks);
    ASSERT_NE(id, -1);

    const sushitrack_track_t* kept = find_by_id(out, tracks, id);

    ASSERT_NE(kept, nullptr) << "the track was dropped in the low-confidence pool";
    EXPECT_EQ(kept->state, K_TRACKED);
}

/**
 * @brief Low-confidence detections that sustain a track do not seed a second one.
 */
TEST_F(Small_TrackerByteAssociationTest, LowConfidenceDetectionsSeedNoSecondTrack)
{
    std::vector<sushitrack_track_t> out(8);
    int tracks = 0;
    ASSERT_NE(sustain_with_low_scores(out, tracks), -1);

    EXPECT_EQ(count_tracked(out, tracks), 1);
}

/**
 * @brief When the score recovers, the object returns to the high pool under the same id.
 */
TEST_F(Small_TrackerByteAssociationTest, TheTrackKeepsItsIdWhenTheScoreRecovers)
{
    std::vector<sushitrack_track_t> out(8);
    int tracks = 0;
    const int64_t id = sustain_with_low_scores(out, tracks);
    ASSERT_NE(id, -1);
    const sushitrack_object_t recovered = make_det(130.0f, 200.0f, 0.9f);

    const int n = update_frame(hdl_, &recovered, 1, out.data(), 8, 0.04f);

    const sushitrack_track_t* kept = find_by_id(out, n, id);
    ASSERT_NE(kept, nullptr) << "the track lost its id on return to high confidence";
    EXPECT_EQ(kept->state, K_TRACKED);
}

/**
 * @brief The fixture, run over the scores that may not seed a track.
 */
class Small_TrackerByteAssociationSeedTest : public Small_TrackerByteAssociationTest,
                                             public ::testing::WithParamInterface<float>
{
};

/**
 * @brief Four frames of a detection scoring below high_thresh, with no track yet, create no
 *        track: 0.3 is in the low pool, 0.55 is in the high pool but under the seeding bar.
 */
TEST_P(Small_TrackerByteAssociationSeedTest, ScoreBelowHighThreshDoesNotSeedATrack)
{
    std::vector<sushitrack_track_t> out(8);
    const sushitrack_object_t detection = make_det(300.0f, 300.0f, GetParam());

    const int tracks = feed_same_box(detection, 4, out);

    EXPECT_EQ(count_tracked(out, tracks), 0);
}

INSTANTIATE_TEST_SUITE_P(Scores, Small_TrackerByteAssociationSeedTest,
                         ::testing::Values(0.3f, 0.55f));

/**
 * @brief A detection above high_thresh seeds a track.
 */
TEST_F(Small_TrackerByteAssociationTest, HighConfidenceDetectionSeedsATrack)
{
    std::vector<sushitrack_track_t> out(8);
    const sushitrack_object_t detection = make_det(300.0f, 300.0f, 0.9f);

    const int n = update_frame(hdl_, &detection, 1, out.data(), 8, 0.04f);

    EXPECT_EQ(count_tracked(out, n), 1);
}
