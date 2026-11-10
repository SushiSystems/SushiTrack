/**************************************************************************/
/* test_association_stages.cpp                                            */
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
 * @file test_association_stages.cpp
 * @brief Tests the tracker's association stages.
 * @author Mustafa Garip
 */

#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/c_api/sushitrack_c.h>

#include <c_detection_builders.hpp>

using CDetections::make_det;
using CDetections::update_frame;

/**
 * @brief Fixture for match_high_confidence tests over primary_assoc_mode: 1 is LAPJV and 0 is the
 *        age cascade.
 *
 * enable_tentative=0 so new tracks are immediately Tracked after one hit.
 * track_thresh=0.1, high_thresh=0.2 ensure all test detections fall in the
 * high-confidence pool.
 */
class Small_TrackerHighConfidenceTest : public ::testing::TestWithParam<int>
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 1;
            params_.enable_tentative = 0;
            params_.primary_assoc_mode = GetParam();
            params_.track_buffer = 5;
            params_.frame_rate = 25;
            params_.track_thresh = 0.1f;
            params_.high_thresh = 0.2f;
            params_.match_thresh = 0.9f;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override { sushitrack_destroy(hdl_); }
};

/**
 * @brief match_high_confidence sustains a Tracked track across two consecutive frames when a
 *        spatially close detection is presented, in LAPJV mode and in age-cascade mode.
 */
TEST_P(Small_TrackerHighConfidenceTest, AssociatesCloseDetectionToTrackedTrack)
{
    std::vector<sushitrack_track_t> out(10);
    const sushitrack_object_t d0 = make_det(100.0f, 200.0f);
    update_frame(hdl_, &d0, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(out[0].state, 1);
    const int64_t id0 = out[0].track_id;

    const sushitrack_object_t d1 = make_det(102.0f, 202.0f);
    const int n = update_frame(hdl_, &d1, 1, out.data(), 10, 0.04f);

    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].track_id, id0);
    EXPECT_EQ(out[0].state, 1);
}

INSTANTIATE_TEST_SUITE_P(PrimaryAssocModes, Small_TrackerHighConfidenceTest,
                         ::testing::Values(1, 0));

/**
 * @brief Fixture for tentative confirmation-tier tests.
 *
 * tentative_fast_thresh=0.85, tentative_fast_hits=1: a score of 0.85 or more confirms
 * in a single hit.
 * tentative_confirm_hits=3: scores below tentative_fast_thresh need three hits.
 */
class Small_TrackerTentativeTiersTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 1;
            params_.enable_tentative = 1;
            params_.tentative_confirm_hits = 3;
            params_.tentative_fast_thresh = 0.85f;
            params_.tentative_fast_hits = 1;
            params_.tentative_slow_thresh = 0.40f;
            params_.tentative_slow_hits_offset = 1;
            params_.tentative_max_age_seconds = 2.0f;
            params_.track_buffer = 5;
            params_.frame_rate = 25;
            params_.tentative_match_thresh = 0.9f;
            params_.track_thresh = 0.1f;
            params_.high_thresh = 0.2f;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override { sushitrack_destroy(hdl_); }

        /**
         * @brief Feeds @p frames frames of one box with score 0.5 that drifts 2 px right and down
         *        per frame, and returns the track count of the last.
         */
        int feed_standard_score_frames(std::vector<sushitrack_track_t>& out, int frames)
        {
            int count = 0;
            for (int frame = 0; frame < frames; ++frame)
            {
                const float drift = 2.0f * static_cast<float>(frame);
                const sushitrack_object_t detection = make_det(100.0f + drift, 200.0f + drift, 0.5f);
                count = update_frame(hdl_, &detection, 1, out.data(), 10, 0.04f);
            }
            return count;
        }
};

/**
 * @brief A high-score detection (score >= tentative_fast_thresh) promotes to
 *        Tracked on the fast tier after a single confirming match.
 *
 * The first frame creates a Tentative track (state=4, is_confirmed=0); the second frame's
 * match promotes it (state=1, is_confirmed=1).
 */
TEST_F(Small_TrackerTentativeTiersTest, HighScoreDetectionPromotesAfterOneConfirmingHit)
{
    std::vector<sushitrack_track_t> out(10);

    // Frame 1: the high-score detection creates a Tentative track.
    sushitrack_object_t d0 = make_det(100.0f, 200.0f, 0.9f);
    int n = update_frame(hdl_, &d0, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 4);
    EXPECT_EQ(out[0].is_confirmed, 0);

    // Frame 2: the confirming match promotes the tentative to Tracked under the fast tier (tentative_fast_hits=1, score still above fast_thresh).
    sushitrack_object_t d1 = make_det(102.0f, 202.0f, 0.9f);
    n = update_frame(hdl_, &d1, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 1);
    EXPECT_EQ(out[0].is_confirmed, 1);
}

/**
 * @brief A detection with score in (high_thresh, tentative_fast_thresh) is still Tentative
 *        after two matches: it needs tentative_confirm_hits = 3.
 */
TEST_F(Small_TrackerTentativeTiersTest, StandardScoreDetectionIsNotPromotedBeforeTheThirdHit)
{
    std::vector<sushitrack_track_t> out(10);

    const int n = feed_standard_score_frames(out, 2);

    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 4);
}

/**
 * @brief A detection with score in (high_thresh, tentative_fast_thresh) is promoted to Tracked
 *        and confirmed at its third hit.
 */
TEST_F(Small_TrackerTentativeTiersTest, StandardScoreDetectionIsPromotedAtTheThirdHit)
{
    std::vector<sushitrack_track_t> out(10);

    const int n = feed_standard_score_frames(out, 3);

    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 1);
    EXPECT_EQ(out[0].is_confirmed, 1);
}

/**
 * @brief Fixture for the empty-frame test.
 */
class Small_TrackerZeroDetectionTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 1;
            params_.enable_tentative = 0;
            params_.primary_assoc_mode = 1;
            params_.track_buffer = 5;
            params_.frame_rate = 25;
            params_.track_thresh = 0.1f;
            params_.high_thresh = 0.2f;
            params_.match_thresh = 0.9f;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override { sushitrack_destroy(hdl_); }
};

/**
 * @brief A zero-detection frame on an empty tracker returns zero tracks without error.
 */
TEST_F(Small_TrackerZeroDetectionTest, ZeroDetectionsOnEmptyTrackerReturnsZero)
{
    std::vector<sushitrack_track_t> out(10);
    int n = update_frame(hdl_, nullptr, 0, out.data(), 10, 0.04f);
    EXPECT_EQ(n, 0);
}
