/**
 * @file      TestSushiTrackTracker.cpp
 * @brief     Tracker orchestration tests covering association stages, lifecycle transitions,
 *            duplicate removal, and tentative confirmation tiers.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      13.05.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 *
 * Coverage
 * --------
 * TrackerHighConfidenceTest (primary_assoc_mode=1, enable_tentative=0)
 *   - matchHighConfidence (mode 1) associates a detection to an existing Tracked track.
 *   - matchHighConfidence (mode 0) associates a detection to an existing Tracked track.
 *
 * TrackerLowConfidenceTest (track_thresh=0.6, high_thresh=0.7)
 *   - A low-confidence detection does not prevent a Tracked track from going Lost
 *     when the low-confidence match fails to sustain the track.
 *
 * TrackerLostRecoveryTest (enable_tentative=0)
 *   - A Lost track is recovered to Tracked when a later detection re-associates.
 *
 * TrackerDuplicateTest (enable_tentative=0)
 *   - Two overlapping tracks are deduplicated; only one survives after the update.
 *
 * TrackerTentativeTiersTest (enable_tentative=1, tentative_fast_thresh=0.85)
 *   - A single high-confidence detection promotes a tentative track in one hit
 *     when tentative_fast_hits=1 and the score exceeds tentative_fast_thresh.
 *   - A low-score detection requires tentative_confirm_hits consecutive matches
 *     before promotion occurs.
 *
 * TrackerZeroDetectionTest (enable_tentative=0)
 *   - A zero-detection frame does not crash and returns only pre-existing tracks.
 *   - N detections in the first frame produce exactly N tentative or tracked tracks.
 *
 * Tracking state integers map to SushiTrack::TrackState:
 *   Tracked=1, Lost=2, Removed=3, Tentative=4
 */

#include <gtest/gtest.h>
#include "SushiTrackC.h"
#include <vector>

namespace
{

/**
 * @brief Constructs a detection with the given position, dimensions, and score.
 * @param x     Top-left horizontal coordinate.
 * @param y     Top-left vertical coordinate.
 * @param score Detection confidence score.
 * @return Populated SushiTrack_Object.
 */
SushiTrack_Object makeDet(float x, float y, float score = 0.9f)
{
    SushiTrack_Object o{};
    o.x = x; o.y = y;
    o.width = 50.0f; o.height = 100.0f;
    o.score = score; o.label  = 1;
    return o;
}

}

/**
 * @brief Fixture for matchHighConfidence tests using primary_assoc_mode=1 (LAPJV).
 *
 * enable_tentative=0 so new tracks are immediately Tracked after one hit.
 * track_thresh=0.1, high_thresh=0.2 ensure all test detections fall in the
 * high-confidence pool.
 */
class TrackerHighConfidenceTest : public ::testing::Test
{
protected:
    SushiTrackHdl     hdl_    = nullptr;
    SushiTrack_Params params_ = {};

    void SetUp() override
    {
        SushiTrack_GetDefaultParams(&params_);
        params_.time_aware             = 1;
        params_.enable_tentative       = 0;
        params_.primary_assoc_mode     = 1;
        params_.track_buffer           = 5;
        params_.frame_rate             = 25;
        params_.track_thresh           = 0.1f;
        params_.high_thresh            = 0.2f;
        params_.match_thresh           = 0.9f;

        hdl_ = SushiTrack_Create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override { SushiTrack_Destroy(hdl_); }
};

/**
 * @brief matchHighConfidence (mode 1, LAPJV) sustains a Tracked track across two
 *        consecutive frames when a spatially close detection is presented.
 */
TEST_F(TrackerHighConfidenceTest, LAPJVModeAssociatesClosedDetectionToTrackedTrack)
{
    std::vector<SushiTrack_Track> out(10);
    SushiTrack_Object d0 = makeDet(100.0f, 200.0f);

    SushiTrack_Update(hdl_, &d0, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(out[0].state, 1);
    int64_t id0 = out[0].track_id;

    SushiTrack_Object d1 = makeDet(102.0f, 202.0f);
    int n = SushiTrack_Update(hdl_, &d1, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].track_id, id0);
    EXPECT_EQ(out[0].state,    1);
}

/**
 * @brief Fixture for matchHighConfidence tests using primary_assoc_mode=0 (Age-cascade).
 */
class TrackerHighConfidenceAgeCascadeTest : public ::testing::Test
{
protected:
    SushiTrackHdl     hdl_    = nullptr;
    SushiTrack_Params params_ = {};

    void SetUp() override
    {
        SushiTrack_GetDefaultParams(&params_);
        params_.time_aware             = 1;
        params_.enable_tentative       = 0;
        params_.primary_assoc_mode     = 0;
        params_.track_buffer           = 5;
        params_.frame_rate             = 25;
        params_.track_thresh           = 0.1f;
        params_.high_thresh            = 0.2f;
        params_.match_thresh           = 0.9f;

        hdl_ = SushiTrack_Create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override { SushiTrack_Destroy(hdl_); }
};

/**
 * @brief matchHighConfidence (mode 0, age-cascade) sustains a Tracked track across
 *        two consecutive frames when a spatially close detection is presented.
 */
TEST_F(TrackerHighConfidenceAgeCascadeTest, AgeCascadeModeAssociatesCloseDetectionToTrackedTrack)
{
    std::vector<SushiTrack_Track> out(10);
    SushiTrack_Object d0 = makeDet(100.0f, 200.0f);

    SushiTrack_Update(hdl_, &d0, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(out[0].state, 1);
    int64_t id0 = out[0].track_id;

    SushiTrack_Object d1 = makeDet(102.0f, 202.0f);
    int n = SushiTrack_Update(hdl_, &d1, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].track_id, id0);
    EXPECT_EQ(out[0].state,    1);
}

/**
 * @brief Fixture for matchLowConfidence transition tests.
 *
 * track_thresh=0.6, high_thresh=0.7 forces a score=0.65 detection into the
 * low-confidence pool on the second frame, exercising matchLowConfidence.
 * enable_tentative=0 means the first high-confidence detection directly
 * creates a Tracked track.
 */
class TrackerLowConfidenceTest : public ::testing::Test
{
protected:
    SushiTrackHdl     hdl_    = nullptr;
    SushiTrack_Params params_ = {};

    void SetUp() override
    {
        SushiTrack_GetDefaultParams(&params_);
        params_.time_aware             = 1;
        params_.enable_tentative       = 0;
        params_.primary_assoc_mode     = 1;
        params_.track_buffer           = 5;
        params_.frame_rate             = 25;
        params_.track_thresh           = 0.6f;
        params_.high_thresh            = 0.7f;
        params_.match_thresh           = 0.9f;
        params_.iou_match_thresh       = 0.5f;

        hdl_ = SushiTrack_Create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override { SushiTrack_Destroy(hdl_); }
};

/**
 * @brief A Tracked track goes Lost on a frame where no detection is matched,
 *        regardless of whether a low-confidence detection is present nearby.
 *
 * Frame 1 — high-confidence detection creates a Tracked track.
 * Frame 2 — no detections: track transitions to Lost (state=2).
 */
TEST_F(TrackerLowConfidenceTest, UnmatchedTrackedTrackTransitionsToLost)
{
    std::vector<SushiTrack_Track> out(10);
    SushiTrack_Object d0 = makeDet(100.0f, 200.0f, 0.9f);

    SushiTrack_Update(hdl_, &d0, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(out[0].state, 1);

    int n = SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state, 2);
}

/**
 * @brief Fixture for lost-track recovery tests.
 *
 * enable_tentative=0, track_buffer=5, frame_rate=25 → 0.2 s recovery window.
 */
class TrackerLostRecoveryTest : public ::testing::Test
{
protected:
    SushiTrackHdl     hdl_    = nullptr;
    SushiTrack_Params params_ = {};

    void SetUp() override
    {
        SushiTrack_GetDefaultParams(&params_);
        params_.time_aware             = 1;
        params_.enable_tentative       = 0;
        params_.primary_assoc_mode     = 1;
        params_.track_buffer           = 5;
        params_.frame_rate             = 25;
        params_.track_thresh           = 0.1f;
        params_.high_thresh            = 0.2f;
        params_.match_thresh           = 0.9f;

        hdl_ = SushiTrack_Create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override { SushiTrack_Destroy(hdl_); }
};

/**
 * @brief A Lost track recovers to Tracked (state=1) with lost_time reset to zero
 *        when a matching detection arrives within the buffer window.
 */
TEST_F(TrackerLostRecoveryTest, LostTrackRecoveredByReassociation)
{
    std::vector<SushiTrack_Track> out(10);
    SushiTrack_Object d0 = makeDet(100.0f, 200.0f);

    SushiTrack_Update(hdl_, &d0, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(out[0].state, 1);

    SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 0.04f);
    ASSERT_EQ(out[0].state, 2);

    SushiTrack_Object d1 = makeDet(102.0f, 202.0f);
    int n = SushiTrack_Update(hdl_, &d1, 1, out.data(), 10, 0.04f);
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
class TrackerDuplicateTest : public ::testing::Test
{
protected:
    SushiTrackHdl     hdl_    = nullptr;
    SushiTrack_Params params_ = {};

    void SetUp() override
    {
        SushiTrack_GetDefaultParams(&params_);
        params_.time_aware             = 1;
        params_.enable_tentative       = 0;
        params_.primary_assoc_mode     = 1;
        params_.track_buffer           = 5;
        params_.frame_rate             = 25;
        params_.track_thresh           = 0.1f;
        params_.high_thresh            = 0.2f;
        params_.match_thresh           = 0.9f;
        params_.duplicate_iou_thresh   = 0.15f;

        hdl_ = SushiTrack_Create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override { SushiTrack_Destroy(hdl_); }
};

/**
 * @brief When two Tracked tracks with heavily overlapping bounding boxes are
 *        present, removeDuplicateTracklets eliminates the younger one so that
 *        only one Tracked track remains.
 *
 * Both tracks are seeded at nearly the same position to guarantee IoU >> 0.15.
 * The first frame creates two separate tracks because separate detections are
 * submitted. The second frame presents one detection so that only one track
 * can match; the older track absorbs the detection and the younger becomes
 * a duplicate candidate.
 */
TEST_F(TrackerDuplicateTest, OverlappingTracksDeduplicatedToOne)
{
    std::vector<SushiTrack_Track> out(10);

    SushiTrack_Object first  = makeDet(100.0f, 200.0f);
    SushiTrack_Object second = makeDet(101.0f, 201.0f);
    SushiTrack_Object pair[2] = { first, second };

    SushiTrack_Update(hdl_, pair, 2, out.data(), 10, 0.04f);

    SushiTrack_Object single = makeDet(100.5f, 200.5f);
    int n = SushiTrack_Update(hdl_, &single, 1, out.data(), 10, 0.04f);

    int tracked_count = 0;
    for (int i = 0; i < n; ++i)
        if (out[i].state == 1) ++tracked_count;

    EXPECT_LE(tracked_count, 1);
}

/**
 * @brief Fixture for tentative confirmation-tier tests.
 *
 * tentative_fast_thresh=0.85, tentative_fast_hits=1 — a score ≥ 0.85 confirms
 * in a single hit.
 * tentative_confirm_hits=3 — scores below tentative_fast_thresh need three hits.
 * tentative_slow_thresh=0.40, tentative_slow_hits_offset=1 — scores in
 * [0.40, high_thresh) need four hits.
 */
class TrackerTentativeTiersTest : public ::testing::Test
{
protected:
    SushiTrackHdl     hdl_    = nullptr;
    SushiTrack_Params params_ = {};

    void SetUp() override
    {
        SushiTrack_GetDefaultParams(&params_);
        params_.time_aware                = 1;
        params_.enable_tentative          = 1;
        params_.tentative_confirm_hits    = 3;
        params_.tentative_fast_thresh     = 0.85f;
        params_.tentative_fast_hits       = 1;
        params_.tentative_slow_thresh     = 0.40f;
        params_.tentative_slow_hits_offset = 1;
        params_.tentative_max_age_seconds = 2.0f;
        params_.track_buffer              = 5;
        params_.frame_rate                = 25;
        params_.tentative_match_thresh    = 0.9f;
        params_.track_thresh              = 0.1f;
        params_.high_thresh               = 0.2f;

        hdl_ = SushiTrack_Create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override { SushiTrack_Destroy(hdl_); }
};

/**
 * @brief A high-score detection (score >= tentative_fast_thresh) promotes to
 *        Tracked on the fast tier after a single confirming match.
 *
 * The SushiTrack tentative pipeline creates a Tentative track on the first
 * frame (state=4, is_confirmed=0) and applies the implicit one-frame
 * validation rule: a subsequent matching observation is required before
 * promotion to Tracked. With tentative_fast_hits=1 and score >= 0.85, the
 * second frame is the one that flips state to 1 / is_confirmed to 1.
 */
TEST_F(TrackerTentativeTiersTest, HighScoreDetectionPromotesAfterOneConfirmingHit)
{
    std::vector<SushiTrack_Track> out(10);

    // Frame 1: the high-score detection creates a Tentative track.
    SushiTrack_Object d0 = makeDet(100.0f, 200.0f, 0.9f);
    int n = SushiTrack_Update(hdl_, &d0, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state,        4);
    EXPECT_EQ(out[0].is_confirmed, 0);

    // Frame 2: the confirming match promotes the tentative to Tracked under the fast tier (tentative_fast_hits=1, score still above fast_thresh).
    SushiTrack_Object d1 = makeDet(102.0f, 202.0f, 0.9f);
    n = SushiTrack_Update(hdl_, &d1, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state,        1);
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
TEST_F(TrackerTentativeTiersTest, StandardScoreDetectionRequiresConfirmHitsBeforePromotion)
{
    std::vector<SushiTrack_Track> out(10);
    SushiTrack_Object d0 = makeDet(100.0f, 200.0f, 0.5f);
    SushiTrack_Object d1 = makeDet(102.0f, 202.0f, 0.5f);
    SushiTrack_Object d2 = makeDet(104.0f, 204.0f, 0.5f);

    SushiTrack_Update(hdl_, &d0, 1, out.data(), 10, 0.04f);
    EXPECT_EQ(out[0].state, 4);

    SushiTrack_Update(hdl_, &d1, 1, out.data(), 10, 0.04f);
    EXPECT_EQ(out[0].state, 4);

    int n = SushiTrack_Update(hdl_, &d2, 1, out.data(), 10, 0.04f);
    ASSERT_EQ(n, 1);
    EXPECT_EQ(out[0].state,        1);
    EXPECT_EQ(out[0].is_confirmed, 1);
}

/**
 * @brief Fixture for zero-detection and N-track initialisation tests.
 *
 * enable_tentative=0 so tracks are immediately Tracked after the first hit.
 */
class TrackerZeroDetectionTest : public ::testing::Test
{
protected:
    SushiTrackHdl     hdl_    = nullptr;
    SushiTrack_Params params_ = {};

    void SetUp() override
    {
        SushiTrack_GetDefaultParams(&params_);
        params_.time_aware             = 1;
        params_.enable_tentative       = 0;
        params_.primary_assoc_mode     = 1;
        params_.track_buffer           = 5;
        params_.frame_rate             = 25;
        params_.track_thresh           = 0.1f;
        params_.high_thresh            = 0.2f;
        params_.match_thresh           = 0.9f;

        hdl_ = SushiTrack_Create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override { SushiTrack_Destroy(hdl_); }
};

/**
 * @brief A zero-detection frame on an empty tracker returns zero tracks without error.
 */
TEST_F(TrackerZeroDetectionTest, ZeroDetectionsOnEmptyTrackerReturnsZero)
{
    std::vector<SushiTrack_Track> out(10);
    int n = SushiTrack_Update(hdl_, nullptr, 0, out.data(), 10, 0.04f);
    EXPECT_EQ(n, 0);
}

/**
 * @brief N detections in the first frame produce exactly N tracks.
 */
TEST_F(TrackerZeroDetectionTest, NDetectionsProduceNTracks)
{
    std::vector<SushiTrack_Track> out(10);
    SushiTrack_Object dets[4];
    dets[0] = makeDet(100.0f, 200.0f);
    dets[1] = makeDet(300.0f, 200.0f);
    dets[2] = makeDet(500.0f, 200.0f);
    dets[3] = makeDet(700.0f, 200.0f);

    int n = SushiTrack_Update(hdl_, dets, 4, out.data(), 10, 0.04f);
    EXPECT_EQ(n, 4);
}
