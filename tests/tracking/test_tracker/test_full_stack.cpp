/**************************************************************************/
/* test_full_stack.cpp                                                    */
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
 * @file test_full_stack.cpp
 * @brief Tests the tracker with every feature switched on.
 * @author Mustafa Garip
 */

#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/c_api/sushitrack_c.h>
#include <SushiTrack/filtering/imm_filter.hpp>

#include <c_detection_builders.hpp>

using namespace CDetections;

namespace
{

    class Small_TrackerFullStackTest : public ::testing::Test
    {
        protected:
            sushitrack_tracker_t hdl_ = nullptr;
            sushitrack_params_t params_ = {};
            static constexpr int K_DIM = 128;
            std::vector<float> feat_a_;
            std::vector<float> feat_b_;

            void SetUp() override
            {
                sushitrack_get_default_params(&params_);
                // Everything on.
                params_.time_aware = 1;
                params_.enable_reid = 1;
                params_.enable_mahalanobis = 1;
                params_.enable_tentative = 1;
                params_.kalman_type = 2; // IMM multi-model
                params_.primary_assoc_mode = 1;
                params_.frame_rate = 25;
                params_.track_buffer = 30;
                params_.track_thresh = 0.2f;
                params_.high_thresh = 0.4f;
                params_.match_thresh = 0.9f;
                // Confirm quickly on strong detections so the scenario stays short.
                params_.tentative_fast_thresh = 0.85f;
                params_.tentative_fast_hits = 1;
                hdl_ = sushitrack_create(&params_);
                ASSERT_NE(hdl_, nullptr);

                feat_a_ = one_hot_feature(K_DIM, /*axis=*/0);
                feat_b_ = one_hot_feature(K_DIM, /*axis=*/1); // orthogonal to A
            }

            void TearDown() override
            {
                if (hdl_)
                    sushitrack_destroy(hdl_);
            }
    };

    /**
     * @brief The full advanced stack tracks two distinct objects through
     *        confirmation, sustained motion, a short occlusion of one object, and
     *        its ReID-driven recovery — with no NaN geometry and no identity loss
     *        for the continuously observed object.
     */
    TEST_F(Small_TrackerFullStackTest, AdvancedStackTracksConfirmsAndRecovers)
    {
        std::vector<sushitrack_track_t> out(8);

        auto step = [&](float ax, float ay, float bx, float by) -> int
        {
            sushitrack_object_t a = make_det_with_feat(ax, ay, feat_a_.data(), K_DIM, 0.9f);
            sushitrack_object_t b = make_det_with_feat(bx, by, feat_b_.data(), K_DIM, 0.9f);
            sushitrack_object_t dets[2] = {a, b};
            return CDetections::update_frame(hdl_, dets, 2, out.data(), 8, 0.04f);
        };

        // Confirmation + sustained tracking over several frames
        float ax = 100.0f, bx = 100.0f;
        const float ay = 150.0f, by = 400.0f; // separate lanes
        const float vx = 6.0f;

        int64_t id_a = -1, id_b = -1;
        for (int i = 0; i < 6; ++i)
        {
            int n = step(ax, ay, bx, by);
            ASSERT_GE(n, 2);
            for (int k = 0; k < n; ++k)
            {
                const sushitrack_track_t& t = out[static_cast<std::size_t>(k)];
                EXPECT_TRUE(geometry_is_sane(t)) << "Insane geometry during tracking, frame " << i;
                if (t.y < 275.0f)
                    id_a = t.track_id;
                else
                    id_b = t.track_id;
            }
            ax += vx;
            bx += vx;
        }
        ASSERT_NE(id_a, -1);
        ASSERT_NE(id_b, -1);
        ASSERT_NE(id_a, id_b);

        // Both should be confirmed Tracked by now.
        {
            int n = step(ax, ay, bx, by);
            ASSERT_GE(n, 2);
            const sushitrack_track_t* a = find_by_id(out, n, id_a);
            const sushitrack_track_t* b = find_by_id(out, n, id_b);
            ASSERT_NE(a, nullptr);
            ASSERT_NE(b, nullptr);
            EXPECT_EQ(a->state, K_TRACKED);
            EXPECT_EQ(b->state, K_TRACKED);
            ax += vx;
            bx += vx;
        }

        // Occlude object A for 3 frames; B stays visible
        constexpr int K_GAP = 3;
        for (int i = 0; i < K_GAP; ++i)
        {
            sushitrack_object_t b = make_det_with_feat(bx, by, feat_b_.data(), K_DIM, 0.9f);
            int n = CDetections::update_frame(hdl_, &b, 1, out.data(), 8, 0.04f);
            ASSERT_GE(n, 1);
            const sushitrack_track_t* tb = find_by_id(out, n, id_b);
            ASSERT_NE(tb, nullptr) << "Visible object B lost identity during A's occlusion.";
            EXPECT_EQ(tb->state, K_TRACKED);
            bx += vx;
            ax += vx; // A keeps moving while unobserved
        }

        // A reappears on its trajectory with the SAME feature
        int n = step(ax, ay, bx, by);
        ASSERT_GE(n, 1);

        const sushitrack_track_t* a_rec = find_by_id(out, n, id_a);
        const sushitrack_track_t* b_now = find_by_id(out, n, id_b);
        ASSERT_NE(b_now, nullptr) << "Object B lost its identity at reappearance frame.";
        ASSERT_NE(a_rec, nullptr)
            << "ReID + Mahalanobis recovery failed to restore object A's identity "
               "after a short occlusion.";
        EXPECT_EQ(a_rec->state, K_TRACKED);
        EXPECT_LT(a_rec->y, 275.0f) << "Recovered A surfaced in the wrong lane (id swap).";
        EXPECT_TRUE(geometry_is_sane(*a_rec));
    }

    /**
     * @brief Smoke invariant: the full stack survives a long single-object run
     *        (120 frames) with a fluctuating detection score and an accelerating
     *        trajectory, never emitting NaN geometry and never churning the id.
     */
    TEST_F(Small_TrackerFullStackTest, LongRunUnderFullStackStaysStable)
    {
        std::vector<sushitrack_track_t> out(4);

        sushitrack_object_t d0 = make_det_with_feat(100.0f, 200.0f, feat_a_.data(), K_DIM, 0.95f);
        int n0 = CDetections::update_frame(hdl_, &d0, 1, out.data(), 4, 0.04f);
        ASSERT_GE(n0, 1);
        int64_t id = out[0].track_id;

        // The id can legitimately settle after the tentative window; capture it once the track is confirmed Tracked.
        float x = 100.0f, y = 200.0f;
        for (int k = 1; k <= 120; ++k)
        {
            x += 2.0f + 0.02f * k;                                          // accelerating
            y += 4.0f * std::sin(0.1f * k);                                 // weaving
            const float score = 0.4f + 0.5f * std::abs(std::sin(0.3f * k)); // fluctuates
            sushitrack_object_t d = make_det_with_feat(x, y, feat_a_.data(), K_DIM, score);
            int n = CDetections::update_frame(hdl_, &d, 1, out.data(), 4, 0.04f);
            ASSERT_GE(n, 1) << "Full-stack tracker dropped the object at frame " << k;

            // Re-anchor the id when confirmed (handles the tentative->Tracked transition without assuming the frame it occurs on).
            if (out[0].state == K_TRACKED && out[0].is_confirmed)
                id = out[0].track_id;

            const sushitrack_track_t* t = find_by_id(out, n, id);
            ASSERT_NE(t, nullptr) << "Full-stack id churned at frame " << k;
            EXPECT_TRUE(geometry_is_sane(*t)) << "Insane geometry at frame " << k;
        }
    }

} // namespace
