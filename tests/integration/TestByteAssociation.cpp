/**************************************************************************/
/* TestByteAssociation.cpp                                                */
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
 * @file TestByteAssociation.cpp
 * @brief Integration tests for ByteTrack association logic.
 * @author Mustafa Garip
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
    sushitrack_tracker_t     hdl_    = nullptr;
    sushitrack_params_t params_ = {};

    void SetUp() override
    {
        sushitrack_get_default_params(&params_);
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

        hdl_ = sushitrack_create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override { if (hdl_) sushitrack_destroy(hdl_); }
};

/**
 * @brief A confirmed track survives a run of low-confidence detections via the
 *        secondary association stage, keeping its identity throughout, and is
 *        not duplicated by the low-score boxes.
 */
TEST_F(ByteAssociationTest, ConfirmedTrackSustainedByLowConfidenceDetections)
{
    std::vector<sushitrack_track_t> out(8);

    // Confirm the track on high-confidence detections.
    int64_t id = -1;
    for (int i = 0; i < 4; ++i)
    {
        sushitrack_object_t d = makeDet(100.0f + 4.0f * i, 200.0f, /*score=*/0.9f);
        int n = sushitrack_update(hdl_, &d, 1, out.data(), 8, 0.04f);
        ASSERT_GE(n, 1);
        id = out[0].track_id;
    }
    ASSERT_NE(id, -1);

    // Three frames of low-confidence (below track_thresh) detections on the object's path. These land in the low pool and must keep the track alive.
    for (int i = 0; i < 3; ++i)
    {
        sushitrack_object_t d_low = makeDet(116.0f + 4.0f * i, 200.0f, /*score=*/0.35f);
        int n = sushitrack_update(hdl_, &d_low, 1, out.data(), 8, 0.04f);
        ASSERT_GE(n, 1);

        const sushitrack_track_t* t = findById(out, n, id);
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
    sushitrack_object_t d_high = makeDet(130.0f, 200.0f, /*score=*/0.9f);
    int n = sushitrack_update(hdl_, &d_high, 1, out.data(), 8, 0.04f);
    ASSERT_GE(n, 1);
    const sushitrack_track_t* t = findById(out, n, id);
    ASSERT_NE(t, nullptr) << "Track lost its identity on return to high confidence.";
    EXPECT_EQ(t->state, kTracked);
}

/**
 * @brief A low-confidence detection presented to an empty tracker creates no
 *        track at all — low scores may never seed an identity.
 */
TEST_F(ByteAssociationTest, LowConfidenceDetectionDoesNotSeedNewTrack)
{
    std::vector<sushitrack_track_t> out(8);

    // Several frames of nothing but low-score boxes.
    for (int i = 0; i < 5; ++i)
    {
        sushitrack_object_t d_low = makeDet(300.0f, 300.0f, /*score=*/0.3f);
        int n = sushitrack_update(hdl_, &d_low, 1, out.data(), 8, 0.04f);
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
    std::vector<sushitrack_track_t> out(8);

    // score 0.55 is in [track_thresh=0.5, high_thresh=0.6): primary pool, but below the new-track seeding bar.
    for (int i = 0; i < 4; ++i)
    {
        sushitrack_object_t d = makeDet(300.0f, 300.0f, /*score=*/0.55f);
        int n = sushitrack_update(hdl_, &d, 1, out.data(), 8, 0.04f);
        EXPECT_EQ(countTracked(out, n), 0)
            << "A sub-high_thresh detection seeded a track at frame " << i << ".";
    }

    // Raising the score above high_thresh finally seeds the identity.
    sushitrack_object_t d_high = makeDet(300.0f, 300.0f, /*score=*/0.9f);
    int n = sushitrack_update(hdl_, &d_high, 1, out.data(), 8, 0.04f);
    ASSERT_GE(n, 1);
    EXPECT_GE(countTracked(out, n), 1)
        << "A high-confidence detection failed to seed a track.";
}

} // namespace
