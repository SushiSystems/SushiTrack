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
 * @file test_full_stack.cpp
 * @brief Tests the tracker with every feature switched on.
 * @author Mustafa Garip
 */

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/c_api/sushitrack_c.h>

#include <c_detection_builders.hpp>

using namespace CDetections;

namespace
{
    /** @brief Lane split of the two-object scene: A runs above this y, B below it. */
    constexpr float K_LANE_SPLIT = 275.0f;

    /** @brief Returns the parameters of the full stack: the defaults with every refinement on. */
    sushitrack_params_t full_stack_params()
    {
        sushitrack_params_t params = {};
        sushitrack_get_default_params(&params);
        params.time_aware = 1;
        params.enable_reid = 1;
        params.enable_mahalanobis = 1;
        params.enable_tentative = 1;
        params.kalman_type = 2;
        params.primary_assoc_mode = 1;
        params.frame_rate = 25;
        params.track_buffer = 30;
        params.track_thresh = 0.2f;
        params.high_thresh = 0.4f;
        params.match_thresh = 0.9f;
        params.tentative_fast_thresh = 0.85f;
        params.tentative_fast_hits = 1;
        return params;
    }
} // namespace

/**
 * @brief Fixture of the full stack: time aware, ReID, Mahalanobis gating, tentative confirmation
 *        and the IMM filter, over two objects of orthogonal 128 dimensional features.
 *
 * Object A runs along y 150 and object B along y 400, both at 6 px per frame from x 100. The
 * steps of the scene are methods, so that each test reads one promise.
 */
class Small_TrackerFullStackTest : public ::testing::Test
{
    protected:
        static constexpr int K_DIM = 128;
        static constexpr float K_AY = 150.0f;
        static constexpr float K_BY = 400.0f;
        static constexpr float K_VX = 6.0f;

        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};
        std::vector<float> feat_a_;
        std::vector<float> feat_b_;
        std::vector<sushitrack_track_t> out_ = std::vector<sushitrack_track_t>(8);
        int last_count_ = 0;
        float ax_ = 100.0f;
        float bx_ = 100.0f;
        int64_t id_a_ = -1;
        int64_t id_b_ = -1;
        int id_changes_ = 0;
        int insane_tracks_ = 0;

        void SetUp() override
        {
            params_ = full_stack_params();
            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);

            feat_a_ = one_hot_feature(K_DIM, 0);
            feat_b_ = one_hot_feature(K_DIM, 1);
        }

        void TearDown() override
        {
            if (hdl_)
                sushitrack_destroy(hdl_);
        }

        /**
         * @brief Feeds both objects at their current x, moves both on by one step and returns
         *        the track count; the lane ids and the counters follow what the frame showed.
         */
        int step()
        {
            const sushitrack_object_t pair[2] = {
                make_det_with_feat(ax_, K_AY, feat_a_.data(), K_DIM, 0.9f),
                make_det_with_feat(bx_, K_BY, feat_b_.data(), K_DIM, 0.9f)};
            last_count_ = update_frame(hdl_, pair, 2, out_.data(), 8, 0.04f);
            ax_ += K_VX;
            bx_ += K_VX;
            read_lanes(last_count_);
            return last_count_;
        }

        /**
         * @brief Feeds seven frames of the pair, so that both tracks are past their confirmation.
         */
        void settle()
        {
            for (int frame = 0; frame < 7; ++frame)
                ASSERT_GE(step(), 2) << "frame " << frame;
            ASSERT_NE(id_a_, -1);
            ASSERT_NE(id_b_, -1);
            ASSERT_NE(id_a_, id_b_);
        }

        /**
         * @brief Feeds @p frames frames in which only B is visible and A keeps its pace unseen,
         *        and returns how many of them did not show B as a Tracked track under its id.
         */
        int occlude_a(int frames)
        {
            int frames_without_b = 0;
            for (int i = 0; i < frames; ++i)
            {
                const sushitrack_object_t b =
                    make_det_with_feat(bx_, K_BY, feat_b_.data(), K_DIM, 0.9f);
                last_count_ = update_frame(hdl_, &b, 1, out_.data(), 8, 0.04f);
                const sushitrack_track_t* track = find_by_id(out_, last_count_, id_b_);
                if (track == nullptr || track->state != K_TRACKED)
                    ++frames_without_b;
                ax_ += K_VX;
                bx_ += K_VX;
            }
            return frames_without_b;
        }

    private:
        /**
         * @brief Assigns each track of the frame to its lane by y, counts a lane whose id
         *        changed and the tracks whose box is not finite with a positive extent.
         */
        void read_lanes(int n)
        {
            for (int k = 0; k < n; ++k)
            {
                const sushitrack_track_t& track = out_[static_cast<std::size_t>(k)];
                if (!geometry_is_sane(track))
                    ++insane_tracks_;
                int64_t& lane_id = track.y < K_LANE_SPLIT ? id_a_ : id_b_;
                if (lane_id != -1 && lane_id != track.track_id)
                    ++id_changes_;
                lane_id = track.track_id;
            }
        }
};

/**
 * @brief Both objects are confirmed Tracked after the settling frames.
 *
 * Both detections score 0.9, at least tentative_fast_thresh 0.85, so the requirement is
 * tentative_fast_hits, 1. Frame 1 seeds two tentative tracks, and the 6 px shift of frame 2
 * keeps the IoU cost near 0.21, under tentative_match_thresh 0.3, so both are confirmed there.
 */
TEST_F(Small_TrackerFullStackTest, BothObjectsAreConfirmedTracked)
{
    ASSERT_NO_FATAL_FAILURE(settle());

    const sushitrack_track_t* a = find_by_id(out_, last_count_, id_a_);
    const sushitrack_track_t* b = find_by_id(out_, last_count_, id_b_);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(a->state, K_TRACKED);
    EXPECT_EQ(b->state, K_TRACKED);
    EXPECT_EQ(a->is_confirmed, 1);
    EXPECT_EQ(b->is_confirmed, 1);
}

/**
 * @brief Each lane keeps the id its track got when it was seeded while the objects move.
 */
TEST_F(Small_TrackerFullStackTest, EachLaneKeepsItsIdWhileTheObjectsMove)
{
    ASSERT_NO_FATAL_FAILURE(settle());

    EXPECT_EQ(id_changes_, 0);
}

/**
 * @brief Every reported box is finite with a positive extent while the objects move.
 */
TEST_F(Small_TrackerFullStackTest, GeometryStaysSaneWhileTheObjectsMove)
{
    ASSERT_NO_FATAL_FAILURE(settle());

    EXPECT_EQ(insane_tracks_, 0);
}

/**
 * @brief The visible object keeps its id and its Tracked state while the other is occluded.
 */
TEST_F(Small_TrackerFullStackTest, VisibleObjectKeepsItsIdDuringTheOcclusion)
{
    ASSERT_NO_FATAL_FAILURE(settle());

    EXPECT_EQ(occlude_a(3), 0);
}

/**
 * @brief The occluded object returns on its own lane under its original id.
 *
 * After three unseen frames the track is Lost and in the lost pool. The reappearing detection
 * carries the same feature, so the visual recovery costs 1 - 1 = 0, under
 * reid_recovery_thresh 0.4, when the detection lies inside the gate of the track.
 */
TEST_F(Small_TrackerFullStackTest, OccludedObjectIsRecoveredByItsFeature)
{
    ASSERT_NO_FATAL_FAILURE(settle());
    ASSERT_EQ(occlude_a(3), 0);

    step();

    const sushitrack_track_t* recovered = find_by_id(out_, last_count_, id_a_);
    ASSERT_NE(recovered, nullptr) << "ReID recovery did not restore the id of object A";
    EXPECT_EQ(recovered->state, K_TRACKED);
    EXPECT_LT(recovered->y, K_LANE_SPLIT) << "the recovered track surfaced in the wrong lane";
    EXPECT_TRUE(geometry_is_sane(*recovered));
    EXPECT_NE(find_by_id(out_, last_count_, id_b_), nullptr)
        << "object B lost its id at the reappearance";
}

namespace
{
/**
 * @brief What the long run showed: the id the track confirmed with and the frames that broke a promise.
 */
struct LongRunOutcome
{
        bool built = false;
        int64_t confirmed_id = -1;
        int frames_without_confirmed_id = 0;
        int frames_with_bad_geometry = 0;
};

/**
 * @brief Feeds a single object that accelerates and weaves for 120 frames at a score that swings
 *        between 0.4 and 0.9, and returns what the run showed.
 *
 * The first frame seeds a tentative track. Once a frame reports the track Tracked and confirmed,
 * its id is frozen and every later frame must show that id.
 */
LongRunOutcome run_long_sequence()
{
    LongRunOutcome outcome;
    sushitrack_params_t params = full_stack_params();
    sushitrack_tracker_t hdl = sushitrack_create(&params);
    if (hdl == nullptr)
        return outcome;
    outcome.built = true;
    const std::vector<float> feat = one_hot_feature(128, 0);
    std::vector<sushitrack_track_t> out(4);
    const sushitrack_object_t seed = make_det_with_feat(100.0f, 200.0f, feat.data(), 128, 0.95f);
    if (update_frame(hdl, seed, out.data(), 4, 0.04f) < 1)
        outcome.built = false;

    float x = 100.0f;
    float y = 200.0f;
    for (int k = 1; outcome.built && k <= 120; ++k)
    {
        const float frame = static_cast<float>(k);
        x += 2.0f + 0.02f * frame;
        y += 4.0f * std::sin(0.1f * frame);
        const float score = 0.4f + 0.5f * std::abs(std::sin(0.3f * frame));
        const sushitrack_object_t detection = make_det_with_feat(x, y, feat.data(), 128, score);
        const int n = update_frame(hdl, detection, out.data(), 4, 0.04f);
        if (n < 1)
            continue;
        if (outcome.confirmed_id == -1)
        {
            if (out[0].state == K_TRACKED && out[0].is_confirmed == 1)
                outcome.confirmed_id = out[0].track_id;
            continue;
        }
        const sushitrack_track_t* track = find_by_id(out, n, outcome.confirmed_id);
        if (track == nullptr)
            ++outcome.frames_without_confirmed_id;
        else if (!geometry_is_sane(*track))
            ++outcome.frames_with_bad_geometry;
    }
    sushitrack_destroy(hdl);
    return outcome;
}
} // namespace

/**
 * @brief Fixture of the long run; the sequence runs once for the whole suite, not once per test.
 */
class Small_TrackerFullStackLongRunTest : public ::testing::Test
{
    protected:
        static void SetUpTestSuite()
        {
            outcome_ = run_long_sequence();
        }

        static LongRunOutcome outcome_;
};

LongRunOutcome Small_TrackerFullStackLongRunTest::outcome_;

/**
 * @brief The track becomes Tracked and confirmed at some frame of the run.
 */
TEST_F(Small_TrackerFullStackLongRunTest, TrackConfirmsDuringTheRun)
{
    ASSERT_TRUE(outcome_.built);

    EXPECT_NE(outcome_.confirmed_id, -1);
}

/**
 * @brief After its confirmation the track shows its id on every frame, so no swap or re-seed
 *        hides behind a re-anchored id.
 */
TEST_F(Small_TrackerFullStackLongRunTest, ConfirmedIdIsShownOnEveryLaterFrame)
{
    ASSERT_NE(outcome_.confirmed_id, -1);

    EXPECT_EQ(outcome_.frames_without_confirmed_id, 0);
}

/**
 * @brief After its confirmation the track's box stays finite with a positive extent.
 */
TEST_F(Small_TrackerFullStackLongRunTest, ConfirmedTrackGeometryStaysSane)
{
    ASSERT_NE(outcome_.confirmed_id, -1);

    EXPECT_EQ(outcome_.frames_with_bad_geometry, 0);
}
