/**************************************************************************/
/* test_advanced_path.cpp                                                 */
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
 * @file test_advanced_path.cpp
 * @brief Tests the tracker on a path that turns and stops.
 * @author Mustafa Garip
 */

#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/c_api/sushitrack_c.h>

#include <c_detection_builders.hpp>

using CDetections::find_by_id;
using CDetections::make_det_with_feat;
using CDetections::update_frame;

namespace
{
    /**
     * @brief Feeds a box with @p feature down the first three frames, 5 px per frame, and
     *        returns the id of its track after the third, or -1 when a frame gave no track.
     */
    int64_t confirm_target(sushitrack_tracker_t tracker, const std::vector<float>& feature)
    {
        std::vector<sushitrack_track_t> out(8);
        int64_t track_id = -1;
        for (int frame = 0; frame < 3; ++frame)
        {
            const sushitrack_object_t detection =
                make_det_with_feat(100.0f, 100.0f + frame * 5.0f, feature.data(), 512, 0.9f);
            if (update_frame(tracker, &detection, 1, out.data(), 8, 0.04f) < 1)
                return -1;
            track_id = out[0].track_id;
        }
        return track_id;
    }
} // namespace

/**
 * @brief Fixture for the advanced path: ReID and Mahalanobis gating on, the age cascade off.
 */
class Small_TrackerAdvancedPathTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};
        std::vector<float> target_feat_;
        std::vector<float> decoy_feat_;

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.enable_reid = 1;
            params_.enable_mahalanobis = 1;
            params_.primary_assoc_mode = 1;
            params_.time_aware = 1;
            params_.match_thresh = 0.9f;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);

            target_feat_.assign(512, 0.1f);
            target_feat_[0] = 1.0f;

            decoy_feat_.assign(512, 0.5f);
            decoy_feat_[0] = -1.0f;
        }

        void TearDown() override { sushitrack_destroy(hdl_); }
};

/**
 * @brief With ReID and the Mahalanobis gate, a decoy at the predicted place does not take the
 *        id of a confirmed target that reappears 10 px away with its own feature.
 *
 * The Kalman output blends prediction and measurement, so the track lands between x = 100 (the
 * decoy) and x = 110 (the target); a track that followed the decoy stays near 100, one that
 * followed the target is above 105.
 */
TEST_F(Small_TrackerAdvancedPathTest, ReIDAndMahalanobisEndToEnd)
{
    std::vector<sushitrack_track_t> out(8);
    const int64_t target_id = confirm_target(hdl_, target_feat_);
    ASSERT_NE(target_id, -1);
    const sushitrack_object_t decoy = make_det_with_feat(100.0f, 115.0f, decoy_feat_.data(), 512);
    const sushitrack_object_t target = make_det_with_feat(110.0f, 115.0f, target_feat_.data(), 512);
    const sushitrack_object_t detections[2] = {decoy, target};

    const int n = update_frame(hdl_, detections, 2, out.data(), 8, 0.04f);

    ASSERT_GE(n, 1);
    const sushitrack_track_t* kept = find_by_id(out, n, target_id);
    ASSERT_NE(kept, nullptr) << "the target lost its id";
    EXPECT_GT(kept->x, 105.0f) << "the id followed the decoy";
}
