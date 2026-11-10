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
 * @file test_observation_momentum.cpp
 * @brief Tests the tracker's observation-centric momentum.
 * @author Mustafa Garip
 */

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <set>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/c_api/sushitrack_c.h>

#include <c_detection_builders.hpp>

using namespace CDetections;

namespace
{
    /** @brief The box of a track: x, y, width and height. */
    using Box = std::array<float, 4>;

    /** @brief What the Tracked tracks of the steady-cruise phase showed. */
    struct CruiseReport
    {
            std::set<int64_t> lane_a_ids;
            std::set<int64_t> lane_b_ids;
            int tracks_with_bad_geometry = 0;
    };

    /** @brief Builds a tracker on the IMM regime path with OCM toggled by @p ocm. */
    sushitrack_tracker_t build_hdl(int ocm)
    {
        sushitrack_params_t p = {};
        sushitrack_get_default_params(&p);
        p.time_aware = 0;
        p.enable_tentative = 0;
        p.kalman_type = 2;
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
     * @brief Feeds a motionless box for @p frames frames into a tracker with OCM set to
     *        @p ocm and returns the box of the first track after each frame.
     */
    std::vector<Box> motionless_boxes(int ocm, int frames)
    {
        sushitrack_tracker_t hdl = build_hdl(ocm);
        EXPECT_NE(hdl, nullptr);
        std::vector<sushitrack_track_t> out(4);
        std::vector<Box> boxes;
        for (int i = 0; i < frames; ++i)
        {
            const sushitrack_object_t detection = make_det(100.0f, 200.0f);
            EXPECT_GE(update_frame(hdl, detection, out.data(), 4, 0.04f), 1);
            boxes.push_back({out[0].x, out[0].y, out[0].width, out[0].height});
        }
        sushitrack_destroy(hdl);
        return boxes;
    }

    /**
     * @brief Counts the frames whose boxes differ in any of the four values.
     */
    int differing_frames(const std::vector<Box>& first, const std::vector<Box>& second)
    {
        int differing = 0;
        for (std::size_t i = 0; i < first.size(); ++i)
        {
            if (first[i] != second[i])
                ++differing;
        }
        return differing;
    }
} // namespace

/**
 * @brief Fixture of a tracker with OCM on over the IMM regime path.
 */
class Small_TrackerObservationMomentumTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;

        void SetUp() override
        {
            hdl_ = build_hdl(1);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override
        {
            if (hdl_)
                sushitrack_destroy(hdl_);
        }

        /**
         * @brief Ramps @p lanes objects, 300 px apart in y from y 200, to 5 px per frame over 20
         *        frames, holds the pace to frame 45 and records the Tracked tracks from frame
         *        20 on, one id set per lane, and the tracks whose box is not sane.
         */
        CruiseReport cruise(int lanes)
        {
            CruiseReport report;
            std::vector<sushitrack_track_t> out(8);
            float x = 100.0f;
            float v = 0.0f;
            for (int i = 0; i < 45; ++i)
            {
                v = std::min(5.0f, v + 0.25f);
                x += v;
                std::vector<sushitrack_object_t> frame;
                for (int lane = 0; lane < lanes; ++lane)
                    frame.push_back(make_det(x, 200.0f + 300.0f * static_cast<float>(lane)));
                const int n = update_frame(hdl_, frame.data(), lanes, out.data(), 8, 0.04f);
                for (int k = 0; k < n; ++k)
                {
                    const sushitrack_track_t& track = out[static_cast<std::size_t>(k)];
                    if (!geometry_is_sane(track))
                        ++report.tracks_with_bad_geometry;
                    if (i < 20 || track.state != K_TRACKED)
                        continue;
                    (track.y < 350.0f ? report.lane_a_ids : report.lane_b_ids)
                        .insert(track.track_id);
                }
            }
            return report;
        }
};

/**
 * @brief A single object that ramps to a steady cruising speed shows one Tracked id from frame
 *        20 on with OCM enabled.
 *
 * The scene also passes with OCM off, so it checks that the momentum term does no harm to a
 * clean track; the term itself is checked where a direction decides the assignment.
 */
TEST_F(Small_TrackerObservationMomentumTest, CruisingObjectKeepsStableId)
{
    const CruiseReport report = cruise(1);

    EXPECT_EQ(report.lane_a_ids.size(), static_cast<std::size_t>(1))
        << "A steadily cruising object did not keep a single identity under OCM.";
}

/**
 * @brief Every box a cruising object reports under OCM is finite with a positive extent.
 */
TEST_F(Small_TrackerObservationMomentumTest, CruisingObjectGeometryStaysSane)
{
    EXPECT_EQ(cruise(1).tracks_with_bad_geometry, 0);
}

/**
 * @brief A motionless object produces a bit-identical trajectory whether OCM is on or off.
 *
 * The speed gate (observed speed under ocm_min_velocity 1 px per frame) suppresses the momentum
 * term for a track that is not moving, so enabling the feature must be a no-op here.
 */
TEST(Small_TrackerObservationMomentumSwitchTest, StationaryObjectUnaffectedByMomentum)
{
    const std::vector<Box> off = motionless_boxes(0, 15);
    const std::vector<Box> on = motionless_boxes(1, 15);

    ASSERT_EQ(off.size(), on.size());
    EXPECT_EQ(differing_frames(off, on), 0);
}

/**
 * @brief Two objects cruising in parallel lanes keep two distinct, stable Tracked ids under
 *        OCM: the momentum term does not mix them.
 */
TEST_F(Small_TrackerObservationMomentumTest, ParallelCruisersKeepDistinctIds)
{
    const CruiseReport report = cruise(2);

    ASSERT_EQ(report.lane_a_ids.size(), static_cast<std::size_t>(1));
    ASSERT_EQ(report.lane_b_ids.size(), static_cast<std::size_t>(1));
    EXPECT_NE(*report.lane_a_ids.begin(), *report.lane_b_ids.begin())
        << "Two parallel cruisers collapsed onto a shared identity under OCM.";
}
