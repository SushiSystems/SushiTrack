/**************************************************************************/
/* TestDuplicateSuppression.cpp                                           */
/**************************************************************************/
/*                          This file is part of:                         */
/*                               SushiTrack                               */
/*              https://github.com/SushiSystems/SushiTrack                */
/*                          https://sushisystems.io                       */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                          */
/**************************************************************************/

/**
 * @file TestDuplicateSuppression.cpp
 * @brief Integration tests for duplicate track suppression.
 * @author Mustafa Garip
 */

#include <gtest/gtest.h>
#include "Helpers.h"

#include <cmath>
#include <vector>

using namespace sushitrack_it;

namespace
{

/**
 * @brief IoU of two TLWH output tracks (inclusive of the +1 convention is
 *        unnecessary here — a coarse overlap measure suffices for the invariant).
 */
float trackIoU(const sushitrack_track_t& a, const sushitrack_track_t& b)
{
    const float ax2 = a.x + a.width,  ay2 = a.y + a.height;
    const float bx2 = b.x + b.width,  by2 = b.y + b.height;
    const float ix1 = std::max(a.x, b.x), iy1 = std::max(a.y, b.y);
    const float ix2 = std::min(ax2, bx2), iy2 = std::min(ay2, by2);
    const float iw = std::max(0.0f, ix2 - ix1), ih = std::max(0.0f, iy2 - iy1);
    const float inter = iw * ih;
    const float uni = a.width * a.height + b.width * b.height - inter;
    return uni > 0.0f ? inter / uni : 0.0f;
}

/**
 * @brief Asserts no two simultaneously-Tracked output tracks overlap above 0.85.
 */
void expectNoOverlappingTrackedPair(const std::vector<sushitrack_track_t>& out, int n)
{
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j)
        {
            const sushitrack_track_t& a = out[static_cast<std::size_t>(i)];
            const sushitrack_track_t& b = out[static_cast<std::size_t>(j)];
            if (a.state == kTracked && b.state == kTracked)
                EXPECT_LE(trackIoU(a, b), 0.85f)
                    << "Two Tracked tracks (" << a.track_id << ", " << b.track_id
                    << ") survived with > 0.85 IoU — duplicate not suppressed.";
        }
}

/**
 * @brief Suppression fixture: core path, tentative off so an unmatched
 *        high-confidence detection seeds a track immediately.
 */
class DuplicateSuppressionTest : public ::testing::Test
{
protected:
    sushitrack_tracker_t     hdl_    = nullptr;
    sushitrack_params_t params_ = {};

    void SetUp() override
    {
        sushitrack_get_default_params(&params_);
        params_.time_aware           = 0;
        params_.enable_tentative     = 0;
        params_.primary_assoc_mode   = 1;
        params_.frame_rate           = 25;
        params_.track_buffer         = 30;
        params_.track_thresh         = 0.2f;
        params_.high_thresh          = 0.5f;
        params_.match_thresh         = 0.9f;
        params_.duplicate_iou_thresh = 0.15f;   // suppress when IoU > 0.85

        hdl_ = sushitrack_create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override { if (hdl_) sushitrack_destroy(hdl_); }
};

/**
 * @brief A confirmed track briefly split by a near-coincident second detection
 *        collapses back to one identity: the spurious twin is suppressed and a
 *        single Tracked track remains.
 */
TEST_F(DuplicateSuppressionTest, TransientDuplicateCollapsesToSingleIdentity)
{
    std::vector<sushitrack_track_t> out(8);

    // Confirm a single, long-lived track at a fixed location
    for (int i = 0; i < 6; ++i)
    {
        sushitrack_object_t d = makeDet(100.0f, 200.0f);
        int n = sushitrack_update(hdl_, &d, 1, out.data(), 8, 0.04f);
        ASSERT_GE(n, 1);
        expectNoOverlappingTrackedPair(out, n);
    }

    // One frame with a near-coincident twin (IoU ~0.89). The extra high-confidence detection seeds a second, younger track.
    {
        sushitrack_object_t d1 = makeDet(100.0f, 200.0f);
        sushitrack_object_t d2 = makeDet(103.0f, 200.0f);   // 3 px offset -> IoU > 0.85
        sushitrack_object_t dets[2] = { d1, d2 };
        int n = sushitrack_update(hdl_, dets, 2, out.data(), 8, 0.04f);
        ASSERT_GE(n, 1);
    }

    // Resume single detections. The unmatched twin falls to Lost and is removed as a duplicate of the surviving track within a few frames.
    int final_tracked = 0;
    for (int i = 0; i < 4; ++i)
    {
        sushitrack_object_t d = makeDet(100.0f, 200.0f);
        int n = sushitrack_update(hdl_, &d, 1, out.data(), 8, 0.04f);
        ASSERT_GE(n, 1);
        expectNoOverlappingTrackedPair(out, n);
        final_tracked = countTracked(out, n);
    }

    EXPECT_EQ(final_tracked, 1)
        << "A transient duplicate detection left a permanently duplicated identity.";
}

/**
 * @brief Two heavily overlapping detections presented on the very first frame do
 *        not establish two persistent identities for the same object: the
 *        duplicate suppressor converges the scene to a single Tracked track.
 */
TEST_F(DuplicateSuppressionTest, CoincidentDetectionsDoNotEstablishTwinIdentities)
{
    std::vector<sushitrack_track_t> out(8);

    // Seed with two near-identical detections, then feed a single detection.
    sushitrack_object_t s1 = makeDet(250.0f, 250.0f);
    sushitrack_object_t s2 = makeDet(253.0f, 250.0f);
    sushitrack_object_t seed[2] = { s1, s2 };
    int n0 = sushitrack_update(hdl_, seed, 2, out.data(), 8, 0.04f);
    ASSERT_GE(n0, 1);

    int final_tracked = 0;
    for (int i = 0; i < 6; ++i)
    {
        sushitrack_object_t d = makeDet(250.0f, 250.0f);
        int n = sushitrack_update(hdl_, &d, 1, out.data(), 8, 0.04f);
        ASSERT_GE(n, 1);
        expectNoOverlappingTrackedPair(out, n);
        final_tracked = countTracked(out, n);
    }

    EXPECT_EQ(final_tracked, 1)
        << "Coincident detections produced two lasting identities for one object.";
}

} // namespace
