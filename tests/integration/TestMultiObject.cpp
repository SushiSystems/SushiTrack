/**
 * @file      TestMultiObject.cpp
 * @brief     End-to-end coverage of multi-object identity, scale, and output capacity.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      15.06.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 *
 * Scope
 * -----
 *   - Identity separation: many simultaneous, well-separated objects each keep
 *     a stable, distinct id over a full sequence (no churn, no collisions).
 *   - Counter-moving pass: two separable objects crossing in x while held apart
 *     in y do not swap identities — the motion/geometry path alone suffices
 *     when the boxes never overlap.
 *   - Output-capacity contract: SushiTrack_Update never writes more than
 *     max_tracks entries and prioritises Tracked tracks when truncating.
 */

#include <gtest/gtest.h>
#include "Helpers.h"

#include <set>
#include <vector>

using namespace sushitrack_it;

namespace
{

class MultiObjectTest : public ::testing::Test
{
protected:
    SushiTrackHdl     hdl_    = nullptr;
    SushiTrack_Params params_ = {};

    void SetUp() override
    {
        SushiTrack_GetDefaultParams(&params_);
        params_.time_aware         = 0;
        params_.enable_tentative   = 0;
        params_.primary_assoc_mode = 1;
        params_.frame_rate         = 25;
        params_.track_buffer       = 30;
        params_.track_thresh       = 0.2f;
        params_.high_thresh        = 0.5f;
        params_.match_thresh       = 0.9f;

        hdl_ = SushiTrack_Create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override { if (hdl_) SushiTrack_Destroy(hdl_); }
};

/**
 * @brief A crowded grid of 25 well-separated objects moving uniformly: every
 *        object keeps a stable, unique identity across the run with sane
 *        geometry on every frame.
 */
TEST_F(MultiObjectTest, CrowdedSceneKeepsDistinctStableIdentities)
{
    constexpr int kCols   = 5;
    constexpr int kRows   = 5;
    constexpr int kCount  = kCols * kRows;
    constexpr int kFrames = 20;
    constexpr float kSpacing = 200.0f; // wide separation -> unambiguous association

    std::vector<SushiTrack_Track> out(static_cast<std::size_t>(kCount) + 8);

    auto buildFrame = [&](int frame) {
        std::vector<SushiTrack_Object> dets;
        dets.reserve(kCount);
        for (int r = 0; r < kRows; ++r)
            for (int c = 0; c < kCols; ++c)
            {
                const float x = 100.0f + c * kSpacing + 3.0f * frame; // drift right
                const float y = 100.0f + r * kSpacing + 1.0f * frame; // drift down
                dets.push_back(makeDet(x, y));
            }
        return dets;
    };

    // Frame 0: seed.
    auto f0 = buildFrame(0);
    int n0 = SushiTrack_Update(hdl_, f0.data(), kCount, out.data(),
                              static_cast<int>(out.size()), 0.04f);
    ASSERT_EQ(n0, kCount) << "Not every object produced a track on seed frame.";

    std::set<int64_t> seed_ids;
    for (int i = 0; i < n0; ++i)
        seed_ids.insert(out[static_cast<std::size_t>(i)].track_id);
    ASSERT_EQ(static_cast<int>(seed_ids.size()), kCount)
        << "Duplicate ids assigned on the seed frame.";

    // Drive the scene and require the id set to remain exactly the seed set.
    for (int frame = 1; frame < kFrames; ++frame)
    {
        auto f = buildFrame(frame);
        int n = SushiTrack_Update(hdl_, f.data(), kCount, out.data(),
                                 static_cast<int>(out.size()), 0.04f);
        ASSERT_EQ(n, kCount) << "Object count changed at frame " << frame;

        std::set<int64_t> ids;
        for (int i = 0; i < n; ++i)
        {
            const SushiTrack_Track& t = out[static_cast<std::size_t>(i)];
            EXPECT_TRUE(geometryIsSane(t)) << "Insane geometry at frame " << frame;
            ids.insert(t.track_id);
        }
        EXPECT_EQ(ids, seed_ids)
            << "Identity set drifted at frame " << frame
            << " (a track churned or swapped).";
    }
}

/**
 * @brief Two objects in distinct lanes (held 150 px apart in y so their boxes
 *        never overlap) moving in opposite x directions pass each other without
 *        exchanging identities.
 */
TEST_F(MultiObjectTest, CounterMovingSeparableObjectsDoNotSwapIds)
{
    std::vector<SushiTrack_Track> out(8);

    // Lane separation 150 px, box height 100 -> no vertical overlap at any time.
    const float y_left  = 150.0f;
    const float y_right = 300.0f;
    float lx = 100.0f, rx = 500.0f;
    const float speed = 40.0f;

    SushiTrack_Object l0 = makeDet(lx, y_left);
    SushiTrack_Object r0 = makeDet(rx, y_right);
    SushiTrack_Object d0[2] = { l0, r0 };
    int n0 = SushiTrack_Update(hdl_, d0, 2, out.data(), 8, 0.04f);
    ASSERT_GE(n0, 2);

    int64_t id_left = -1, id_right = -1;
    for (int i = 0; i < n0; ++i)
    {
        const SushiTrack_Track& t = out[static_cast<std::size_t>(i)];
        if (t.y < 225.0f) id_left = t.track_id;
        else              id_right = t.track_id;
    }
    ASSERT_NE(id_left, -1);
    ASSERT_NE(id_right, -1);
    ASSERT_NE(id_left, id_right);

    // March them past each other in x (they overlap in x mid-run, never in y).
    for (int k = 1; k <= 10; ++k)
    {
        lx += speed;
        rx -= speed;
        SushiTrack_Object l = makeDet(lx, y_left);
        SushiTrack_Object r = makeDet(rx, y_right);
        SushiTrack_Object d[2] = { l, r };
        int n = SushiTrack_Update(hdl_, d, 2, out.data(), 8, 0.04f);
        ASSERT_GE(n, 2);

        const SushiTrack_Track* tl = findById(out, n, id_left);
        const SushiTrack_Track* tr = findById(out, n, id_right);
        ASSERT_NE(tl, nullptr) << "Left-lane id vanished at step " << k;
        ASSERT_NE(tr, nullptr) << "Right-lane id vanished at step " << k;
        // Each id must stay anchored to its own lane.
        EXPECT_LT(tl->y, 225.0f) << "Left id jumped lanes at step " << k;
        EXPECT_GT(tr->y, 225.0f) << "Right id jumped lanes at step " << k;
    }
}

/**
 * @brief The output-capacity contract: with more active objects than the
 *        supplied buffer can hold, Update returns exactly max_tracks and every
 *        returned entry is a Tracked track (the highest output priority).
 */
TEST_F(MultiObjectTest, OutputRespectsMaxTracksCapacityAndPriority)
{
    constexpr int kObjects = 6;
    std::vector<SushiTrack_Track> out(kObjects);

    auto frame = [&](int f) {
        std::vector<SushiTrack_Object> dets;
        for (int i = 0; i < kObjects; ++i)
            dets.push_back(makeDet(100.0f + i * 200.0f + 2.0f * f, 200.0f));
        return dets;
    };

    // Establish all six objects with a generous buffer.
    for (int f = 0; f < 3; ++f)
    {
        auto d = frame(f);
        int n = SushiTrack_Update(hdl_, d.data(), kObjects, out.data(), kObjects, 0.04f);
        ASSERT_GE(n, 1);
    }

    // Now request the same frame but only allow room for half the tracks.
    constexpr int kCap = 3;
    auto d = frame(3);
    int n = SushiTrack_Update(hdl_, d.data(), kObjects, out.data(), kCap, 0.04f);

    EXPECT_LE(n, kCap) << "Update wrote more tracks than max_tracks permitted.";
    EXPECT_EQ(n, kCap) << "With six active tracks and capacity three, exactly "
                          "three should be returned.";
    for (int i = 0; i < n; ++i)
    {
        EXPECT_EQ(out[static_cast<std::size_t>(i)].state, kTracked)
            << "Truncated output should retain Tracked tracks ahead of others.";
        EXPECT_TRUE(geometryIsSane(out[static_cast<std::size_t>(i)]));
    }
}

} // namespace
