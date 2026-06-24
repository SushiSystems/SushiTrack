/**
 * @file      TestByteAssociation.cpp
 * @brief     End-to-end coverage of the two-stage (BYTE) association contract.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      15.06.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 *
 * Scope
 * -----
 * The defining idea of ByteTrack: detections are split by `track_thresh` into a
 * high-confidence pool (primary association + new-track seeding) and a
 * low-confidence pool (secondary association only). Two contract points follow
 * directly and are validated here through the C API:
 *
 *   1. A confirmed track whose detection score collapses into the low pool
 *      (e.g. partial occlusion dimming the detector) is *sustained* by the
 *      secondary stage rather than being dropped — the headline ByteTrack win.
 *   2. Low-confidence detections never *seed* a new track. Only detections at
 *      or above `high_thresh` may initialise an identity, so spurious
 *      low-score boxes cannot pollute the track set.
 */

#include <gtest/gtest.h>
#include "Helpers.h"

#include <vector>

using namespace sushitrack_it;

namespace
{

class ByteAssociationTest : public ::testing::Test
{
protected:
    SushiTrackHdl     hdl_    = nullptr;
    SushiTrack_Params params_ = {};

    void SetUp() override
    {
        SushiTrack_GetDefaultParams(&params_);
        params_.time_aware         = 0;
        params_.enable_tentative   = 0;
        params_.primary_assoc_mode = 1;
        params_.frame_rate         = 25;
        params_.track_buffer       = 30;
        // High/low split at 0.5; new tracks require >= 0.6.
        params_.track_thresh       = 0.5f;
        params_.high_thresh        = 0.6f;
        params_.match_thresh       = 0.9f;
        params_.iou_match_thresh   = 0.5f;

        hdl_ = SushiTrack_Create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override { if (hdl_) SushiTrack_Destroy(hdl_); }
};

/**
 * @brief A confirmed track survives a run of low-confidence detections via the
 *        secondary association stage, keeping its identity throughout, and is
 *        not duplicated by the low-score boxes.
 */
TEST_F(ByteAssociationTest, ConfirmedTrackSustainedByLowConfidenceDetections)
{
    std::vector<SushiTrack_Track> out(8);

    // Confirm the track on high-confidence detections.
    int64_t id = -1;
    for (int i = 0; i < 4; ++i)
    {
        SushiTrack_Object d = makeDet(100.0f + 4.0f * i, 200.0f, /*score=*/0.9f);
        int n = SushiTrack_Update(hdl_, &d, 1, out.data(), 8, 0.04f);
        ASSERT_GE(n, 1);
        id = out[0].track_id;
    }
    ASSERT_NE(id, -1);

    // Three frames of low-confidence (below track_thresh) detections on the object's path. These land in the low pool and must keep the track alive.
    for (int i = 0; i < 3; ++i)
    {
        SushiTrack_Object d_low = makeDet(116.0f + 4.0f * i, 200.0f, /*score=*/0.35f);
        int n = SushiTrack_Update(hdl_, &d_low, 1, out.data(), 8, 0.04f);
        ASSERT_GE(n, 1);

        const SushiTrack_Track* t = findById(out, n, id);
        ASSERT_NE(t, nullptr)
            << "Track was dropped when its detection fell into the low-confidence "
               "pool (frame " << i << ").";
        EXPECT_EQ(t->state, kTracked)
            << "Low-confidence-sustained track should remain Tracked, not Lost.";

        // The low-score box must not have spawned a parallel identity.
        EXPECT_EQ(countTracked(out, n), 1)
            << "A low-confidence detection seeded a spurious second track.";
    }

    // Score recovers: the object returns to the high pool under the same id.
    SushiTrack_Object d_high = makeDet(130.0f, 200.0f, /*score=*/0.9f);
    int n = SushiTrack_Update(hdl_, &d_high, 1, out.data(), 8, 0.04f);
    ASSERT_GE(n, 1);
    const SushiTrack_Track* t = findById(out, n, id);
    ASSERT_NE(t, nullptr) << "Track lost its identity on return to high confidence.";
    EXPECT_EQ(t->state, kTracked);
}

/**
 * @brief A low-confidence detection presented to an empty tracker creates no
 *        track at all — low scores may never seed an identity.
 */
TEST_F(ByteAssociationTest, LowConfidenceDetectionDoesNotSeedNewTrack)
{
    std::vector<SushiTrack_Track> out(8);

    // Several frames of nothing but low-score boxes.
    for (int i = 0; i < 5; ++i)
    {
        SushiTrack_Object d_low = makeDet(300.0f, 300.0f, /*score=*/0.3f);
        int n = SushiTrack_Update(hdl_, &d_low, 1, out.data(), 8, 0.04f);
        EXPECT_EQ(countTracked(out, n), 0)
            << "Low-confidence detection seeded a track at frame " << i << ".";
    }
}

/**
 * @brief A mid-confidence detection (>= track_thresh but < high_thresh) is high
 *        enough to associate but too low to seed: with no existing track it
 *        must not create one.
 */
TEST_F(ByteAssociationTest, MidConfidenceDetectionDoesNotSeedNewTrack)
{
    std::vector<SushiTrack_Track> out(8);

    // score 0.55 is in [track_thresh=0.5, high_thresh=0.6): primary pool, but below the new-track seeding bar.
    for (int i = 0; i < 4; ++i)
    {
        SushiTrack_Object d = makeDet(300.0f, 300.0f, /*score=*/0.55f);
        int n = SushiTrack_Update(hdl_, &d, 1, out.data(), 8, 0.04f);
        EXPECT_EQ(countTracked(out, n), 0)
            << "A sub-high_thresh detection seeded a track at frame " << i << ".";
    }

    // Raising the score above high_thresh finally seeds the identity.
    SushiTrack_Object d_high = makeDet(300.0f, 300.0f, /*score=*/0.9f);
    int n = SushiTrack_Update(hdl_, &d_high, 1, out.data(), 8, 0.04f);
    ASSERT_GE(n, 1);
    EXPECT_GE(countTracked(out, n), 1)
        << "A high-confidence detection failed to seed a track.";
}

} // namespace
