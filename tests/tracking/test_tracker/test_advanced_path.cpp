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
 * @file test_advanced_path.cpp
 * @brief Tests the tracker on a path that turns and stops.
 * @author Mustafa Garip
 */

#include <cmath>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/c_api/sushitrack_c.h>

namespace
{

    sushitrack_object_t make_det_with_feat(float x, float y, const float* feat, int feat_dim,
                                           float score = 0.9f)
    {
        sushitrack_object_t o{};
        o.x = x;
        o.y = y;
        o.width = 50.0f;
        o.height = 100.0f;
        o.score = score;
        o.label = 1;
        o.feat = feat;
        o.feat_dim = feat_dim;
        return o;
    }

    const sushitrack_track_t* find_by_id(const std::vector<sushitrack_track_t>& out, int n,
                                         int64_t id)
    {
        for (int i = 0; i < n; ++i)
            if (out[static_cast<std::size_t>(i)].track_id == id)
                return &out[static_cast<std::size_t>(i)];
        return nullptr;
    }

} // namespace

/**
 * @brief Integration test validating the Advanced Path (ReID + Mahalanobis Gating).
 *
 * Closes Gap 3 from RELEASE_READINESS.md: "Mahalanobis + fused path lacks end-to-end coverage."
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

TEST_F(Small_TrackerAdvancedPathTest, ReIDAndMahalanobisEndToEnd)
{
    std::vector<sushitrack_track_t> out(8);

    // Frame 1 to 3: Detect target to confirm the tracklet
    int64_t target_id = -1;
    for (int i = 0; i < 3; ++i)
    {
        sushitrack_object_t d =
            make_det_with_feat(100.0f, 100.0f + i * 5.0f, target_feat_.data(), 512, 0.9f);
        int n = sushitrack_update(hdl_, &d, 1, out.data(), 8, 0.04f);
        ASSERT_GE(n, 1);
        if (i == 2)
        {
            target_id = out[0].track_id;
        }
    }
    ASSERT_NE(target_id, -1);

    // Frame 4: An object appears at the exact same location, but with a different feature (Decoy) And our true target object appears a bit further away (Mahalanobis gate should catch it, and ReID should match it)

    // Decoy object at the predicted location (IoU is high, but ReID feature is very different)
    sushitrack_object_t decoy = make_det_with_feat(100.0f, 115.0f, decoy_feat_.data(), 512, 0.9f);

    // True object further away (IoU is lower, but ReID feature matches perfectly) Placed at 110, so it's still well within the Mahalanobis gate.
    sushitrack_object_t target = make_det_with_feat(110.0f, 115.0f, target_feat_.data(), 512, 0.9f);

    sushitrack_object_t dets_f4[2] = {decoy, target};
    int n4 = sushitrack_update(hdl_, dets_f4, 2, out.data(), 8, 0.04f);
    ASSERT_GE(n4, 1);

    // Check if the true object retained the target_id
    const sushitrack_track_t* t = find_by_id(out, n4, target_id);
    ASSERT_NE(t, nullptr)
        << "The tracker failed to associate the target using ReID and Mahalanobis gating.";

    // The decoy sits at the predicted location (x~=100); the ReID-matched target is at x=110. The Kalman output is the filtered estimate, not the raw detection: it blends prediction and measurement as x = 100 + K*(110-100), which lands near 107.6 for this covariance. Asserting equality with the raw 110 would test "did the KF ignore its own prediction", not the ReID association. Instead assert the track moved decisively toward the target (x>105) rather than staying at the decoy (x~=100), which is the property this test actually validates and is robust to the Kalman gain.
    EXPECT_GT(t->x, 105.0f)
        << "Tracker associated the ID with the decoy object instead of using ReID.";
}
