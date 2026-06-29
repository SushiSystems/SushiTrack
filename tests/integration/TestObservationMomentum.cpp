/**
 * @file      TestObservationMomentum.cpp
 * @brief     End-to-end coverage of the observation-centric momentum (OCM) path.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      24.06.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 *
 * Scope
 * -----
 * The momentum decorator is unit-tested in isolation against a stub cost; this
 * file exercises it through the public C API with enable_ocm=1, driving real
 * Kalman-estimated motion so the IMM regime gate (momentum applies only while a
 * track is Cruising) is genuinely in the loop. The assertions check observable
 * contract — identity persistence, output sanity, and that the term stays inert
 * exactly where it is gated off — rather than a metric delta, which belongs to
 * the regression harness.
 */

#include <gtest/gtest.h>
#include "Helpers.h"

#include <algorithm>
#include <array>
#include <set>
#include <vector>

using namespace sushitrack_it;

namespace
{

/** @brief Builds a tracker on the IMM regime path with OCM toggled by @p ocm. */
inline SushiTrackHdl buildHdl(int ocm)
{
    SushiTrack_Params p = {};
    SushiTrack_GetDefaultParams(&p);
    p.time_aware         = 0;
    p.enable_tentative   = 0;
    p.kalman_type        = 2;   // IMM provides the motion regime
    p.enable_ocm         = ocm;
    p.primary_assoc_mode = 1;
    p.frame_rate         = 25;
    p.track_buffer       = 30;
    p.track_thresh       = 0.1f;
    p.high_thresh        = 0.5f;
    p.match_thresh       = 0.9f;
    return SushiTrack_Create(&p);
}

/**
 * @brief A single object that ramps to a steady cruising speed keeps one stable
 *        identity with OCM enabled, and every reported box stays finite.
 */
TEST(ObservationMomentumIT, CruisingObjectKeepsStableId)
{
    SushiTrackHdl hdl = buildHdl(/*ocm=*/1);
    ASSERT_NE(hdl, nullptr);

    std::vector<SushiTrack_Track> out(4);
    std::set<int64_t> hold_ids;

    float x = 100.0f, v = 0.0f;
    for (int i = 0; i < 45; ++i)
    {
        v = std::min(5.0f, v + 0.25f);
        x += v;
        SushiTrack_Object d = makeDet(x, 200.0f);
        int n = SushiTrack_Update(hdl, &d, 1, out.data(), 4, 0.04f);
        ASSERT_GE(n, 1);

        for (int k = 0; k < n; ++k)
            EXPECT_TRUE(geometryIsSane(out[static_cast<std::size_t>(k)]));

        // Sample identities only in the steady-cruise phase, after the ramp settles.
        if (i >= 20)
            for (int k = 0; k < n; ++k)
                if (out[static_cast<std::size_t>(k)].state == kTracked)
                    hold_ids.insert(out[static_cast<std::size_t>(k)].track_id);
    }

    EXPECT_EQ(hold_ids.size(), static_cast<std::size_t>(1))
        << "A steadily cruising object did not keep a single identity under OCM.";

    SushiTrack_Destroy(hdl);
}

/**
 * @brief A motionless object produces a bit-identical trajectory whether OCM is
 *        on or off: the speed gate suppresses momentum for a track that is not
 *        moving, so enabling the feature must be a no-op here.
 */
TEST(ObservationMomentumIT, StationaryObjectUnaffectedByMomentum)
{
    constexpr int kFrames = 15;

    auto run = [&](int ocm)
    {
        SushiTrackHdl hdl = buildHdl(ocm);
        EXPECT_NE(hdl, nullptr);
        std::vector<SushiTrack_Track> out(4);
        std::vector<std::array<float, 4>> boxes;
        for (int i = 0; i < kFrames; ++i)
        {
            SushiTrack_Object d = makeDet(100.0f, 200.0f);
            int n = SushiTrack_Update(hdl, &d, 1, out.data(), 4, 0.04f);
            EXPECT_GE(n, 1);
            boxes.push_back({out[0].x, out[0].y, out[0].width, out[0].height});
        }
        SushiTrack_Destroy(hdl);
        return boxes;
    };

    auto off = run(0);
    auto on  = run(1);

    ASSERT_EQ(off.size(), on.size());
    for (std::size_t i = 0; i < off.size(); ++i)
    {
        EXPECT_FLOAT_EQ(off[i][0], on[i][0]);
        EXPECT_FLOAT_EQ(off[i][1], on[i][1]);
        EXPECT_FLOAT_EQ(off[i][2], on[i][2]);
        EXPECT_FLOAT_EQ(off[i][3], on[i][3]);
    }
}

/**
 * @brief Two objects cruising in parallel lanes keep two distinct, stable
 *        identities under OCM — the momentum term does not mix them.
 */
TEST(ObservationMomentumIT, ParallelCruisersKeepDistinctIds)
{
    SushiTrackHdl hdl = buildHdl(/*ocm=*/1);
    ASSERT_NE(hdl, nullptr);

    std::vector<SushiTrack_Track> out(8);
    std::set<int64_t> a_ids, b_ids;

    float xa = 100.0f, xb = 100.0f, v = 0.0f;
    for (int i = 0; i < 45; ++i)
    {
        v = std::min(5.0f, v + 0.25f);
        xa += v;
        xb += v;
        SushiTrack_Object d[2] = { makeDet(xa, 200.0f), makeDet(xb, 500.0f) };
        int n = SushiTrack_Update(hdl, d, 2, out.data(), 8, 0.04f);
        ASSERT_GE(n, 2);

        if (i >= 20)
        {
            for (int k = 0; k < n; ++k)
            {
                if (out[static_cast<std::size_t>(k)].state != kTracked) continue;
                if (out[static_cast<std::size_t>(k)].y < 350.0f)
                    a_ids.insert(out[static_cast<std::size_t>(k)].track_id);
                else
                    b_ids.insert(out[static_cast<std::size_t>(k)].track_id);
            }
        }
    }

    ASSERT_EQ(a_ids.size(), static_cast<std::size_t>(1));
    ASSERT_EQ(b_ids.size(), static_cast<std::size_t>(1));
    EXPECT_NE(*a_ids.begin(), *b_ids.begin())
        << "Two parallel cruisers collapsed onto a shared identity under OCM.";

    SushiTrack_Destroy(hdl);
}

} // namespace
