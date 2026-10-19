/**************************************************************************/
/* test_observation_momentum.cpp                                          */
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
 * @file test_observation_momentum.cpp
 * @brief Tests the tracker's observation-centric momentum.
 * @author Mustafa Garip
 */

#include <algorithm>
#include <array>
#include <set>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/c_api/sushitrack_c.h>

#include <c_detection_builders.hpp>

using namespace CDetections;

namespace
{

    /** @brief Builds a tracker on the IMM regime path with OCM toggled by @p ocm. */
    inline sushitrack_tracker_t build_hdl(int ocm)
    {
        sushitrack_params_t p = {};
        sushitrack_get_default_params(&p);
        p.time_aware = 0;
        p.enable_tentative = 0;
        p.kalman_type = 2; // IMM provides the motion regime
        p.enable_ocm = ocm;
        p.primary_assoc_mode = 1;
        p.frame_rate = 25;
        p.track_buffer = 30;
        p.track_thresh = 0.1f;
        p.high_thresh = 0.5f;
        p.match_thresh = 0.9f;
        return sushitrack_create(&p);
    }

    /**
     * @brief A single object that ramps to a steady cruising speed keeps one stable
     *        identity with OCM enabled, and every reported box stays finite.
     */
    TEST(Small_TrackerObservationMomentumTest, CruisingObjectKeepsStableId)
    {
        sushitrack_tracker_t hdl = build_hdl(/*ocm=*/1);
        ASSERT_NE(hdl, nullptr);

        std::vector<sushitrack_track_t> out(4);
        std::set<int64_t> hold_ids;

        float x = 100.0f, v = 0.0f;
        for (int i = 0; i < 45; ++i)
        {
            v = std::min(5.0f, v + 0.25f);
            x += v;
            sushitrack_object_t d = make_det(x, 200.0f);
            int n = sushitrack_update(hdl, &d, 1, out.data(), 4, 0.04f);
            ASSERT_GE(n, 1);

            for (int k = 0; k < n; ++k)
                EXPECT_TRUE(geometry_is_sane(out[static_cast<std::size_t>(k)]));

            // Sample identities only in the steady-cruise phase, after the ramp settles.
            if (i >= 20)
                for (int k = 0; k < n; ++k)
                    if (out[static_cast<std::size_t>(k)].state == K_TRACKED)
                        hold_ids.insert(out[static_cast<std::size_t>(k)].track_id);
        }

        EXPECT_EQ(hold_ids.size(), static_cast<std::size_t>(1))
            << "A steadily cruising object did not keep a single identity under OCM.";

        sushitrack_destroy(hdl);
    }

    /**
     * @brief A motionless object produces a bit-identical trajectory whether OCM is
     *        on or off: the speed gate suppresses momentum for a track that is not
     *        moving, so enabling the feature must be a no-op here.
     */
    TEST(Small_TrackerObservationMomentumTest, StationaryObjectUnaffectedByMomentum)
    {
        constexpr int K_FRAMES = 15;

        auto run = [&](int ocm)
        {
            sushitrack_tracker_t hdl = build_hdl(ocm);
            EXPECT_NE(hdl, nullptr);
            std::vector<sushitrack_track_t> out(4);
            std::vector<std::array<float, 4>> boxes;
            for (int i = 0; i < K_FRAMES; ++i)
            {
                sushitrack_object_t d = make_det(100.0f, 200.0f);
                int n = sushitrack_update(hdl, &d, 1, out.data(), 4, 0.04f);
                EXPECT_GE(n, 1);
                boxes.push_back({out[0].x, out[0].y, out[0].width, out[0].height});
            }
            sushitrack_destroy(hdl);
            return boxes;
        };

        auto off = run(0);
        auto on = run(1);

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
    TEST(Small_TrackerObservationMomentumTest, ParallelCruisersKeepDistinctIds)
    {
        sushitrack_tracker_t hdl = build_hdl(/*ocm=*/1);
        ASSERT_NE(hdl, nullptr);

        std::vector<sushitrack_track_t> out(8);
        std::set<int64_t> a_ids, b_ids;

        float xa = 100.0f, xb = 100.0f, v = 0.0f;
        for (int i = 0; i < 45; ++i)
        {
            v = std::min(5.0f, v + 0.25f);
            xa += v;
            xb += v;
            sushitrack_object_t d[2] = {make_det(xa, 200.0f), make_det(xb, 500.0f)};
            int n = sushitrack_update(hdl, d, 2, out.data(), 8, 0.04f);
            ASSERT_GE(n, 2);

            if (i >= 20)
            {
                for (int k = 0; k < n; ++k)
                {
                    if (out[static_cast<std::size_t>(k)].state != K_TRACKED)
                        continue;
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

        sushitrack_destroy(hdl);
    }

} // namespace
