/**************************************************************************/
/* test_time_aware.cpp                                                    */
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
 * @file test_time_aware.cpp
 * @brief Tests the tracker's time-aware mode.
 * @author Mustafa Garip
 */

#include <cstddef>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/c_api/sushitrack_c.h>

#include <c_detection_builders.hpp>

using namespace CDetections;

namespace
{
    /** @brief The number of empty frames of an occlusion and the dt of each one. */
    struct GapRow
    {
            int empty_frames;
            float dt;
    };

    /** @brief What the run over irregular intervals showed on its steps. */
    struct IrregularReport
    {
            int steps_without_the_id = 0;
            int steps_with_bad_geometry = 0;
    };
} // namespace

/**
 * @brief Wall-clock fixture: time_aware=1, 25 fps, 30-frame buffer.
 *
 * The retirement budget is track_buffer / frame_rate = 30 / 25 = 1.2 s of accumulated lost time.
 * The tests express an occlusion in seconds of dt and compare it with that budget.
 */
class Small_TrackerTimeAwareTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};
        std::vector<sushitrack_track_t> out_ = std::vector<sushitrack_track_t>(8);

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 1;
            params_.enable_tentative = 0;
            params_.primary_assoc_mode = 1;
            params_.frame_rate = 25;
            params_.track_buffer = 30;
            params_.track_thresh = 0.2f;
            params_.high_thresh = 0.5f;
            params_.match_thresh = 0.9f;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override
        {
            if (hdl_)
                sushitrack_destroy(hdl_);
        }

        /**
         * @brief Feeds @p frames frames of a box held at (@p x, @p y) with the given dt and
         *        returns the id of the track.
         */
        int64_t confirm_stationary(float x, float y, int frames, float dt)
        {
            int64_t id = -1;
            for (int i = 0; i < frames; ++i)
            {
                const int n = update_frame(hdl_, make_det(x, y), out_.data(), 8, dt);
                EXPECT_GE(n, 1);
                if (n >= 1)
                    id = out_[0].track_id;
            }
            return id;
        }

        /**
         * @brief Feeds a held track, hides it for the gap of @p row, shows it again at the same
         *        spot with the same dt and returns the id it had and the track count.
         */
        int64_t occlude_and_return(const GapRow& row, int& count)
        {
            const int64_t id = confirm_stationary(100.0f, 200.0f, 6, 0.04f);
            for (int i = 0; i < row.empty_frames; ++i)
                update_frame(hdl_, nullptr, 0, out_.data(), 8, row.dt);
            count = update_frame(hdl_, make_det(100.0f, 200.0f), out_.data(), 8, row.dt);
            return id;
        }

        /**
         * @brief Feeds a box that moves 100 px/s through eight steps of uneven dt and counts the
         *        steps that did not show the id of the first frame or showed a bad box.
         *
         * The position advances by v * dt, so each detection lands on the propagated prediction
         * whatever the gap is.
         */
        IrregularReport run_irregular_intervals()
        {
            IrregularReport report;
            const sushitrack_object_t seed = make_det(100.0f, 200.0f);
            EXPECT_GE(update_frame(hdl_, seed, out_.data(), 8, 0.04f), 1);
            const int64_t id = out_[0].track_id;

            const float dts[] = {0.02f, 0.09f, 0.04f, 0.12f, 0.03f, 0.08f, 0.05f, 0.15f};
            float x = 100.0f;
            for (const float dt : dts)
            {
                x += 100.0f * dt;
                const int n = update_frame(hdl_, make_det(x, 200.0f), out_.data(), 8, dt);
                const sushitrack_track_t* track = find_by_id(out_, n, id);
                if (track == nullptr)
                    ++report.steps_without_the_id;
                else if (!geometry_is_sane(*track))
                    ++report.steps_with_bad_geometry;
            }
            return report;
        }

        /**
         * @brief Holds a box for five frames, shows it once more after a stall of 10 s, then
         *        for four frames of 0.04 s, and counts the frames that did not show its id.
         */
        int frames_without_the_id_after_a_stall()
        {
            const int64_t id = confirm_stationary(100.0f, 200.0f, 5, 0.04f);
            int missing = 0;
            const float dts[] = {10.0f, 0.04f, 0.04f, 0.04f, 0.04f};
            for (const float dt : dts)
            {
                const int n = update_frame(hdl_, make_det(100.0f, 200.0f), out_.data(), 8, dt);
                const sushitrack_track_t* track = find_by_id(out_, n, id);
                if (track == nullptr || !geometry_is_sane(*track))
                    ++missing;
            }
            return missing;
        }
};

/**
 * @brief The fixture, run over occlusions whose lost time stays under the budget.
 */
class Small_TrackerSubBudgetGapTest : public Small_TrackerTimeAwareTest,
                                      public ::testing::WithParamInterface<GapRow>
{
};

/**
 * @brief An occlusion whose accumulated lost time stays under 1.2 s is bridged: the object
 *        recovers its original id and a sane box.
 *
 * The first empty frame marks the track Lost with its dt and each later one adds its dt. At
 * 3 frames of 0.2 s the lost time is 0.6 s, at 2 frames of 0.5 s it is 1.0 s; both are under the
 * budget, so cleanup_lost keeps the track and the reappearing detection re-activates it.
 */
TEST_P(Small_TrackerSubBudgetGapTest, ObjectRecoversItsId)
{
    int count = 0;
    const int64_t id = occlude_and_return(GetParam(), count);
    ASSERT_NE(id, -1);

    ASSERT_EQ(count, 1);
    EXPECT_EQ(out_[0].track_id, id);
    EXPECT_EQ(out_[0].state, K_TRACKED);
    EXPECT_TRUE(geometry_is_sane(out_[0]));
}

INSTANTIATE_TEST_SUITE_P(Gaps, Small_TrackerSubBudgetGapTest,
                         ::testing::Values(GapRow{3, 0.2f}, GapRow{2, 0.5f}));

/**
 * @brief The fixture, run over occlusions whose lost time exceeds the budget.
 */
class Small_TrackerOverBudgetGapTest : public Small_TrackerTimeAwareTest,
                                       public ::testing::WithParamInterface<GapRow>
{
};

/**
 * @brief An occlusion whose accumulated lost time exceeds 1.2 s retires the track: the object
 *        returns under a brand-new id and the stale one is gone, although the gap is only a few
 *        updates.
 *
 * A track marked Lost is first checked on the next frame. 4 frames of 0.5 s make 0.5, 1.0 and
 * 1.5 s, retired on the third; 2 frames of 1.0 s make 1.0 and 2.0 s, retired on the second.
 */
TEST_P(Small_TrackerOverBudgetGapTest, ObjectReturnsUnderANewId)
{
    int count = 0;
    const int64_t id = occlude_and_return(GetParam(), count);
    ASSERT_NE(id, -1);

    ASSERT_EQ(count, 1) << "The stale track stayed in the output, or no fresh track appeared.";
    EXPECT_NE(out_[0].track_id, id);
    EXPECT_TRUE(geometry_is_sane(out_[0]));
}

INSTANTIATE_TEST_SUITE_P(Gaps, Small_TrackerOverBudgetGapTest,
                         ::testing::Values(GapRow{4, 0.5f}, GapRow{2, 1.0f}));

/**
 * @brief Irregular inter-frame intervals (jitter and dropped frames) do not churn identity: a
 *        steadily moving object shows the id of its first frame and a sane box on every step.
 */
TEST_F(Small_TrackerTimeAwareTest, IrregularFrameIntervalsKeepIdentityStable)
{
    const IrregularReport report = run_irregular_intervals();

    EXPECT_EQ(report.steps_without_the_id, 0);
    EXPECT_EQ(report.steps_with_bad_geometry, 0);
}

/**
 * @brief A stall of 10 s between two frames of a held object leaves its id and a finite box on
 *        that frame and on the four frames of normal cadence after it.
 *
 * The cap of the prediction step is checked in test_wall_clock_timing_policy.cpp; the filter's
 * covariance is not visible through the C API, so this checks the result the cap protects.
 */
TEST_F(Small_TrackerTimeAwareTest, StalledFrameDoesNotLoseTheTrack)
{
    EXPECT_EQ(frames_without_the_id_after_a_stall(), 0);
}
