/**************************************************************************/
/* test_config_variations.cpp                                             */
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
 * @file test_config_variations.cpp
 * @brief Tests the tracker under each cost, coordinate and filter variant.
 * @author Mustafa Garip
 */

#include <cmath>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/c_api/sushitrack_c.h>

#include <c_detection_builders.hpp>

using namespace CDetections;

namespace
{
    /** @brief Whether follow_path() also requires every reported track to be Tracked. */
    enum class StateCheck
    {
        RequireTracked,
        Ignore
    };

    /**
     * @brief What follow_path() found: whether the first frame seeded a track, and the first
     *        later step at which the track was missing, changed id, left the required state or
     *        had insane geometry (K_NO_BAD_STEP when none did).
     */
    struct PathOutcome
    {
            static constexpr int K_NO_BAD_STEP = -1;

            bool seeded = false;
            int first_bad_step = K_NO_BAD_STEP;
    };

    /**
     * @brief Feeds @p path to @p tracker one detection per frame at 0.04 s and reports where the
     *        track seeded by the first detection stopped being the same sane track.
     */
    PathOutcome follow_path(sushitrack_tracker_t tracker,
                            const std::vector<sushitrack_object_t>& path, StateCheck state_check)
    {
        PathOutcome outcome;
        std::vector<sushitrack_track_t> out(4);
        if (update_frame(tracker, &path[0], 1, out.data(), 4, 0.04f) < 1)
            return outcome;
        outcome.seeded = true;
        const int64_t id = out[0].track_id;

        for (std::size_t step = 1; step < path.size(); ++step)
        {
            const int n = update_frame(tracker, &path[step], 1, out.data(), 4, 0.04f);
            const sushitrack_track_t* track = find_by_id(out, n, id);
            const bool state_ok =
                state_check == StateCheck::Ignore || (track != nullptr && track->state == K_TRACKED);
            if (n < 1 || track == nullptr || !state_ok || !geometry_is_sane(*track))
            {
                outcome.first_bad_step = static_cast<int>(step);
                return outcome;
            }
        }
        return outcome;
    }

    /** @brief Returns a smooth curve: accelerating in x, a slow sine drift in y; 31 frames. */
    std::vector<sushitrack_object_t> curved_path()
    {
        std::vector<sushitrack_object_t> path{make_det(100.0f, 200.0f)};
        for (int k = 1; k <= 30; ++k)
        {
            const float x = 100.0f + 3.0f * k + 0.05f * k * k;
            const float y = 200.0f + 8.0f * std::sin(0.2f * k);
            path.push_back(make_det(x, y));
        }
        return path;
    }

    /** @brief Returns a box that grows in width and height as it moves right; 21 frames. */
    std::vector<sushitrack_object_t> growing_box_path()
    {
        std::vector<sushitrack_object_t> path{make_det(100.0f, 200.0f, 0.9f, 50.0f, 100.0f)};
        for (int k = 1; k <= 20; ++k)
        {
            path.push_back(make_det(100.0f + 3.0f * k, 200.0f, 0.9f, 50.0f + 1.5f * k,
                                    100.0f + 2.0f * k));
        }
        return path;
    }

    /** @brief Returns a diagonal path whose score alternates between high and low; 9 frames. */
    std::vector<sushitrack_object_t> fluctuating_score_path()
    {
        std::vector<sushitrack_object_t> path{make_det(100.0f, 200.0f, 0.95f)};
        const float scores[] = {0.9f, 0.4f, 0.85f, 0.3f, 0.95f, 0.5f, 0.8f, 0.45f};
        int k = 1;
        for (const float score : scores)
        {
            path.push_back(make_det(100.0f + 4.0f * k, 200.0f + 1.0f * k, score));
            ++k;
        }
        return path;
    }

    sushitrack_params_t base_params()
    {
        sushitrack_params_t p{};
        sushitrack_get_default_params(&p);
        p.time_aware = 0;
        p.enable_tentative = 0;
        p.primary_assoc_mode = 1;
        p.frame_rate = 25;
        p.track_buffer = 30;
        p.track_thresh = 0.2f;
        p.high_thresh = 0.5f;
        p.match_thresh = 0.9f;
        return p;
    }
} // namespace

/** @brief Parameterised over iou_type: 0=IoU, 1=DIoU, 2=GIoU, 3=SIoU. */
class Small_TrackerIoUVariantTest : public ::testing::TestWithParam<int>
{
};

/**
 * @brief With each IoU variant, one object on a smooth curve keeps its id, stays Tracked and
 *        keeps sane geometry for 30 frames.
 */
TEST_P(Small_TrackerIoUVariantTest, EachIoUVariantKeepsOneObjectTrackedOnACurve)
{
    sushitrack_params_t p = base_params();
    p.iou_type = GetParam();
    sushitrack_tracker_t hdl = sushitrack_create(&p);
    ASSERT_NE(hdl, nullptr);

    const PathOutcome outcome = follow_path(hdl, curved_path(), StateCheck::RequireTracked);

    EXPECT_TRUE(outcome.seeded);
    EXPECT_EQ(outcome.first_bad_step, PathOutcome::K_NO_BAD_STEP);
    sushitrack_destroy(hdl);
}

INSTANTIATE_TEST_SUITE_P(AllVariants, Small_TrackerIoUVariantTest, ::testing::Values(0, 1, 2, 3));

/**
 * @brief In XYAH mode a box that grows in width and height, so its aspect ratio changes, keeps
 *        one id and sane geometry for 20 frames.
 */
TEST(Small_TrackerCoordinateSystemTest, XyahModeKeepsTheIdWhileTheAspectChanges)
{
    sushitrack_params_t p = base_params();
    p.enable_xyah = 1;
    sushitrack_tracker_t hdl = sushitrack_create(&p);
    ASSERT_NE(hdl, nullptr);

    const PathOutcome outcome = follow_path(hdl, growing_box_path(), StateCheck::Ignore);

    EXPECT_TRUE(outcome.seeded);
    EXPECT_EQ(outcome.first_bad_step, PathOutcome::K_NO_BAD_STEP);
    sushitrack_destroy(hdl);
}

/**
 * @brief With the NSA Kalman filter an object whose score swings between 0.3 and 0.95 keeps one
 *        id and sane geometry for eight frames.
 */
TEST(Small_TrackerKalmanVariantTest, NsaFilterKeepsTheIdWhileTheScoreFluctuates)
{
    sushitrack_params_t p = base_params();
    p.kalman_type = 1;     // NSA
    p.track_thresh = 0.1f; // keep mid/low scores in the primary pool
    sushitrack_tracker_t hdl = sushitrack_create(&p);
    ASSERT_NE(hdl, nullptr);

    const PathOutcome outcome =
        follow_path(hdl, fluctuating_score_path(), StateCheck::Ignore);

    EXPECT_TRUE(outcome.seeded);
    EXPECT_EQ(outcome.first_bad_step, PathOutcome::K_NO_BAD_STEP);
    sushitrack_destroy(hdl);
}
