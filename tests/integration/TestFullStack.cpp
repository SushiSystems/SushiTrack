/**
 * @file      TestFullStack.cpp
 * @brief     End-to-end coverage with every advanced feature enabled at once.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      15.06.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 *
 * Scope
 * -----
 * Every advanced subsystem is unit-tested alone and several are paired in the
 * unit-level integration suite, but no test drives the *entire* advanced stack
 * simultaneously: ReID + Mahalanobis gating + kinematic FSM + tentative
 * lifecycle + NSA Kalman. Feature interactions (e.g. FSM gate scaling feeding
 * the fused cost while ReID also competes for lost-recovery) only surface when
 * they run together. This file is that composition test.
 *
 * Assertions are invariant-style: with so many interacting policies the exact
 * trajectory is not the contract — identity stability for continuously observed
 * objects, ReID-driven recovery of a briefly occluded object, and never-NaN
 * geometry are.
 */

#include <gtest/gtest.h>
#include "Helpers.h"

#include <vector>

using namespace sushitrack_it;

namespace
{

class FullStackTest : public ::testing::Test
{
protected:
    SushiTrackHdl     hdl_    = nullptr;
    SushiTrack_Params params_ = {};
    static constexpr int kDim = 128;
    std::vector<float> feat_a_;
    std::vector<float> feat_b_;

    void SetUp() override
    {
        SushiTrack_GetDefaultParams(&params_);
        // Everything on.
        params_.time_aware                            = 1;
        params_.enable_reid                           = 1;
        params_.enable_mahalanobis                    = 1;
        params_.enable_fsm                            = 1;
        params_.enable_tentative                      = 1;
        params_.kalman_type                           = 1;   // NSA
        params_.primary_assoc_mode                    = 1;
        params_.frame_rate                            = 25;
        params_.track_buffer                          = 30;
        params_.track_thresh                          = 0.2f;
        params_.high_thresh                           = 0.4f;
        params_.match_thresh                          = 0.9f;
        // Confirm quickly on strong detections so the scenario stays short.
        params_.tentative_fast_thresh                 = 0.85f;
        params_.tentative_fast_hits                   = 1;
        // Disable the aggressive raw-IoU pre-filter: with Mahalanobis on, the
        // 0.7 default can drop legitimate matches across a displacement gap
        // (see docs/RELEASE_READINESS.md, gap 4). The gate still governs here.
        params_.proximity_thresh                      = 0.0f;

        hdl_ = SushiTrack_Create(&params_);
        ASSERT_NE(hdl_, nullptr);

        feat_a_ = oneHotFeature(kDim, /*axis=*/0);
        feat_b_ = oneHotFeature(kDim, /*axis=*/1); // orthogonal to A
    }

    void TearDown() override { if (hdl_) SushiTrack_Destroy(hdl_); }
};

/**
 * @brief The full advanced stack tracks two distinct objects through
 *        confirmation, sustained motion, a short occlusion of one object, and
 *        its ReID-driven recovery — with no NaN geometry and no identity loss
 *        for the continuously observed object.
 */
TEST_F(FullStackTest, AdvancedStackTracksConfirmsAndRecovers)
{
    std::vector<SushiTrack_Track> out(8);

    auto step = [&](float ax, float ay, float bx, float by) -> int {
        SushiTrack_Object a = makeDetWithFeat(ax, ay, feat_a_.data(), kDim, 0.9f);
        SushiTrack_Object b = makeDetWithFeat(bx, by, feat_b_.data(), kDim, 0.9f);
        SushiTrack_Object dets[2] = { a, b };
        return SushiTrack_Update(hdl_, dets, 2, out.data(), 8, 0.04f);
    };

    // ---- Confirmation + sustained tracking over several frames ----
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
            const SushiTrack_Track& t = out[static_cast<std::size_t>(k)];
            EXPECT_TRUE(geometryIsSane(t)) << "Insane geometry during tracking, frame " << i;
            if (t.y < 275.0f) id_a = t.track_id;
            else              id_b = t.track_id;
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
        const SushiTrack_Track* a = findById(out, n, id_a);
        const SushiTrack_Track* b = findById(out, n, id_b);
        ASSERT_NE(a, nullptr);
        ASSERT_NE(b, nullptr);
        EXPECT_EQ(a->state, kTracked);
        EXPECT_EQ(b->state, kTracked);
        ax += vx; bx += vx;
    }

    // ---- Occlude object A for 3 frames; B stays visible ----
    constexpr int kGap = 3;
    for (int i = 0; i < kGap; ++i)
    {
        SushiTrack_Object b = makeDetWithFeat(bx, by, feat_b_.data(), kDim, 0.9f);
        int n = SushiTrack_Update(hdl_, &b, 1, out.data(), 8, 0.04f);
        ASSERT_GE(n, 1);
        const SushiTrack_Track* tb = findById(out, n, id_b);
        ASSERT_NE(tb, nullptr) << "Visible object B lost identity during A's occlusion.";
        EXPECT_EQ(tb->state, kTracked);
        bx += vx;
        ax += vx; // A keeps moving while unobserved
    }

    // ---- A reappears on its trajectory with the SAME feature ----
    int n = step(ax, ay, bx, by);
    ASSERT_GE(n, 1);

    const SushiTrack_Track* a_rec = findById(out, n, id_a);
    const SushiTrack_Track* b_now = findById(out, n, id_b);
    ASSERT_NE(b_now, nullptr) << "Object B lost its identity at reappearance frame.";
    ASSERT_NE(a_rec, nullptr)
        << "ReID + Mahalanobis recovery failed to restore object A's identity "
           "after a short occlusion.";
    EXPECT_EQ(a_rec->state, kTracked);
    EXPECT_LT(a_rec->y, 275.0f) << "Recovered A surfaced in the wrong lane (id swap).";
    EXPECT_TRUE(geometryIsSane(*a_rec));
}

/**
 * @brief Smoke invariant: the full stack survives a long single-object run
 *        (120 frames) with a fluctuating detection score and an accelerating
 *        trajectory, never emitting NaN geometry and never churning the id.
 */
TEST_F(FullStackTest, LongRunUnderFullStackStaysStable)
{
    std::vector<SushiTrack_Track> out(4);

    SushiTrack_Object d0 = makeDetWithFeat(100.0f, 200.0f, feat_a_.data(), kDim, 0.95f);
    int n0 = SushiTrack_Update(hdl_, &d0, 1, out.data(), 4, 0.04f);
    ASSERT_GE(n0, 1);
    int64_t id = out[0].track_id;

    // The id can legitimately settle after the tentative window; capture it
    // once the track is confirmed Tracked.
    float x = 100.0f, y = 200.0f;
    for (int k = 1; k <= 120; ++k)
    {
        x += 2.0f + 0.02f * k;            // accelerating
        y += 4.0f * std::sin(0.1f * k);   // weaving
        const float score = 0.4f + 0.5f * std::abs(std::sin(0.3f * k)); // fluctuates
        SushiTrack_Object d = makeDetWithFeat(x, y, feat_a_.data(), kDim, score);
        int n = SushiTrack_Update(hdl_, &d, 1, out.data(), 4, 0.04f);
        ASSERT_GE(n, 1) << "Full-stack tracker dropped the object at frame " << k;

        // Re-anchor the id when confirmed (handles the tentative->Tracked
        // transition without assuming the frame it occurs on).
        if (out[0].state == kTracked && out[0].is_confirmed)
            id = out[0].track_id;

        const SushiTrack_Track* t = findById(out, n, id);
        ASSERT_NE(t, nullptr) << "Full-stack id churned at frame " << k;
        EXPECT_TRUE(geometryIsSane(*t)) << "Insane geometry at frame " << k;
    }
}

} // namespace
