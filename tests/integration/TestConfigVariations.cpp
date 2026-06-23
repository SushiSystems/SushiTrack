/**
 * @file      TestConfigVariations.cpp
 * @brief     End-to-end robustness across interchangeable algorithm components.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      15.06.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 *
 * Scope
 * -----
 * The tracker exposes several swappable strategies via configuration: four IoU
 * cost variants (IoU / DIoU / GIoU / SIoU), two coordinate parameterisations
 * (XYWH / XYAH), and two Kalman filters (linear / NSA confidence-adaptive).
 * Each is unit-tested in isolation; this file proves every one of them drives a
 * full multi-frame tracking run through the C API without destabilising
 * identity or producing degenerate geometry. These are the substitutions a
 * downstream integrator is most likely to flip, so each must be exercised
 * end-to-end.
 */

#include <gtest/gtest.h>
#include "Helpers.h"

#include <vector>

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
void runSingleObjectStable(SushiTrack_Params params, const char* label)
{
    SushiTrackHdl hdl = SushiTrack_Create(&params);
    ASSERT_NE(hdl, nullptr) << "[" << label << "] tracker creation failed.";

    std::vector<SushiTrack_Track> out(4);

    SushiTrack_Object d0 = makeDet(100.0f, 200.0f);
    int n0 = SushiTrack_Update(hdl, &d0, 1, out.data(), 4, 0.04f);
    ASSERT_GE(n0, 1) << "[" << label << "] failed to seed a track.";
    const int64_t id = out[0].track_id;

    for (int k = 1; k <= 30; ++k)
    {
        // Gentle curve: accelerating in x, sinusoidal-ish drift in y.
        const float x = 100.0f + 3.0f * k + 0.05f * k * k;
        const float y = 200.0f + 8.0f * std::sin(0.2f * k);
        SushiTrack_Object d = makeDet(x, y);
        int n = SushiTrack_Update(hdl, &d, 1, out.data(), 4, 0.04f);
        ASSERT_GE(n, 1) << "[" << label << "] dropped the object at step " << k;

        const SushiTrack_Track* t = findById(out, n, id);
        ASSERT_NE(t, nullptr) << "[" << label << "] id churned at step " << k;
        EXPECT_EQ(t->state, kTracked) << "[" << label << "] left Tracked at step " << k;
        EXPECT_TRUE(geometryIsSane(*t)) << "[" << label << "] insane geometry at step " << k;
    }

    SushiTrack_Destroy(hdl);
}

SushiTrack_Params baseParams()
{
    SushiTrack_Params p{};
    SushiTrack_GetDefaultParams(&p);
    p.time_aware         = 0;
    p.enable_tentative   = 0;
    p.primary_assoc_mode = 1;
    p.frame_rate         = 25;
    p.track_buffer       = 30;
    p.track_thresh       = 0.2f;
    p.high_thresh        = 0.5f;
    p.match_thresh       = 0.9f;
    return p;
}

// ---------------------------------------------------------------------------
// IoU variant sweep
// ---------------------------------------------------------------------------

/** @brief Parameterised over iou_type: 0=IoU, 1=DIoU, 2=GIoU, 3=SIoU. */
class IoUVariantTest : public ::testing::TestWithParam<int> {};

TEST_P(IoUVariantTest, EachIoUVariantTracksSingleObjectStably)
{
    SushiTrack_Params p = baseParams();
    p.iou_type = GetParam();
    runSingleObjectStable(p, "iou_type");
}

INSTANTIATE_TEST_SUITE_P(AllVariants, IoUVariantTest,
                         ::testing::Values(0, 1, 2, 3));

// ---------------------------------------------------------------------------
// Coordinate parameterisation
// ---------------------------------------------------------------------------

/**
 * @brief XYAH (centroid / aspect-ratio / height) mode tracks an object whose
 *        box changes aspect ratio over time without destabilising identity.
 */
TEST(CoordinateSystemTest, XyahModeHandlesAspectChange)
{
    SushiTrack_Params p = baseParams();
    p.enable_xyah = 1;

    SushiTrackHdl hdl = SushiTrack_Create(&p);
    ASSERT_NE(hdl, nullptr);

    std::vector<SushiTrack_Track> out(4);
    SushiTrack_Object d0 = makeDet(100.0f, 200.0f, 0.9f, 50.0f, 100.0f);
    int n0 = SushiTrack_Update(hdl, &d0, 1, out.data(), 4, 0.04f);
    ASSERT_GE(n0, 1);
    const int64_t id = out[0].track_id;

    // Grow the box (person stands up / approaches camera): width and height both
    // increase, changing the aspect ratio the XYAH state tracks directly.
    for (int k = 1; k <= 20; ++k)
    {
        const float w = 50.0f + 1.5f * k;
        const float h = 100.0f + 2.0f * k;
        SushiTrack_Object d = makeDet(100.0f + 3.0f * k, 200.0f, 0.9f, w, h);
        int n = SushiTrack_Update(hdl, &d, 1, out.data(), 4, 0.04f);
        ASSERT_GE(n, 1);
        const SushiTrack_Track* t = findById(out, n, id);
        ASSERT_NE(t, nullptr) << "XYAH track churned at step " << k;
        EXPECT_TRUE(geometryIsSane(*t));
    }

    SushiTrack_Destroy(hdl);
}

// ---------------------------------------------------------------------------
// Kalman filter variant
// ---------------------------------------------------------------------------

/**
 * @brief The NSA (Noise Scale Adaptive) Kalman filter tracks an object whose
 *        detection confidence fluctuates frame to frame — the exact regime NSA
 *        is designed for — without losing identity.
 */
TEST(KalmanVariantTest, NsaFilterHandlesFluctuatingConfidence)
{
    SushiTrack_Params p = baseParams();
    p.kalman_type  = 1;     // NSA
    p.track_thresh = 0.1f;  // keep mid/low scores in the primary pool

    SushiTrackHdl hdl = SushiTrack_Create(&p);
    ASSERT_NE(hdl, nullptr);

    std::vector<SushiTrack_Track> out(4);
    SushiTrack_Object d0 = makeDet(100.0f, 200.0f, 0.95f);
    int n0 = SushiTrack_Update(hdl, &d0, 1, out.data(), 4, 0.04f);
    ASSERT_GE(n0, 1);
    const int64_t id = out[0].track_id;

    const float scores[] = { 0.9f, 0.4f, 0.85f, 0.3f, 0.95f, 0.5f, 0.8f, 0.45f };
    int k = 1;
    for (float s : scores)
    {
        SushiTrack_Object d = makeDet(100.0f + 4.0f * k, 200.0f + 1.0f * k, s);
        int n = SushiTrack_Update(hdl, &d, 1, out.data(), 4, 0.04f);
        ASSERT_GE(n, 1);
        const SushiTrack_Track* t = findById(out, n, id);
        ASSERT_NE(t, nullptr) << "NSA track churned at step " << k << " (score " << s << ")";
        EXPECT_TRUE(geometryIsSane(*t));
        ++k;
    }

    SushiTrack_Destroy(hdl);
}

} // namespace
