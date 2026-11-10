/**************************************************************************/
/* test_multi_object.cpp                                                  */
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
 * @file test_multi_object.cpp
 * @brief Tests the tracker on several objects at once.
 * @author Mustafa Garip
 */

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
    /** @brief Distance between neighbouring objects of a scene, wide enough to be unambiguous. */
    constexpr float K_SPACING = 200.0f;

    /** @brief Columns and rows of a grid of objects. */
    struct GridRow
    {
            int cols;
            int rows;
    };

    /**
     * @brief Builds the detections of a grid whose objects drift (3, 1) px per frame.
     * @param frame Frame index, from 0.
     */
    std::vector<sushitrack_object_t> grid_frame(int cols, int rows, int frame)
    {
        std::vector<sushitrack_object_t> detections;
        detections.reserve(static_cast<std::size_t>(cols * rows));
        for (int r = 0; r < rows; ++r)
        {
            for (int c = 0; c < cols; ++c)
            {
                detections.push_back(make_det(
                    100.0f + static_cast<float>(c) * K_SPACING + 3.0f * static_cast<float>(frame),
                    100.0f + static_cast<float>(r) * K_SPACING + static_cast<float>(frame)));
            }
        }
        return detections;
    }

    /**
     * @brief Builds the detections of a row of objects at y 200 that drift 2 px per frame.
     * @param count Number of objects, 200 px apart from x 100.
     * @param visible Number of leading objects the frame contains.
     */
    std::vector<sushitrack_object_t> row_frame(int count, int visible, int frame)
    {
        std::vector<sushitrack_object_t> detections;
        for (int i = 0; i < count && i < visible; ++i)
        {
            detections.push_back(make_det(
                100.0f + static_cast<float>(i) * K_SPACING + 2.0f * static_cast<float>(frame),
                200.0f));
        }
        return detections;
    }
} // namespace

/**
 * @brief Frame-counted core fixture of the multi-object scenes.
 */
class Small_TrackerMultiObjectTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 0;
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
};

/**
 * @brief The multi-object fixture, run over grids of different size.
 */
class Small_TrackerSeedFrameTest : public Small_TrackerMultiObjectTest,
                                   public ::testing::WithParamInterface<GridRow>
{
};

/**
 * @brief The first frame of N well-separated objects produces N tracks.
 */
TEST_P(Small_TrackerSeedFrameTest, EachObjectSeedsOneTrack)
{
    const GridRow row = GetParam();
    const int count = row.cols * row.rows;
    std::vector<sushitrack_track_t> out(static_cast<std::size_t>(count) + 8);
    const std::vector<sushitrack_object_t> seed = grid_frame(row.cols, row.rows, 0);

    const int n = update_frame(hdl_, seed.data(), count, out.data(), static_cast<int>(out.size()),
                               0.04f);

    EXPECT_EQ(n, count);
}

INSTANTIATE_TEST_SUITE_P(Grids, Small_TrackerSeedFrameTest,
                         ::testing::Values(GridRow{1, 1}, GridRow{5, 1}, GridRow{5, 5}));

/**
 * @brief Fixture of a crowded grid: 25 well-separated objects drifting uniformly for 20 frames.
 *        It runs the scene once and keeps what each frame showed.
 */
class Small_TrackerCrowdedSceneTest : public Small_TrackerMultiObjectTest
{
    protected:
        static constexpr int K_COLS = 5;
        static constexpr int K_ROWS = 5;
        static constexpr int K_COUNT = K_COLS * K_ROWS;
        static constexpr int K_FRAMES = 20;

        std::set<int64_t> seed_ids_;
        int frames_with_wrong_count_ = 0;
        int frames_with_drifted_ids_ = 0;
        int tracks_with_bad_geometry_ = 0;

        void SetUp() override
        {
            Small_TrackerMultiObjectTest::SetUp();
            run_scene();
        }

    private:
        void run_scene()
        {
            std::vector<sushitrack_track_t> out(static_cast<std::size_t>(K_COUNT) + 8);
            for (int frame = 0; frame < K_FRAMES; ++frame)
            {
                const std::vector<sushitrack_object_t> detections =
                    grid_frame(K_COLS, K_ROWS, frame);
                const int n = update_frame(hdl_, detections.data(), K_COUNT, out.data(),
                                           static_cast<int>(out.size()), 0.04f);
                std::set<int64_t> ids;
                for (int i = 0; i < n; ++i)
                {
                    const sushitrack_track_t& track = out[static_cast<std::size_t>(i)];
                    if (!geometry_is_sane(track))
                        ++tracks_with_bad_geometry_;
                    ids.insert(track.track_id);
                }
                if (frame == 0)
                    seed_ids_ = ids;
                if (n != K_COUNT)
                    ++frames_with_wrong_count_;
                if (ids != seed_ids_)
                    ++frames_with_drifted_ids_;
            }
        }
};

/**
 * @brief The 25 objects get 25 distinct ids on the seed frame and show exactly that id set on
 *        every later frame, so no track churns, swaps or doubles up.
 */
TEST_F(Small_TrackerCrowdedSceneTest, CrowdedSceneKeepsDistinctStableIdentities)
{
    EXPECT_EQ(static_cast<int>(seed_ids_.size()), K_COUNT);
    EXPECT_EQ(frames_with_drifted_ids_, 0);
}

/**
 * @brief The scene shows one track per object on every frame.
 */
TEST_F(Small_TrackerCrowdedSceneTest, EveryFrameShowsOneTrackPerObject)
{
    EXPECT_EQ(frames_with_wrong_count_, 0);
}

/**
 * @brief Every reported box is finite with a positive extent on every frame.
 */
TEST_F(Small_TrackerCrowdedSceneTest, GeometryStaysSane)
{
    EXPECT_EQ(tracks_with_bad_geometry_, 0);
}

/**
 * @brief Fixture of two objects in lanes 150 px apart in y, so their boxes (100 px high) never
 *        overlap, that move in opposite x directions. The run keeps the ids anchored to a lane.
 */
class Small_TrackerCounterMovingTest : public Small_TrackerMultiObjectTest
{
    protected:
        static constexpr float K_LANE_SPLIT = 225.0f;

        int steps_without_left_id_ = 0;
        int steps_without_right_id_ = 0;
        int steps_with_left_id_in_right_lane_ = 0;
        int steps_with_right_id_in_left_lane_ = 0;
        int64_t id_left_ = -1;
        int64_t id_right_ = -1;

        void SetUp() override
        {
            Small_TrackerMultiObjectTest::SetUp();
            run_crossing();
        }

    private:
        /**
         * @brief Seeds both objects, then marches them 40 px per step past each other in x for
         *        ten steps and counts the steps that break each lane promise.
         */
        void run_crossing()
        {
            std::vector<sushitrack_track_t> out(8);
            float left_x = 100.0f;
            float right_x = 500.0f;
            const sushitrack_object_t seed[2] = {make_det(left_x, 150.0f),
                                                 make_det(right_x, 300.0f)};
            const int n0 = update_frame(hdl_, seed, 2, out.data(), 8, 0.04f);
            for (int i = 0; i < n0; ++i)
            {
                const sushitrack_track_t& track = out[static_cast<std::size_t>(i)];
                (track.y < K_LANE_SPLIT ? id_left_ : id_right_) = track.track_id;
            }

            for (int k = 1; k <= 10; ++k)
            {
                left_x += 40.0f;
                right_x -= 40.0f;
                const sushitrack_object_t pair[2] = {make_det(left_x, 150.0f),
                                                     make_det(right_x, 300.0f)};
                const int n = update_frame(hdl_, pair, 2, out.data(), 8, 0.04f);
                const sushitrack_track_t* left = find_by_id(out, n, id_left_);
                const sushitrack_track_t* right = find_by_id(out, n, id_right_);
                if (left == nullptr)
                    ++steps_without_left_id_;
                else if (left->y >= K_LANE_SPLIT)
                    ++steps_with_left_id_in_right_lane_;
                if (right == nullptr)
                    ++steps_without_right_id_;
                else if (right->y <= K_LANE_SPLIT)
                    ++steps_with_right_id_in_left_lane_;
            }
        }
};

/**
 * @brief The seed frame gives the two lanes two different ids.
 */
TEST_F(Small_TrackerCounterMovingTest, SeedFrameGivesTheLanesDifferentIds)
{
    ASSERT_NE(id_left_, -1);
    ASSERT_NE(id_right_, -1);
    EXPECT_NE(id_left_, id_right_);
}

/**
 * @brief The left lane's id stays in the output and in the left lane while the objects pass
 *        each other.
 */
TEST_F(Small_TrackerCounterMovingTest, LeftIdStaysInTheLeftLane)
{
    EXPECT_EQ(steps_without_left_id_, 0);
    EXPECT_EQ(steps_with_left_id_in_right_lane_, 0);
}

/**
 * @brief The right lane's id stays in the output and in the right lane while the objects pass
 *        each other.
 */
TEST_F(Small_TrackerCounterMovingTest, RightIdStaysInTheRightLane)
{
    EXPECT_EQ(steps_without_right_id_, 0);
    EXPECT_EQ(steps_with_right_id_in_left_lane_, 0);
}

/**
 * @brief Fixture of six Tracked objects 200 px apart, established over three frames.
 */
class Small_TrackerOutputCapacityTest : public Small_TrackerMultiObjectTest
{
    protected:
        static constexpr int K_OBJECTS = 6;
        static constexpr int K_CAP = 3;

        std::vector<sushitrack_track_t> out_ = std::vector<sushitrack_track_t>(K_OBJECTS);

        /**
         * @brief Feeds three frames of all six objects, so that every one is Tracked.
         */
        void establish_all()
        {
            for (int frame = 0; frame < 3; ++frame)
            {
                const std::vector<sushitrack_object_t> detections =
                    row_frame(K_OBJECTS, K_OBJECTS, frame);
                ASSERT_EQ(update_frame(hdl_, detections.data(), K_OBJECTS, out_.data(),
                                       K_OBJECTS, 0.04f),
                          K_OBJECTS);
            }
        }

        /**
         * @brief Applies frame 3 with the first @p visible objects into a buffer of K_CAP
         *        tracks and returns the status, with the count in @p count.
         */
        sushitrack_status_t update_into_small_buffer(int visible, int& count)
        {
            const std::vector<sushitrack_object_t> detections =
                row_frame(K_OBJECTS, visible, 3);
            return sushitrack_update(hdl_, detections.data(), static_cast<int>(detections.size()),
                                     out_.data(), K_CAP, 0.04f, &count);
        }
};

/**
 * @brief With six tracks and room for three, the update writes exactly three and reports the
 *        overflow.
 */
TEST_F(Small_TrackerOutputCapacityTest, UpdateWritesAtMostTheBufferAndReportsTheOverflow)
{
    ASSERT_NO_FATAL_FAILURE(establish_all());
    int count = -1;

    const sushitrack_status_t status = update_into_small_buffer(K_OBJECTS, count);

    EXPECT_EQ(status, SUSHITRACK_BUFFER_OVERFLOW);
    EXPECT_EQ(count, K_CAP);
}

/**
 * @brief The three tracks that fit are the Tracked ones: when three objects vanish, their Lost
 *        tracks queue behind the three visible ones.
 *
 * Frame 3 shows objects 0 to 2, which move 2 px and match. Objects 3 to 5 find no detection and
 * turn Lost, which sorts after Tracked, so the first three entries are the visible objects.
 */
TEST_F(Small_TrackerOutputCapacityTest, TrackedTracksAreWrittenBeforeLostOnes)
{
    ASSERT_NO_FATAL_FAILURE(establish_all());
    int count = -1;

    update_into_small_buffer(K_CAP, count);

    ASSERT_EQ(count, K_CAP);
    EXPECT_EQ(count_tracked(out_, count), K_CAP);
}
