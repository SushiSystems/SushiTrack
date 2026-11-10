/**************************************************************************/
/* test_end_to_end.cpp                                                    */
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
 * @file test_end_to_end.cpp
 * @brief Tests the tracker end to end through the C API.
 * @author Mustafa Garip
 */

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/c_api/sushitrack_c.h>

#include <c_detection_builders.hpp>

using namespace CDetections;

namespace
{
    /**
     * @brief Returns the id of the track among the first @p n of @p out whose x is closest to
     *        @p x, or -1 when @p n is zero.
     */
    int64_t id_of_nearest_x(const std::vector<sushitrack_track_t>& out, int n, float x)
    {
        int64_t best_id = -1;
        float best_distance = std::numeric_limits<float>::max();
        for (int i = 0; i < n; ++i)
        {
            const float distance = std::abs(out[static_cast<std::size_t>(i)].x - x);
            if (distance < best_distance)
            {
                best_distance = distance;
                best_id = out[static_cast<std::size_t>(i)].track_id;
            }
        }
        return best_id;
    }

    /**
     * @brief What one pass over the long sequence found wrong, one counter per promise.
     */
    struct SequenceReport
    {
            int frames_without_the_id = 0;
            int frames_with_wrong_age = 0;
            int frames_with_bad_geometry = 0;
            int frames_with_update_gap = 0;
    };
} // namespace

/**
 * @brief Fixture of a long single-object trajectory: it runs the whole sequence once and keeps
 *        what each frame showed, so that each test reads one promise.
 */
class Small_TrackerLongSequenceTrackingTest : public ::testing::Test
{
    protected:
        static constexpr int K_FRAMES = 300;

        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};
        SequenceReport report_;

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 1;
            params_.enable_tentative = 0;
            params_.primary_assoc_mode = 1;
            params_.frame_rate = 30;
            params_.track_buffer = 60;
            params_.track_thresh = 0.1f;
            params_.high_thresh = 0.2f;
            params_.match_thresh = 0.9f;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);
            run_sequence();
        }

        void TearDown() override { sushitrack_destroy(hdl_); }

    private:
        /**
         * @brief Feeds 300 frames of a box moving (1.5, 0.5) px per frame and fills report_.
         *
         * The first frame seeds the track. Every later frame is a match, so the track length is
         * the frame number plus one (the tracker numbers frames from 1 and the track starts at
         * frame 1) and the last update is the current frame.
         */
        void run_sequence()
        {
            std::vector<sushitrack_track_t> out(8);
            const sushitrack_object_t first = make_det(100.0f, 200.0f);
            ASSERT_EQ(update_frame(hdl_, first, out.data(), 8, 1.0f / 30.0f), 1);
            const int64_t id = out[0].track_id;

            for (int frame = 1; frame < K_FRAMES; ++frame)
            {
                const sushitrack_object_t detection =
                    make_det(100.0f + 1.5f * static_cast<float>(frame),
                             200.0f + 0.5f * static_cast<float>(frame));
                const int n = update_frame(hdl_, detection, out.data(), 8, 1.0f / 30.0f);
                const sushitrack_track_t* track = find_by_id(out, n, id);
                if (track == nullptr)
                {
                    ++report_.frames_without_the_id;
                    continue;
                }
                if (track->age != frame + 1)
                    ++report_.frames_with_wrong_age;
                if (!geometry_is_sane(*track))
                    ++report_.frames_with_bad_geometry;
                if (track->frames_since_last_update != 0)
                    ++report_.frames_with_update_gap;
            }
        }
};

/**
 * @brief The seeded track is present under its id on each of the 299 later frames.
 */
TEST_F(Small_TrackerLongSequenceTrackingTest, SingleObjectKeepsItsIdOverThreeHundredFrames)
{
    EXPECT_EQ(report_.frames_without_the_id, 0);
}

/**
 * @brief The reported age is the number of frames the object was seen, so it never regresses.
 */
TEST_F(Small_TrackerLongSequenceTrackingTest, AgeCountsEveryObservedFrame)
{
    EXPECT_EQ(report_.frames_with_wrong_age, 0);
}

/**
 * @brief The reported box stays finite with a positive extent on every frame.
 */
TEST_F(Small_TrackerLongSequenceTrackingTest, GeometryStaysFiniteAndPositive)
{
    EXPECT_EQ(report_.frames_with_bad_geometry, 0);
}

/**
 * @brief A track observed on every frame reports no frames since its last update.
 */
TEST_F(Small_TrackerLongSequenceTrackingTest, ObservedTrackReportsNoUpdateGap)
{
    EXPECT_EQ(report_.frames_with_update_gap, 0);
}

/**
 * @brief Fixture for full-pipeline ReID validation.
 *
 * Two objects carry orthogonal unit features of 16 dimensions, A on axis 0 and B on axis 1, so
 * the cosine similarity between them is exactly zero.
 */
class Small_TrackerReIDEndToEndTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};
        std::vector<float> feat_a_;
        std::vector<float> feat_b_;

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 1;
            params_.enable_tentative = 0;
            params_.enable_reid = 1;
            params_.primary_assoc_mode = 1;
            params_.frame_rate = 25;
            params_.track_buffer = 30;
            params_.track_thresh = 0.1f;
            params_.high_thresh = 0.2f;
            params_.match_thresh = 0.95f;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);

            feat_a_ = one_hot_feature(16, 0);
            feat_b_ = one_hot_feature(16, 1);
        }

        void TearDown() override { sushitrack_destroy(hdl_); }

        /**
         * @brief Feeds one frame of object A at @p ax and object B at @p bx, both at y 200.
         */
        int feed_pair(std::vector<sushitrack_track_t>& out, float ax, float bx)
        {
            const sushitrack_object_t pair[2] = {
                make_det_with_feat(ax, 200.0f, feat_a_.data(), 16, 0.95f),
                make_det_with_feat(bx, 200.0f, feat_b_.data(), 16, 0.95f)};
            return update_frame(hdl_, pair, 2, out.data(), 8, 0.04f);
        }

        /**
         * @brief Drives A from x 100 to 340 and B from x 400 to 160 over six frames, 40 px per
         *        frame, and returns the track count of the last frame.
         *
         * The boxes are 50 px wide, so they overlap in frames 4 and 5 (A at 260 and 300, B at
         * 240 and 200) and have passed each other by frame 6.
         */
        int cross_over(std::vector<sushitrack_track_t>& out)
        {
            int n = 0;
            for (int k = 1; k <= 6; ++k)
                n = feed_pair(out, 100.0f + 40.0f * static_cast<float>(k),
                              400.0f - 40.0f * static_cast<float>(k));
            return n;
        }

        /**
         * @brief Feeds one detection of object A and then @p missed empty frames, all of 0.04 s.
         */
        void feed_then_miss(std::vector<sushitrack_track_t>& out, int missed)
        {
            const sushitrack_object_t first =
                make_det_with_feat(100.0f, 200.0f, feat_a_.data(), 16, 0.95f);
            update_frame(hdl_, first, out.data(), 8, 0.04f);
            for (int i = 0; i < missed; ++i)
                update_frame(hdl_, nullptr, 0, out.data(), 8, 0.04f);
        }
};

/**
 * @brief Two objects of different appearance keep their ids through a crossing.
 *
 * A track and a detection of the same feature cost 1 - 1 * 0.95 = 0.05 after score fusion,
 * wherever the boxes are; a pair of different features costs at least 0.7 (1 - 0.3 * 0.95
 * = 0.715), so the cheapest assignment pairs each track with its own feature. After the
 * crossing A is on the right, so the id seeded at x 100 must report the larger x.
 */
TEST_F(Small_TrackerReIDEndToEndTest, EachIdFollowsItsOwnFeatureAcrossACrossing)
{
    std::vector<sushitrack_track_t> out(8);
    ASSERT_EQ(feed_pair(out, 100.0f, 400.0f), 2);
    const int64_t id_a = id_of_nearest_x(out, 2, 100.0f);
    const int64_t id_b = id_of_nearest_x(out, 2, 400.0f);
    ASSERT_NE(id_a, id_b);

    const int n = cross_over(out);

    ASSERT_EQ(n, 2);
    const sushitrack_track_t* a = find_by_id(out, n, id_a);
    const sushitrack_track_t* b = find_by_id(out, n, id_b);
    ASSERT_NE(a, nullptr) << "track A lost its id across the crossing";
    ASSERT_NE(b, nullptr) << "track B lost its id across the crossing";
    EXPECT_GT(a->x, b->x) << "the ids swapped places at the crossing";
}

namespace
{
    /**
     * @brief The number of frames a track is missing in a row.
     */
    struct MissRow
    {
            int missed_frames;
    };
} // namespace

/**
 * @brief The ReID fixture, run over gaps of different length.
 */
class Small_TrackerReIDRecoveryTest : public Small_TrackerReIDEndToEndTest,
                                      public ::testing::WithParamInterface<MissRow>
{
};

/**
 * @brief A track Lost for a few frames comes back under its original id, Tracked and with its
 *        lost time reset, when the same feature reappears 60 px right and 20 px down.
 *
 * The timeout is 30 / 25 = 1.2 s. The first missed frame marks the track Lost at 0.04 s and
 * each later one adds 0.04 s, so 20 missed frames hold 0.80 s. Visual recovery costs 1 - 1 =
 * 0, below reid_recovery_thresh 0.4, wherever the detection is.
 */
TEST_P(Small_TrackerReIDRecoveryTest, LostTrackIsRecoveredByMatchingFeature)
{
    std::vector<sushitrack_track_t> out(8);
    feed_then_miss(out, GetParam().missed_frames);
    ASSERT_EQ(read_tracklets(hdl_, out.data(), 8), 1);
    ASSERT_EQ(out[0].state, K_LOST);
    const int64_t original_id = out[0].track_id;

    const sushitrack_object_t back =
        make_det_with_feat(160.0f, 220.0f, feat_a_.data(), 16, 0.95f);
    const int n = update_frame(hdl_, back, out.data(), 8, 0.04f);

    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].track_id, original_id);
    EXPECT_EQ(out[0].state, K_TRACKED);
    EXPECT_FLOAT_EQ(out[0].lost_time, 0.0f);
}

INSTANTIATE_TEST_SUITE_P(Gaps, Small_TrackerReIDRecoveryTest,
                         ::testing::Values(MissRow{1}, MissRow{5}, MissRow{20}));

/**
 * @brief Fixture exercising tentative-stage promotion under ReID.
 */
class Small_TrackerTentativeWithReIDTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};
        std::vector<float> feat_;

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 1;
            params_.enable_tentative = 1;
            params_.enable_reid = 1;
            params_.tentative_confirm_hits = 3;
            params_.frame_rate = 25;
            params_.track_buffer = 30;
            params_.track_thresh = 0.1f;
            params_.high_thresh = 0.2f;
            params_.match_thresh = 0.9f;
            params_.tentative_match_thresh = 0.95f;
            params_.tentative_fast_thresh = 0.99f;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);

            feat_ = one_hot_feature(16, 3);
        }

        void TearDown() override { sushitrack_destroy(hdl_); }

        /**
         * @brief Feeds @p hits frames of a score 0.7 detection that moves (1, 1) px per frame
         *        and returns the track count of the last one.
         */
        int feed_hits(std::vector<sushitrack_track_t>& out, int hits)
        {
            int n = 0;
            for (int i = 0; i < hits; ++i)
            {
                const sushitrack_object_t detection =
                    make_det_with_feat(100.0f + static_cast<float>(i),
                                       200.0f + static_cast<float>(i), feat_.data(), 16, 0.7f);
                n = update_frame(hdl_, detection, out.data(), 4, 0.04f);
            }
            return n;
        }
};

namespace
{
    /**
     * @brief The number of matched frames, and the state and confirmation flag they leave.
     */
    struct HitRow
    {
            int hits;
            int state;
            int is_confirmed;
    };
} // namespace

/**
 * @brief The tentative fixture, run over the hit counts on both sides of the confirmation.
 */
class Small_TrackerTentativeConfirmationTest : public Small_TrackerTentativeWithReIDTest,
                                               public ::testing::WithParamInterface<HitRow>
{
};

/**
 * @brief A tentative track with features confirms at its third hit, and not before.
 *
 * Score 0.7 lies between high_thresh 0.2 and tentative_fast_thresh 0.99, so the requirement is
 * tentative_confirm_hits, 3. Frame 1 seeds the track with one hit, frame 2 makes two (still
 * Tentative) and frame 3 makes three and confirms it. A 1 px shift keeps the IoU cost under
 * tentative_match_thresh 0.95.
 */
TEST_P(Small_TrackerTentativeConfirmationTest, ConfirmsAtTheConfiguredHitCount)
{
    std::vector<sushitrack_track_t> out(4);

    const int n = feed_hits(out, GetParam().hits);

    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, GetParam().state);
    EXPECT_EQ(out[0].is_confirmed, GetParam().is_confirmed);
}

INSTANTIATE_TEST_SUITE_P(Hits, Small_TrackerTentativeConfirmationTest,
                         ::testing::Values(HitRow{2, K_TENTATIVE, 0}, HitRow{3, K_TRACKED, 1}));

/**
 * @brief A tentative track that misses a single frame and re-acquires retains
 *        its identity rather than spawning a fresh tentative slot.
 */
TEST_F(Small_TrackerTentativeWithReIDTest, TentativeReAcquiresAfterSingleMissedFrame)
{
    std::vector<sushitrack_track_t> out(4);

    const sushitrack_object_t d0 = make_det_with_feat(100.0f, 200.0f, feat_.data(), 16, 0.7f);
    const int n0 = update_frame(hdl_, d0, out.data(), 4, 0.04f);
    ASSERT_GE(n0, 1);
    const int64_t tentative_id = out[0].track_id;

    update_frame(hdl_, nullptr, 0, out.data(), 4, 0.04f);

    const sushitrack_object_t d2 = make_det_with_feat(102.0f, 202.0f, feat_.data(), 16, 0.7f);
    const int n2 = update_frame(hdl_, d2, out.data(), 4, 0.04f);
    ASSERT_GE(n2, 1);

    EXPECT_NE(find_by_id(out, n2, tentative_id), nullptr)
        << "Tentative track lost its ID across a single missed frame.";
}

/**
 * @brief Fixture combining the IMM regime with Mahalanobis gating end-to-end.
 */
class Small_TrackerImmGatingEndToEndTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 1;
            params_.enable_tentative = 0;
            params_.kalman_type = 2;
            params_.enable_mahalanobis = 1;
            params_.primary_assoc_mode = 1;
            params_.frame_rate = 25;
            params_.track_buffer = 30;
            params_.track_thresh = 0.1f;
            params_.high_thresh = 0.2f;
            params_.match_thresh = 0.9f;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override { sushitrack_destroy(hdl_); }

        /**
         * @brief Feeds a box that moves (2, 1) px per frame for 15 frames after its seed and
         *        returns how many of those frames did not show the seeded id.
         */
        int frames_without_the_id(std::vector<sushitrack_track_t>& out, int64_t id)
        {
            int missing = 0;
            for (int k = 1; k <= 15; ++k)
            {
                const sushitrack_object_t detection = make_det(
                    100.0f + 2.0f * static_cast<float>(k), 200.0f + static_cast<float>(k));
                const int n = update_frame(hdl_, detection, out.data(), 4, 0.04f);
                if (find_by_id(out, n, id) == nullptr)
                    ++missing;
            }
            return missing;
        }
};

/**
 * @brief A track moving at a steady (2, 1) px per frame keeps its id on every frame under the
 *        IMM filter with Mahalanobis gating, since each detection lies within a few pixels of
 *        the constant-velocity prediction.
 */
TEST_F(Small_TrackerImmGatingEndToEndTest, SteadyMotionKeepsItsIdInsideTheGate)
{
    std::vector<sushitrack_track_t> out(4);
    const sushitrack_object_t seed = make_det(100.0f, 200.0f);
    ASSERT_EQ(update_frame(hdl_, seed, out.data(), 4, 0.04f), 1);
    ASSERT_EQ(out[0].state, K_TRACKED);

    EXPECT_EQ(frames_without_the_id(out, out[0].track_id), 0);
}

/**
 * @brief A detection 5000 px away does not bind the established track: the track turns Lost
 *        under its own id and the far detection starts a track of its own.
 *
 * The far box shares no area with the track, so its cost is at least 1, above match_thresh 0.9.
 * The unmatched track is marked Lost; the unmatched detection scores 0.9, above high_thresh
 * 0.2, and seeds the second track, which sorts first as the Tracked one.
 */
TEST_F(Small_TrackerImmGatingEndToEndTest, OutOfGateDetectionLeavesTheEstablishedTrackLost)
{
    std::vector<sushitrack_track_t> out(4);
    const sushitrack_object_t seed = make_det(100.0f, 200.0f);
    ASSERT_EQ(update_frame(hdl_, seed, out.data(), 4, 0.04f), 1);
    ASSERT_EQ(out[0].state, K_TRACKED);
    const int64_t established_id = out[0].track_id;

    const sushitrack_object_t far_away = make_det(5100.0f, 5200.0f);
    const int n = update_frame(hdl_, far_away, out.data(), 4, 0.04f);

    ASSERT_EQ(n, 2);
    const sushitrack_track_t* established = find_by_id(out, n, established_id);
    ASSERT_NE(established, nullptr);
    EXPECT_EQ(established->state, K_LOST);
    EXPECT_NE(out[0].track_id, established_id) << "the far detection joined the established id";
}

/**
 * @brief Fixture of a time-aware tracker with a one-second buffer: 30 frames at 30 fps.
 */
class Small_TrackerVariableDtTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 1;
            params_.enable_tentative = 0;
            params_.frame_rate = 30;
            params_.track_buffer = 30;
            params_.track_thresh = 0.1f;
            params_.high_thresh = 0.2f;
            params_.match_thresh = 0.95f;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override { sushitrack_destroy(hdl_); }
};

/**
 * @brief A one-second gap leaves the track Lost with a finite box and a lost time of one second.
 *
 * The empty frame marks the track Lost with a lost time of exactly 1.0. cleanup_lost looks at
 * the lost list of the frame before, so the track is first checked on the next frame; its
 * 1.0 s equals the timeout 30 / 30 and the policy retires only above timeout + 1e-4.
 * The prediction step is capped at 5 steps, which keeps the box finite.
 */
TEST_F(Small_TrackerVariableDtTest, OneSecondGapLeavesTheTrackLost)
{
    std::vector<sushitrack_track_t> out(4);
    const sushitrack_object_t seed = make_det(100.0f, 200.0f);
    ASSERT_EQ(update_frame(hdl_, seed, out.data(), 4, 1.0f / 30.0f), 1);
    ASSERT_EQ(out[0].state, K_TRACKED);
    const int64_t id = out[0].track_id;

    const int n = update_frame(hdl_, nullptr, 0, out.data(), 4, 1.0f);

    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].track_id, id);
    EXPECT_EQ(out[0].state, K_LOST);
    EXPECT_FLOAT_EQ(out[0].lost_time, 1.0f);
    EXPECT_TRUE(geometry_is_sane(out[0]));
}
