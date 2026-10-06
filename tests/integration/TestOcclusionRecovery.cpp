/**************************************************************************/
/* TestOcclusionRecovery.cpp                                              */
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
 * @file TestOcclusionRecovery.cpp
 * @brief Integration tests for occlusion recovery behaviour.
 * @author Mustafa Garip
 */

#include <gtest/gtest.h>
#include "Helpers.h"

#include <vector>

using namespace sushitrack_it;

namespace
{

/**
 * @brief Core-path fixture: frame-based timing, no advanced features.
 *
 * track_buffer is deliberately left at a moderate value per test so the
 * within-buffer and beyond-buffer cases can be expressed by varying only the
 * length of the occlusion gap.
 */
class OcclusionRecoveryTest : public ::testing::Test
{
protected:
    sushitrack_tracker_t     hdl_    = nullptr;
    sushitrack_params_t params_ = {};

    void configure(int track_buffer)
    {
        sushitrack_get_default_params(&params_);
        params_.time_aware         = 0;          // frame-counted expiry
        params_.enable_tentative   = 0;
        params_.primary_assoc_mode = 1;          // global LAPJV (ByteTrack-identical)
        params_.frame_rate         = 25;
        params_.track_buffer       = track_buffer;
        params_.track_thresh       = 0.2f;
        params_.high_thresh        = 0.5f;
        params_.match_thresh       = 0.9f;       // generous primary cost gate

        hdl_ = sushitrack_create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override { if (hdl_) sushitrack_destroy(hdl_); }

    /**
     * @brief Confirms a track moving at a constant velocity and returns its id
     *        plus the final position, so callers can extrapolate the gap.
     */
    int64_t confirmMovingTrack(std::vector<sushitrack_track_t>& out,
                               float x0, float y0, float vx, float vy,
                               int frames, float& out_x, float& out_y)
    {
        int64_t id = -1;
        out_x = x0;
        out_y = y0;
        for (int i = 0; i < frames; ++i)
        {
            out_x = x0 + vx * static_cast<float>(i);
            out_y = y0 + vy * static_cast<float>(i);
            sushitrack_object_t d = makeDet(out_x, out_y);
            int n = sushitrack_update(hdl_, &d, 1, out.data(), 8, 0.04f);
            EXPECT_GE(n, 1);
            if (n >= 1) id = out[0].track_id;
        }
        return id;
    }
};

/**
 * @brief A short occlusion (well inside the buffer) is bridged: the object is
 *        re-acquired under its original ID when it reappears on its predicted
 *        trajectory.
 */
TEST_F(OcclusionRecoveryTest, ShortOcclusionRecoversOriginalId)
{
    configure(/*track_buffer=*/30);
    std::vector<sushitrack_track_t> out(8);

    const float vx = 6.0f, vy = 0.0f;
    float last_x = 0.0f, last_y = 0.0f;
    const int64_t id = confirmMovingTrack(out, 100.0f, 200.0f, vx, vy,
                                           /*frames=*/6, last_x, last_y);
    ASSERT_NE(id, -1);

    // Occlude for 3 frames. The Kalman prediction keeps propagating position velocity for Lost tracks, so the predicted box stays on the trajectory.
    constexpr int kGap = 3;
    for (int i = 0; i < kGap; ++i)
        sushitrack_update(hdl_, nullptr, 0, out.data(), 8, 0.04f);

    // Reappear at the extrapolated position so the detection overlaps the propagated prediction.
    const float reappear_x = last_x + vx * static_cast<float>(kGap + 1);
    const float reappear_y = last_y + vy * static_cast<float>(kGap + 1);
    sushitrack_object_t back = makeDet(reappear_x, reappear_y);
    int n = sushitrack_update(hdl_, &back, 1, out.data(), 8, 0.04f);
    ASSERT_GE(n, 1);

    const sushitrack_track_t* recovered = findById(out, n, id);
    ASSERT_NE(recovered, nullptr)
        << "Object lost its identity across a short occlusion within the buffer.";
    EXPECT_EQ(recovered->state, kTracked);
    EXPECT_TRUE(geometryIsSane(*recovered));
}

/**
 * @brief An occlusion longer than the buffer retires the track: when the object
 *        reappears it is issued a brand-new identity, and the stale id is gone.
 */
TEST_F(OcclusionRecoveryTest, LongOcclusionRetiresTrackAndReassignsNewId)
{
    configure(/*track_buffer=*/5);
    std::vector<sushitrack_track_t> out(8);

    float last_x = 0.0f, last_y = 0.0f;
    const int64_t id = confirmMovingTrack(out, 100.0f, 200.0f, 5.0f, 0.0f,
                                           /*frames=*/6, last_x, last_y);
    ASSERT_NE(id, -1);

    // Gap far exceeding the 5-frame buffer: the lost track must be removed by cleanupLost before the object returns.
    for (int i = 0; i < 20; ++i)
        sushitrack_update(hdl_, nullptr, 0, out.data(), 8, 0.04f);

    // Reappear somewhere unrelated. A retired identity cannot be reclaimed.
    sushitrack_object_t back = makeDet(600.0f, 200.0f);
    int n = sushitrack_update(hdl_, &back, 1, out.data(), 8, 0.04f);
    ASSERT_GE(n, 1);

    const sushitrack_track_t* stale = findById(out, n, id);
    EXPECT_EQ(stale, nullptr)
        << "A track gone far longer than the buffer was resurrected under its "
           "old id instead of being retired.";

    // Whatever identity now exists for the reappeared object must differ from the retired one and have sane geometry.
    bool found_new = false;
    for (int i = 0; i < n; ++i)
    {
        if (out[static_cast<std::size_t>(i)].track_id != id)
        {
            found_new = true;
            EXPECT_TRUE(geometryIsSane(out[static_cast<std::size_t>(i)]));
        }
    }
    EXPECT_TRUE(found_new) << "Reappeared object produced no fresh track.";
}

/**
 * @brief When one of two well-separated objects drops out for a couple of
 *        frames, the visible object keeps its identity uninterrupted and the
 *        occluded one recovers its own identity on reappearance — neither id is
 *        transferred to the other.
 */
TEST_F(OcclusionRecoveryTest, OneOfTwoObjectsRecoversWithoutIdTransfer)
{
    configure(/*track_buffer=*/30);
    std::vector<sushitrack_track_t> out(8);

    // Two objects in separate lanes (200 px apart vertically), both moving right.
    int64_t id_top = -1, id_bot = -1;
    for (int i = 0; i < 6; ++i)
    {
        sushitrack_object_t top = makeDet(100.0f + 5.0f * i, 100.0f);
        sushitrack_object_t bot = makeDet(100.0f + 5.0f * i, 400.0f);
        sushitrack_object_t dets[2] = { top, bot };
        int n = sushitrack_update(hdl_, dets, 2, out.data(), 8, 0.04f);
        ASSERT_GE(n, 2);
        for (int k = 0; k < n; ++k)
        {
            const sushitrack_track_t& t = out[static_cast<std::size_t>(k)];
            if (t.y < 250.0f) id_top = t.track_id;
            else              id_bot = t.track_id;
        }
    }
    ASSERT_NE(id_top, -1);
    ASSERT_NE(id_bot, -1);
    ASSERT_NE(id_top, id_bot);

    // Drop the top object for 2 frames; the bottom object stays visible and must never lose its identity.
    for (int i = 0; i < 2; ++i)
    {
        sushitrack_object_t bot = makeDet(100.0f + 5.0f * (6 + i), 400.0f);
        int n = sushitrack_update(hdl_, &bot, 1, out.data(), 8, 0.04f);
        ASSERT_GE(n, 1);
        const sushitrack_track_t* b = findById(out, n, id_bot);
        ASSERT_NE(b, nullptr) << "Visible object lost its id while its neighbour was occluded.";
        EXPECT_EQ(b->state, kTracked);
    }

    // Top object reappears on its trajectory.
    sushitrack_object_t top = makeDet(100.0f + 5.0f * 8, 100.0f);
    sushitrack_object_t bot = makeDet(100.0f + 5.0f * 8, 400.0f);
    sushitrack_object_t dets[2] = { top, bot };
    int n = sushitrack_update(hdl_, dets, 2, out.data(), 8, 0.04f);
    ASSERT_GE(n, 2);

    const sushitrack_track_t* b = findById(out, n, id_bot);
    const sushitrack_track_t* a = findById(out, n, id_top);
    ASSERT_NE(b, nullptr) << "Continuously-visible object lost its identity.";
    ASSERT_NE(a, nullptr) << "Occluded object failed to recover its identity.";
    // The recovered top object must still be in the top lane — its id was not swapped onto the bottom detection.
    EXPECT_LT(a->y, 250.0f) << "Recovered identity migrated into the wrong lane.";
    EXPECT_GT(b->y, 250.0f);
}

} // namespace
