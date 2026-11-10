/**************************************************************************/
/* test_tentative_lifecycle.cpp                                           */
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
 * @file test_tentative_lifecycle.cpp
 * @brief Tests the tracker's tentative track lifecycle.
 * @author Mustafa Garip
 */

#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/c_api/sushitrack_c.h>

#include <c_detection_builders.hpp>
#include <captured_log.hpp>

using namespace CDetections;

namespace
{
    /** @brief Builds params with tentative enabled and the given init-suppression IoU. */
    sushitrack_params_t suppress_params(float suppress_iou)
    {
        sushitrack_params_t p = {};
        sushitrack_get_default_params(&p);
        p.time_aware = 1;
        p.enable_tentative = 1;
        p.tentative_confirm_hits = 3;
        p.tentative_fast_thresh = 1.0f;
        p.tentative_match_thresh = 0.9f;
        p.track_thresh = 0.1f;
        p.high_thresh = 0.2f;
        p.frame_rate = 25;
        p.tentative_init_suppress_iou = suppress_iou;
        return p;
    }

    /** @brief Confirms a single track over three matched frames at (100,200)->(104,204). */
    void confirm_track(sushitrack_tracker_t hdl, std::vector<sushitrack_track_t>& out)
    {
        update_frame(hdl, make_det(100.0f, 200.0f), out.data(), 10, 0.04f);
        update_frame(hdl, make_det(102.0f, 202.0f), out.data(), 10, 0.04f);
        const int n = update_frame(hdl, make_det(104.0f, 204.0f), out.data(), 10, 0.04f);
        ASSERT_EQ(n, 1);
        ASSERT_EQ(out[0].state, K_TRACKED);
    }

    /** @brief Counts the first @p n tracks of @p out in state @p state. */
    int count_in_state(const std::vector<sushitrack_track_t>& out, int n, int state)
    {
        int count = 0;
        for (int i = 0; i < n; ++i)
        {
            if (out[static_cast<std::size_t>(i)].state == state)
                ++count;
        }
        return count;
    }

    /** @brief Returns the track among the first @p n of @p out whose x is closest to @p x. */
    const sushitrack_track_t* nearest_x(const std::vector<sushitrack_track_t>& out, int n, float x)
    {
        const sushitrack_track_t* nearest = nullptr;
        float best = std::numeric_limits<float>::max();
        for (int i = 0; i < n; ++i)
        {
            const float distance = std::abs(out[static_cast<std::size_t>(i)].x - x);
            if (distance < best)
            {
                best = distance;
                nearest = &out[static_cast<std::size_t>(i)];
            }
        }
        return nearest;
    }

    /** @brief What the DEBUG records of the tracking module said about their frame. */
    struct LifecycleCounts
    {
            int total = 0;
            int without_frame_prefix = 0;
    };

    /** @brief Counts the tracking DEBUG records of @p log and those that do not open with "Frame ". */
    LifecycleCounts count_lifecycle_records(SushiTrack::CapturedLog& log)
    {
        LifecycleCounts counts;
        for (const SushiLog::CapturedRecord& record : log.records())
        {
            if (record.module != "tracking" || record.level != SushiLog::LogLevel::Debug)
                continue;
            ++counts.total;
            if (record.message.rfind("Frame ", 0) != 0U)
                ++counts.without_frame_prefix;
        }
        return counts;
    }

    /**
     * @brief One or two missed frames of a time-aware tentative track, and the state and lost time
     *        they leave. A second_dt of zero means there is no second miss.
     */
    struct MissRow
    {
            float first_dt;
            float second_dt;
            int state;
            float lost_time;
    };

    /** @brief The missed frames of a frame-counted tentative track, their dt and the state left. */
    struct FrameMissRow
    {
            int misses;
            float dt;
            int state;
    };

    /** @brief An init-suppression IoU and the tracks the scene leaves in each state. */
    struct SuppressRow
    {
            float suppress_iou;
            int tracks;
            int tracked;
            int tentative;
    };
} // namespace

/**
 * @brief Tracker fixture configured for time-aware tentative testing.
 *
 * track_buffer=2, frame_rate=25 gives one frame interval of 0.04 s. time_aware=1 makes the age-out
 * use the seconds budget: tentative_max_age_seconds=0.08 removes a track once its accumulated lost
 * time exceeds 0.08 s. tentative_confirm_hits=3 makes a promotion need three hits.
 */
class Small_TrackerTentativeTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};
        std::vector<sushitrack_track_t> out_ = std::vector<sushitrack_track_t>(10);

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 1;
            params_.enable_tentative = 1;
            params_.tentative_confirm_hits = 3;
            params_.tentative_max_age_seconds = 0.08f;
            params_.track_buffer = 2;
            params_.frame_rate = 25;
            params_.tentative_match_thresh = 0.9f;
            params_.track_thresh = 0.1f;
            params_.high_thresh = 0.2f;
            params_.tentative_fast_thresh = 1.0f;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override { sushitrack_destroy(hdl_); }

        /** @brief Feeds a detection at (@p x, @p y) of 0.04 s and returns the track count. */
        int feed(float x, float y)
        {
            return update_frame(hdl_, make_det(x, y), out_.data(), 10, 0.04f);
        }

        /** @brief Feeds a frame without detections of @p dt seconds and returns the count. */
        int miss(float dt) { return update_frame(hdl_, nullptr, 0, out_.data(), 10, dt); }
};

/**
 * @brief The tentative fixture, run over the missed frames around the seconds budget.
 */
class Small_TrackerTentativeAgeOutTest : public Small_TrackerTentativeTest,
                                         public ::testing::WithParamInterface<MissRow>
{
};

/**
 * @brief In time-aware mode a tentative track is kept at a lost time up to the budget of 0.08 s
 *        and removed above it.
 *
 * 0.08 s in one miss and 0.04 + 0.04 s (an exact float doubling) equal the budget, so the track
 * stays Tentative (4); 0.04 + 0.05 s is 0.09 s, so it is Removed (3).
 */
TEST_P(Small_TrackerTentativeAgeOutTest, LostTimeAgainstTheSecondsBudgetDecidesTheState)
{
    ASSERT_EQ(feed(100.0f, 200.0f), 1);
    ASSERT_EQ(out_[0].state, K_TENTATIVE);
    const MissRow row = GetParam();

    int n = miss(row.first_dt);
    if (row.second_dt > 0.0f)
        n = miss(row.second_dt);

    ASSERT_EQ(n, 1);
    EXPECT_EQ(out_[0].state, row.state);
    EXPECT_FLOAT_EQ(out_[0].lost_time, row.lost_time);
}

INSTANTIATE_TEST_SUITE_P(Budget, Small_TrackerTentativeAgeOutTest,
                         ::testing::Values(MissRow{0.08f, 0.0f, K_TENTATIVE, 0.08f},
                                           MissRow{0.04f, 0.04f, K_TENTATIVE, 0.08f},
                                           MissRow{0.04f, 0.05f, 3, 0.09f}));

/**
 * @brief Fixture of a frame-counted tentative tracker: time_aware=0 and a budget of two missed
 *        frames, so the dt passed to the update has no effect.
 */
class Small_TrackerTentativeFrameBudgetTest : public ::testing::TestWithParam<FrameMissRow>
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        std::vector<sushitrack_track_t> out_ = std::vector<sushitrack_track_t>(10);

        void SetUp() override
        {
            sushitrack_params_t params = {};
            sushitrack_get_default_params(&params);
            params.time_aware = 0;
            params.enable_tentative = 1;
            params.tentative_confirm_hits = 3;
            params.tentative_max_miss_frames = 2;
            params.frame_rate = 25;
            params.tentative_match_thresh = 0.9f;
            params.track_thresh = 0.1f;
            params.high_thresh = 0.2f;
            params.tentative_fast_thresh = 1.0f;

            hdl_ = sushitrack_create(&params);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override { sushitrack_destroy(hdl_); }

        /** @brief Feeds @p misses empty frames of @p dt seconds and returns the last count. */
        int miss_frames(int misses, float dt)
        {
            int n = 0;
            for (int i = 0; i < misses; ++i)
                n = update_frame(hdl_, nullptr, 0, out_.data(), 10, dt);
            return n;
        }
};

/**
 * @brief In frame-counted mode a tentative track is kept for two missed frames and removed on the
 *        third, whatever dt the frames carry.
 *
 * aged_out is true when the frames since the last observation exceed tentative_max_miss_frames, 2.
 * Each missed frame adds one, so the third miss makes 3 > 2.
 */
TEST_P(Small_TrackerTentativeFrameBudgetTest, MissedFramesAgainstTheFrameBudgetDecideTheState)
{
    const sushitrack_object_t seed = make_det(100.0f, 200.0f);
    ASSERT_EQ(update_frame(hdl_, seed, out_.data(), 10, 0.04f), 1);
    ASSERT_EQ(out_[0].state, K_TENTATIVE);

    const int n = miss_frames(GetParam().misses, GetParam().dt);

    ASSERT_EQ(n, 1);
    EXPECT_EQ(out_[0].state, GetParam().state);
}

INSTANTIATE_TEST_SUITE_P(Budget, Small_TrackerTentativeFrameBudgetTest,
                         ::testing::Values(FrameMissRow{1, 0.05f, K_TENTATIVE},
                                           FrameMissRow{2, 0.50f, K_TENTATIVE},
                                           FrameMissRow{2, 0.01f, K_TENTATIVE},
                                           FrameMissRow{3, 0.01f, 3}, FrameMissRow{3, 0.50f, 3}));

/**
 * @brief A missed frame adds its dt to the lost time of a tentative track and leaves it
 *        Tentative with its single hit.
 */
TEST_F(Small_TrackerTentativeTest, MissedFrameAccumulatesLostTime)
{
    ASSERT_EQ(feed(100.0f, 200.0f), 1);
    ASSERT_EQ(out_[0].hits, 1);

    ASSERT_EQ(miss(0.04f), 1);

    EXPECT_EQ(out_[0].state, K_TENTATIVE);
    EXPECT_FLOAT_EQ(out_[0].lost_time, 0.04f);
}

/**
 * @brief A match after a missed frame resets the lost time to zero and adds a hit, leaving the
 *        track Tentative below the hit threshold.
 */
TEST_F(Small_TrackerTentativeTest, MatchAfterAMissResetsLostTimeAndAddsAHit)
{
    ASSERT_EQ(feed(100.0f, 200.0f), 1);
    ASSERT_EQ(miss(0.04f), 1);

    ASSERT_EQ(feed(100.0f, 200.0f), 1);

    EXPECT_EQ(out_[0].state, K_TENTATIVE);
    EXPECT_EQ(out_[0].hits, 2);
    EXPECT_FLOAT_EQ(out_[0].lost_time, 0.0f);
}

/**
 * @brief Two well-separated tentative tracks evolve independently per frame.
 *
 * Frame 1 creates two Tentative tracks. Frame 2 sends only the detection at x 100: that track
 * gains a hit and keeps a lost time of zero, while the track at x 700 keeps one hit and gains
 * 0.04 s of lost time, which is under the 0.08 s budget.
 */
TEST_F(Small_TrackerTentativeTest, TwoSimultaneousTentativeTracksIndependent)
{
    const sushitrack_object_t both[2] = {make_det(100.0f, 200.0f), make_det(700.0f, 500.0f)};
    ASSERT_EQ(update_frame(hdl_, both, 2, out_.data(), 10, 0.04f), 2);

    const int n = feed(100.0f, 200.0f);

    ASSERT_EQ(n, 2);
    const sushitrack_track_t* matched = nearest_x(out_, n, 100.0f);
    const sushitrack_track_t* unmatched = nearest_x(out_, n, 700.0f);
    ASSERT_NE(matched, nullptr);
    ASSERT_NE(unmatched, nullptr);
    EXPECT_EQ(matched->hits, 2);
    EXPECT_FLOAT_EQ(matched->lost_time, 0.0f);
    EXPECT_EQ(unmatched->hits, 1);
    EXPECT_FLOAT_EQ(unmatched->lost_time, 0.04f);
}

/**
 * @brief A track promoted to Tracked persists with the same track_id on
 *        subsequent matched frames and its hit counter continues to increment.
 */
TEST_F(Small_TrackerTentativeTest, TentativePromotedTrackPersistsAfterPromotion)
{
    feed(100.0f, 200.0f);
    feed(102.0f, 202.0f);
    int n = feed(104.0f, 204.0f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out_[0].state, K_TRACKED);
    const int64_t promoted_id = out_[0].track_id;

    n = feed(106.0f, 206.0f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out_[0].track_id, promoted_id);
    EXPECT_EQ(out_[0].state, K_TRACKED);
    EXPECT_EQ(out_[0].hits, 4);
}

/**
 * @brief The init-suppression fixture, run over the gate on and off.
 */
class Small_TrackerTentativeInitSuppressTest : public ::testing::TestWithParam<SuppressRow>
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;

        void SetUp() override
        {
            sushitrack_params_t params = suppress_params(GetParam().suppress_iou);
            hdl_ = sushitrack_create(&params);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override { sushitrack_destroy(hdl_); }
};

/**
 * @brief A detection overlapping a confirmed track starts a new tentative track only when the
 *        init-suppression gate is off.
 *
 * A frame after the confirmation carries a detection on the track and one 5 px right of it, which
 * stays unmatched and overlaps the track with an IoU near 0.82. At 0.7 it is dropped; at 2.0,
 * which no IoU exceeds, it becomes a Tentative track beside the confirmed one.
 */
TEST_P(Small_TrackerTentativeInitSuppressTest, GateDecidesWhetherTheOverlapStartsATrack)
{
    std::vector<sushitrack_track_t> out(10);
    ASSERT_NO_FATAL_FAILURE(confirm_track(hdl_, out));

    const sushitrack_object_t pair[2] = {make_det(106.0f, 206.0f), make_det(111.0f, 206.0f)};
    const int n = update_frame(hdl_, pair, 2, out.data(), 10, 0.04f);

    ASSERT_EQ(n, GetParam().tracks);
    EXPECT_EQ(count_in_state(out, n, K_TRACKED), GetParam().tracked);
    EXPECT_EQ(count_in_state(out, n, K_TENTATIVE), GetParam().tentative);
}

INSTANTIATE_TEST_SUITE_P(Gate, Small_TrackerTentativeInitSuppressTest,
                         ::testing::Values(SuppressRow{0.7f, 1, 1, 0}, SuppressRow{2.0f, 2, 1, 1}));

/**
 * @brief Tracker fixture with tentative mode disabled.
 *
 * All parameters match Small_TrackerTentativeTest except enable_tentative=0.
 * New tracks must enter Tracked state immediately on the first detection.
 */
class Small_TrackerTentativeDisabledTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 1;
            params_.enable_tentative = 0;
            params_.tentative_confirm_hits = 3;
            params_.tentative_max_age_seconds = 0.08f;
            params_.track_buffer = 2;
            params_.frame_rate = 25;
            params_.tentative_match_thresh = 0.9f;
            params_.track_thresh = 0.1f;
            params_.high_thresh = 0.2f;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override { sushitrack_destroy(hdl_); }
};

/**
 * @brief With tentative mode disabled, the first detection immediately produces
 *        a Tracked, confirmed track without requiring additional hits.
 */
TEST_F(Small_TrackerTentativeDisabledTest, WithTentativeDisabledNewTrackIsImmediatelyTracked)
{
    std::vector<sushitrack_track_t> out(10);

    const int n = update_frame(hdl_, make_det(100.0f, 200.0f), out.data(), 10, 0.04f);

    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, K_TRACKED);
    EXPECT_EQ(out[0].is_confirmed, 1);
}

/**
 * @brief A tentative track confirmed by a low-score detection logs the same DEBUG a high-score
 *        confirmation does.
 */
TEST(Small_TrackerTentativeRecordsTest, LowPoolConfirmationLogsTheConfirmation)
{
    SushiTrack::CapturedLog log(SushiLog::LogLevel::Debug);
    sushitrack_params_t p = {};
    sushitrack_get_default_params(&p);
    p.enable_tentative = 1;
    p.time_aware = 0;
    p.track_thresh = 0.3f;
    p.high_thresh = 0.5f;
    p.tentative_fast_thresh = 0.05f;
    p.tentative_fast_hits = 1;
    p.tentative_match_thresh = 0.9f;
    sushitrack_tracker_t hdl = sushitrack_create(&p);
    ASSERT_NE(hdl, nullptr);
    std::vector<sushitrack_track_t> out(4);

    const sushitrack_object_t high = make_det(100.0f, 200.0f);
    update_frame(hdl, high, out.data(), 4, 0.04f);
    const sushitrack_object_t low = make_det(100.0f, 200.0f, 0.1f);
    update_frame(hdl, low, out.data(), 4, 0.04f);
    sushitrack_destroy(hdl);

    EXPECT_EQ(log.count("tentative track 1 confirmed"), 1);
}

/**
 * @brief The fixture of the lifecycle records: one track confirmed over three frames with the
 *        DEBUG records of the run captured.
 */
class Small_TrackerTentativeLifecycleRecordsTest : public ::testing::Test
{
    protected:
        SushiTrack::CapturedLog log_{SushiLog::LogLevel::Debug};
        LifecycleCounts counts_;

        void SetUp() override
        {
            sushitrack_params_t p = suppress_params(0.7f);
            sushitrack_tracker_t hdl = sushitrack_create(&p);
            ASSERT_NE(hdl, nullptr);
            std::vector<sushitrack_track_t> out(10);
            ASSERT_NO_FATAL_FAILURE(confirm_track(hdl, out));
            sushitrack_destroy(hdl);
            counts_ = count_lifecycle_records(log_);
        }
};

/**
 * @brief The run writes lifecycle DEBUG records at all.
 */
TEST_F(Small_TrackerTentativeLifecycleRecordsTest, ConfirmationWritesLifecycleRecords)
{
    EXPECT_GT(counts_.total, 0);
}

/**
 * @brief Every DEBUG record one update makes starts with that update's frame number.
 */
TEST_F(Small_TrackerTentativeLifecycleRecordsTest, EveryLifecycleRecordCarriesItsFrame)
{
    EXPECT_EQ(counts_.without_frame_prefix, 0);
}
