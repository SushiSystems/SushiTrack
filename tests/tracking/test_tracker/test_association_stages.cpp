/**************************************************************************/
/* test_association_stages.cpp                                            */
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
 * @file test_association_stages.cpp
 * @brief Tests the tracker's association stages.
 * @author Mustafa Garip
 */

#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/c_api/sushitrack_c.h>

#include <c_detection_builders.hpp>

namespace
{

    /**
     * @brief Constructs a detection with the given position, dimensions, and score.
     * @param x     Top-left horizontal coordinate.
     * @param y     Top-left vertical coordinate.
     * @param score Detection confidence score.
     * @return Populated sushitrack_object_t.
     */
    sushitrack_object_t make_det(float x, float y, float score = 0.9f)
    {
        sushitrack_object_t o{};
        o.x = x;
        o.y = y;
        o.width = 50.0f;
        o.height = 100.0f;
        o.score = score;
        o.label = 1;
        return o;
    }

} // namespace

/**
 * @brief Fixture for match_high_confidence tests using primary_assoc_mode=1 (LAPJV).
 *
 * enable_tentative=0 so new tracks are immediately Tracked after one hit.
 * track_thresh=0.1, high_thresh=0.2 ensure all test detections fall in the
 * high-confidence pool.
 */
class Small_TrackerHighConfidenceTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 1;
            params_.enable_tentative = 0;
            params_.primary_assoc_mode = 1;
            params_.track_buffer = 5;
            params_.frame_rate = 25;
            params_.track_thresh = 0.1f;
            params_.high_thresh = 0.2f;
            params_.match_thresh = 0.9f;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override { sushitrack_destroy(hdl_); }
};

/**
 * @brief match_high_confidence (mode 1, LAPJV) sustains a Tracked track across two
 *        consecutive frames when a spatially close detection is presented.
 */
TEST_F(Small_TrackerHighConfidenceTest, LAPJVModeAssociatesClosedDetectionToTrackedTrack)
{
    std::vector<sushitrack_track_t> out(10);
    sushitrack_object_t d0 = make_det(100.0f, 200.0f);

    CDetections::update_frame(hdl_, &d0, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(out[0].state, 1);
    int64_t id0 = out[0].track_id;

    sushitrack_object_t d1 = make_det(102.0f, 202.0f);
    int n = CDetections::update_frame(hdl_, &d1, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].track_id, id0);
    EXPECT_EQ(out[0].state, 1);
}

/**
 * @brief Fixture for match_high_confidence tests using primary_assoc_mode=0 (Age-cascade).
 */
class Small_TrackerHighConfidenceAgeCascadeTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 1;
            params_.enable_tentative = 0;
            params_.primary_assoc_mode = 0;
            params_.track_buffer = 5;
            params_.frame_rate = 25;
            params_.track_thresh = 0.1f;
            params_.high_thresh = 0.2f;
            params_.match_thresh = 0.9f;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override { sushitrack_destroy(hdl_); }
};

/**
 * @brief match_high_confidence (mode 0, age-cascade) sustains a Tracked track across
 *        two consecutive frames when a spatially close detection is presented.
 */
TEST_F(Small_TrackerHighConfidenceAgeCascadeTest,
       AgeCascadeModeAssociatesCloseDetectionToTrackedTrack)
{
    std::vector<sushitrack_track_t> out(10);
    sushitrack_object_t d0 = make_det(100.0f, 200.0f);

    CDetections::update_frame(hdl_, &d0, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(out[0].state, 1);
    int64_t id0 = out[0].track_id;

    sushitrack_object_t d1 = make_det(102.0f, 202.0f);
    int n = CDetections::update_frame(hdl_, &d1, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].track_id, id0);
    EXPECT_EQ(out[0].state, 1);
}

/**
 * @brief Fixture for match_low_confidence transition tests.
 *
 * track_thresh=0.6, high_thresh=0.7 forces a score=0.65 detection into the
 * low-confidence pool on the second frame, exercising match_low_confidence.
 * enable_tentative=0 means the first high-confidence detection directly
 * creates a Tracked track.
 */
class Small_TrackerLowConfidenceTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 1;
            params_.enable_tentative = 0;
            params_.primary_assoc_mode = 1;
            params_.track_buffer = 5;
            params_.frame_rate = 25;
            params_.track_thresh = 0.6f;
            params_.high_thresh = 0.7f;
            params_.match_thresh = 0.9f;
            params_.iou_match_thresh = 0.5f;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override { sushitrack_destroy(hdl_); }
};

/**
 * @brief A Tracked track goes Lost on a frame where no detection is matched,
 *        regardless of whether a low-confidence detection is present nearby.
 *
 * Frame 1 — high-confidence detection creates a Tracked track.
 * Frame 2 — no detections: track transitions to Lost (state=2).
 */
TEST_F(Small_TrackerLowConfidenceTest, UnmatchedTrackedTrackTransitionsToLost)
{
    std::vector<sushitrack_track_t> out(10);
    sushitrack_object_t d0 = make_det(100.0f, 200.0f, 0.9f);

    CDetections::update_frame(hdl_, &d0, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(out[0].state, 1);

    int n = CDetections::update_frame(hdl_, nullptr, 0, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 2);
}

/**
 * @brief Fixture for lost-track recovery tests.
 *
 * enable_tentative=0, track_buffer=5, frame_rate=25 → 0.2 s recovery window.
 */
class Small_TrackerLostRecoveryTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 1;
            params_.enable_tentative = 0;
            params_.primary_assoc_mode = 1;
            params_.track_buffer = 5;
            params_.frame_rate = 25;
            params_.track_thresh = 0.1f;
            params_.high_thresh = 0.2f;
            params_.match_thresh = 0.9f;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override { sushitrack_destroy(hdl_); }
};

/**
 * @brief A Lost track recovers to Tracked (state=1) with lost_time reset to zero
 *        when a matching detection arrives within the buffer window.
 */
TEST_F(Small_TrackerLostRecoveryTest, LostTrackRecoveredByReassociation)
{
    std::vector<sushitrack_track_t> out(10);
    sushitrack_object_t d0 = make_det(100.0f, 200.0f);

    CDetections::update_frame(hdl_, &d0, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(out[0].state, 1);

    CDetections::update_frame(hdl_, nullptr, 0, out.data(), 10, 0.04f);
    ASSERT_EQ(out[0].state, 2);

    sushitrack_object_t d1 = make_det(102.0f, 202.0f);
    int n = CDetections::update_frame(hdl_, &d1, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 1);
    EXPECT_FLOAT_EQ(out[0].lost_time, 0.0f);
}

/**
 * @brief Fixture for duplicate-tracklet removal tests.
 *
 * enable_tentative=0, duplicate_iou_thresh=0.15 (default).
 * Two nearly identical boxes with IoU near 1.0 exceed the threshold and trigger
 * deduplication.
 */
class Small_TrackerDuplicateTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 1;
            params_.enable_tentative = 0;
            params_.primary_assoc_mode = 1;
            params_.track_buffer = 5;
            params_.frame_rate = 25;
            params_.track_thresh = 0.1f;
            params_.high_thresh = 0.2f;
            params_.match_thresh = 0.9f;
            params_.duplicate_iou_thresh = 0.15f;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override { sushitrack_destroy(hdl_); }
};

/**
 * @brief When two Tracked tracks with heavily overlapping bounding boxes are
 *        present, remove_duplicate_tracklets eliminates the younger one so that
 *        only one Tracked track remains.
 *
 * Both tracks are seeded at nearly the same position to guarantee IoU >> 0.15.
 */
TEST_F(Small_TrackerDuplicateTest, OverlappingTracksDeduplicatedToOne)
{
    std::vector<sushitrack_track_t> out(10);

    sushitrack_object_t first = make_det(100.0f, 200.0f);
    sushitrack_object_t second = make_det(101.0f, 201.0f);
    sushitrack_object_t pair[2] = {first, second};

    CDetections::update_frame(hdl_, pair, 2, out.data(), 10, 0.04f);

    sushitrack_object_t single = make_det(100.5f, 200.5f);
    int n = CDetections::update_frame(hdl_, &single, 1, out.data(), 10, 0.04f);

    int tracked_count = 0;
    for (int i = 0; i < n; ++i)
        if (out[i].state == 1)
            ++tracked_count;

    EXPECT_LE(tracked_count, 1);
}

/**
 * @brief Fixture for tentative confirmation-tier tests.
 *
 * tentative_fast_thresh=0.85, tentative_fast_hits=1 — a score ≥ 0.85 confirms
 * in a single hit.
 * tentative_confirm_hits=3 — scores below tentative_fast_thresh need three hits.
 */
class Small_TrackerTentativeTiersTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 1;
            params_.enable_tentative = 1;
            params_.tentative_confirm_hits = 3;
            params_.tentative_fast_thresh = 0.85f;
            params_.tentative_fast_hits = 1;
            params_.tentative_slow_thresh = 0.40f;
            params_.tentative_slow_hits_offset = 1;
            params_.tentative_max_age_seconds = 2.0f;
            params_.track_buffer = 5;
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
 * @brief A high-score detection (score >= tentative_fast_thresh) promotes to
 *        Tracked on the fast tier after a single confirming match.
 *
 * The SushiTrack tentative pipeline creates a Tentative track on the first
 * frame (state=4, is_confirmed=0) and applies the implicit one-frame
 */
TEST_F(Small_TrackerTentativeTiersTest, HighScoreDetectionPromotesAfterOneConfirmingHit)
{
    std::vector<sushitrack_track_t> out(10);

    // Frame 1: the high-score detection creates a Tentative track.
    sushitrack_object_t d0 = make_det(100.0f, 200.0f, 0.9f);
    int n = CDetections::update_frame(hdl_, &d0, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 4);
    EXPECT_EQ(out[0].is_confirmed, 0);

    // Frame 2: the confirming match promotes the tentative to Tracked under the fast tier (tentative_fast_hits=1, score still above fast_thresh).
    sushitrack_object_t d1 = make_det(102.0f, 202.0f, 0.9f);
    n = CDetections::update_frame(hdl_, &d1, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 1);
    EXPECT_EQ(out[0].is_confirmed, 1);
}

/**
 * @brief A detection with score in (high_thresh, tentative_fast_thresh) requires
 *        tentative_confirm_hits consecutive matches before promotion.
 *
 * Uses score=0.5 (above high_thresh=0.2, below fast_thresh=0.85) so that the
 * standard tier applies.  After two frames the track is still Tentative; after
 * the third it becomes Tracked.
 */
TEST_F(Small_TrackerTentativeTiersTest, StandardScoreDetectionRequiresConfirmHitsBeforePromotion)
{
    std::vector<sushitrack_track_t> out(10);
    sushitrack_object_t d0 = make_det(100.0f, 200.0f, 0.5f);
    sushitrack_object_t d1 = make_det(102.0f, 202.0f, 0.5f);
    sushitrack_object_t d2 = make_det(104.0f, 204.0f, 0.5f);

    CDetections::update_frame(hdl_, &d0, 1, out.data(), 10, 0.04f);
    EXPECT_EQ(out[0].state, 4);

    CDetections::update_frame(hdl_, &d1, 1, out.data(), 10, 0.04f);
    EXPECT_EQ(out[0].state, 4);

    int n = CDetections::update_frame(hdl_, &d2, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 1);
    EXPECT_EQ(out[0].is_confirmed, 1);
}

/**
 * @brief Fixture for zero-detection and N-track initialisation tests.
 *
 * enable_tentative=0 so tracks are immediately Tracked after the first hit.
 */
class Small_TrackerZeroDetectionTest : public ::testing::Test
{
    protected:
        sushitrack_tracker_t hdl_ = nullptr;
        sushitrack_params_t params_ = {};

        void SetUp() override
        {
            sushitrack_get_default_params(&params_);
            params_.time_aware = 1;
            params_.enable_tentative = 0;
            params_.primary_assoc_mode = 1;
            params_.track_buffer = 5;
            params_.frame_rate = 25;
            params_.track_thresh = 0.1f;
            params_.high_thresh = 0.2f;
            params_.match_thresh = 0.9f;

            hdl_ = sushitrack_create(&params_);
            ASSERT_NE(hdl_, nullptr);
        }

        void TearDown() override { sushitrack_destroy(hdl_); }
};

/**
 * @brief A zero-detection frame on an empty tracker returns zero tracks without error.
 */
TEST_F(Small_TrackerZeroDetectionTest, ZeroDetectionsOnEmptyTrackerReturnsZero)
{
    std::vector<sushitrack_track_t> out(10);
    int n = CDetections::update_frame(hdl_, nullptr, 0, out.data(), 10, 0.04f);
    EXPECT_EQ(n, 0);
}

/**
 * @brief N detections in the first frame produce exactly N tracks.
 */
TEST_F(Small_TrackerZeroDetectionTest, NDetectionsProduceNTracks)
{
    std::vector<sushitrack_track_t> out(10);
    sushitrack_object_t dets[4];
    dets[0] = make_det(100.0f, 200.0f);
    dets[1] = make_det(300.0f, 200.0f);
    dets[2] = make_det(500.0f, 200.0f);
    dets[3] = make_det(700.0f, 200.0f);

    int n = CDetections::update_frame(hdl_, dets, 4, out.data(), 10, 0.04f);
    EXPECT_EQ(n, 4);
}
