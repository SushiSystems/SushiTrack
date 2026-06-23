/**
 * @file      TestInteractionContext.cpp
 * @brief     End-to-end coverage of the scene-level interaction-context module.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      23.06.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 *
 * Scope
 * -----
 * The interaction-context module (per-frame occlusion graph + occluder-aware
 * recovery penalty) is exercised end-to-end only inside the everything-on
 * full-stack test, where its effect cannot be isolated from ReID, Mahalanobis,
 * and the NSA filter. This file drives it on an otherwise-core path
 * (`enable_interaction_context=1` with its mandatory `enable_fsm=1`, and nothing
 * else) through the canonical scenario the module exists for:
 *
 *   A foreground object sweeps across a background object, suppressing its
 *   detection for a few frames. The background track goes Lost with the
 *   foreground object snapshotted as its occluder. When the background object
 *   reappears on its own trajectory — away from the occluder — it must recover
 *   its original identity, and the occluder must never surrender its own.
 *
 * The contract under test is identity integrity through an occlusion the module
 * is specifically designed to reason about; the assertions are invariant-style
 * (no id swap, correct-lane recovery, sane geometry).
 */

#include <gtest/gtest.h>
#include "Helpers.h"

#include <vector>

using namespace sushitrack_it;

namespace
{

/**
 * @brief Interaction-context fixture: occlusion graph + FSM only.
 */
class InteractionContextTest : public ::testing::Test
{
protected:
    SushiTrackHdl     hdl_    = nullptr;
    SushiTrack_Params params_ = {};

    void SetUp() override
    {
        SushiTrack_GetDefaultParams(&params_);
        params_.time_aware                  = 0;
        params_.enable_tentative            = 0;
        params_.enable_fsm                  = 1;   // required by interaction context
        params_.enable_interaction_context  = 1;
        params_.primary_assoc_mode          = 1;
        params_.frame_rate                  = 25;
        params_.track_buffer                = 30;
        params_.track_thresh                = 0.2f;
        params_.high_thresh                 = 0.5f;
        params_.match_thresh                = 0.9f;
        params_.interaction_iou_thresh      = 0.3f;
        params_.interaction_occluder_penalty = 0.3f;

        hdl_ = SushiTrack_Create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override { if (hdl_) SushiTrack_Destroy(hdl_); }
};

/**
 * @brief A foreground sweep that occludes a background object snapshots the
 *        occluder, and the background object recovers its own identity on
 *        reappearance without either track swapping ids.
 */
TEST_F(InteractionContextTest, OccludedBackgroundRecoversWithoutIdTheft)
{
    std::vector<SushiTrack_Track> out(8);

    // The two objects occupy slightly different rows (40 px apart, box height
    // 100) so their boxes overlap heavily as the foreground crosses — enough to
    // register the occlusion — but never coincide exactly. The distinct rows keep
    // every detection unambiguously bound to its own track, removing the
    // assignment tie that an exact overlap would create.
    const float fg_y = 300.0f;            // foreground row
    const float bg_y = 340.0f;            // background row (heavy vertical overlap)
    const float bg_x = 400.0f;            // background, stationary
    const float fg_v = 20.0f;             // foreground velocity (px/frame)
    const float kLaneSplit = 320.0f;      // midpoint used to classify rows
    float fx         = 220.0f;            // foreground start, well to the left

    // ---- Phase 1: confirm two distinct, separated identities ----
    int64_t id_bg = -1, id_fg = -1;
    for (int i = 0; i < 7; ++i)
    {
        SushiTrack_Object bg = makeDet(bg_x, bg_y);
        SushiTrack_Object fg = makeDet(fx, fg_y);
        SushiTrack_Object dets[2] = { bg, fg };
        int n = SushiTrack_Update(hdl_, dets, 2, out.data(), 8, 0.04f);
        ASSERT_GE(n, 2);
        for (int k = 0; k < n; ++k)
        {
            const SushiTrack_Track& t = out[static_cast<std::size_t>(k)];
            if (t.y < kLaneSplit) id_fg = t.track_id;   // upper row = foreground
            else                  id_bg = t.track_id;   // lower row = background
        }
        fx += fg_v;                                      // 220 -> 360 over 7 frames
    }
    ASSERT_NE(id_bg, -1);
    ASSERT_NE(id_fg, -1);
    ASSERT_NE(id_bg, id_fg);

    // ---- Phase 2: the foreground crosses over the background, its box overlapping
    //      the background's while it continues along its own row. The background
    //      detection is withheld; the background goes Lost with the foreground
    //      snapshotted as its occluder. ----
    for (int i = 0; i < 3; ++i)        // fx = 360, 380, 400 (boxes overlap bg)
    {
        SushiTrack_Object fg = makeDet(fx, fg_y);
        int n = SushiTrack_Update(hdl_, &fg, 1, out.data(), 8, 0.04f);
        ASSERT_GE(n, 1);
        const SushiTrack_Track* f = findById(out, n, id_fg);
        ASSERT_NE(f, nullptr) << "Foreground occluder lost its identity mid-sweep.";
        EXPECT_EQ(f->state, kTracked);
        fx += fg_v;
    }

    // ---- Phase 3: the foreground pulls clear and the background reappears on its
    //      own (unchanged) position and row. ----
    const SushiTrack_Track* f = nullptr;
    const SushiTrack_Track* b = nullptr;
    int n = 0;
    for (int i = 0; i < 2; ++i)        // fx = 420, then 440 (clear of bg again)
    {
        SushiTrack_Object bg = makeDet(bg_x, bg_y);
        SushiTrack_Object fg = makeDet(fx, fg_y);
        SushiTrack_Object dets[2] = { bg, fg };
        n = SushiTrack_Update(hdl_, dets, 2, out.data(), 8, 0.04f);
        ASSERT_GE(n, 2);
        f = findById(out, n, id_fg);
        b = findById(out, n, id_bg);
        fx += fg_v;
    }

    ASSERT_NE(f, nullptr) << "Occluder identity was lost across the sweep.";
    ASSERT_NE(b, nullptr)
        << "Background object failed to recover its identity after the occlusion.";
    EXPECT_EQ(f->state, kTracked);
    EXPECT_EQ(b->state, kTracked);
    EXPECT_TRUE(geometryIsSane(*f));
    EXPECT_TRUE(geometryIsSane(*b));
    // Each identity must stay anchored to its own row — neither was transferred
    // onto the other across the crossing.
    EXPECT_LT(f->y, kLaneSplit) << "Foreground identity migrated rows (id swap).";
    EXPECT_GT(b->y, kLaneSplit) << "Background identity migrated rows (id swap).";
    // The recovered background sits at its own stationary x, behind the occluder
    // that has moved on ahead.
    EXPECT_LT(b->x, f->x) << "Background did not recover at its own position.";
}

} // namespace
