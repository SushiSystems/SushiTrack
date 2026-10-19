/**************************************************************************/
/* TestConfigVariations.cpp                                               */
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
 * @file TestConfigVariations.cpp
 * @brief Integration tests for varying tracker configurations.
 * @author Mustafa Garip
 */

#include <vector>

#include <gtest/gtest.h>

#include "Helpers.h"

using namespace sushitrack_it;

namespace
{

    /**
     * @brief Drives a single object on a smooth curved trajectory and asserts the
     *        id is stable and geometry sane for the whole run.
     *
     * Shared by every variation below so the differences between configs are the
     * only moving part.
     */
    void run_single_object_stable(sushitrack_params_t params, const char* label)
    {
        sushitrack_tracker_t hdl = sushitrack_create(&params);
        ASSERT_NE(hdl, nullptr) << "[" << label << "] tracker creation failed.";

        std::vector<sushitrack_track_t> out(4);

        sushitrack_object_t d0 = make_det(100.0f, 200.0f);
        int n0 = sushitrack_update(hdl, &d0, 1, out.data(), 4, 0.04f);
        ASSERT_GE(n0, 1) << "[" << label << "] failed to seed a track.";
        const int64_t id = out[0].track_id;

        for (int k = 1; k <= 30; ++k)
        {
            // Gentle curve: accelerating in x, sinusoidal-ish drift in y.
            const float x = 100.0f + 3.0f * k + 0.05f * k * k;
            const float y = 200.0f + 8.0f * std::sin(0.2f * k);
            sushitrack_object_t d = make_det(x, y);
            int n = sushitrack_update(hdl, &d, 1, out.data(), 4, 0.04f);
            ASSERT_GE(n, 1) << "[" << label << "] dropped the object at step " << k;

            const sushitrack_track_t* t = find_by_id(out, n, id);
            ASSERT_NE(t, nullptr) << "[" << label << "] id churned at step " << k;
            EXPECT_EQ(t->state, K_TRACKED) << "[" << label << "] left Tracked at step " << k;
            EXPECT_TRUE(geometry_is_sane(*t)) << "[" << label << "] insane geometry at step " << k;
        }

        sushitrack_destroy(hdl);
    }

    sushitrack_params_t base_params()
    {
        sushitrack_params_t p{};
        sushitrack_get_default_params(&p);
        p.time_aware = 0;
        p.enable_tentative = 0;
        p.primary_assoc_mode = 1;
        p.frame_rate = 25;
        p.track_buffer = 30;
        p.track_thresh = 0.2f;
        p.high_thresh = 0.5f;
        p.match_thresh = 0.9f;
        return p;
    }

    // IoU variant sweep

    /** @brief Parameterised over iou_type: 0=IoU, 1=DIoU, 2=GIoU, 3=SIoU. */
    class IoUVariantTest : public ::testing::TestWithParam<int>
    {
    };

    TEST_P(IoUVariantTest, EachIoUVariantTracksSingleObjectStably)
    {
        sushitrack_params_t p = base_params();
        p.iou_type = GetParam();
        run_single_object_stable(p, "iou_type");
    }

    INSTANTIATE_TEST_SUITE_P(AllVariants, IoUVariantTest, ::testing::Values(0, 1, 2, 3));

    // Coordinate parameterisation

    /**
     * @brief XYAH (centroid / aspect-ratio / height) mode tracks an object whose
     *        box changes aspect ratio over time without destabilising identity.
     */
    TEST(CoordinateSystemTest, XyahModeHandlesAspectChange)
    {
        sushitrack_params_t p = base_params();
        p.enable_xyah = 1;

        sushitrack_tracker_t hdl = sushitrack_create(&p);
        ASSERT_NE(hdl, nullptr);

        std::vector<sushitrack_track_t> out(4);
        sushitrack_object_t d0 = make_det(100.0f, 200.0f, 0.9f, 50.0f, 100.0f);
        int n0 = sushitrack_update(hdl, &d0, 1, out.data(), 4, 0.04f);
        ASSERT_GE(n0, 1);
        const int64_t id = out[0].track_id;

        // Grow the box (person stands up / approaches camera): width and height both increase, changing the aspect ratio the XYAH state tracks directly.
        for (int k = 1; k <= 20; ++k)
        {
            const float w = 50.0f + 1.5f * k;
            const float h = 100.0f + 2.0f * k;
            sushitrack_object_t d = make_det(100.0f + 3.0f * k, 200.0f, 0.9f, w, h);
            int n = sushitrack_update(hdl, &d, 1, out.data(), 4, 0.04f);
            ASSERT_GE(n, 1);
            const sushitrack_track_t* t = find_by_id(out, n, id);
            ASSERT_NE(t, nullptr) << "XYAH track churned at step " << k;
            EXPECT_TRUE(geometry_is_sane(*t));
        }

        sushitrack_destroy(hdl);
    }

    // Kalman filter variant

    /**
     * @brief The NSA (Noise Scale Adaptive) Kalman filter tracks an object whose
     *        detection confidence fluctuates frame to frame — the exact regime NSA
     *        is designed for — without losing identity.
     */
    TEST(KalmanVariantTest, NsaFilterHandlesFluctuatingConfidence)
    {
        sushitrack_params_t p = base_params();
        p.kalman_type = 1;     // NSA
        p.track_thresh = 0.1f; // keep mid/low scores in the primary pool

        sushitrack_tracker_t hdl = sushitrack_create(&p);
        ASSERT_NE(hdl, nullptr);

        std::vector<sushitrack_track_t> out(4);
        sushitrack_object_t d0 = make_det(100.0f, 200.0f, 0.95f);
        int n0 = sushitrack_update(hdl, &d0, 1, out.data(), 4, 0.04f);
        ASSERT_GE(n0, 1);
        const int64_t id = out[0].track_id;

        const float scores[] = {0.9f, 0.4f, 0.85f, 0.3f, 0.95f, 0.5f, 0.8f, 0.45f};
        int k = 1;
        for (float s : scores)
        {
            sushitrack_object_t d = make_det(100.0f + 4.0f * k, 200.0f + 1.0f * k, s);
            int n = sushitrack_update(hdl, &d, 1, out.data(), 4, 0.04f);
            ASSERT_GE(n, 1);
            const sushitrack_track_t* t = find_by_id(out, n, id);
            ASSERT_NE(t, nullptr) << "NSA track churned at step " << k << " (score " << s << ")";
            EXPECT_TRUE(geometry_is_sane(*t));
            ++k;
        }

        sushitrack_destroy(hdl);
    }

} // namespace
