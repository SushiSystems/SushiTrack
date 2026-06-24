/**
 * @file      TestDuplicateSuppression.cpp
 * @brief     End-to-end coverage of redundant-tracklet (duplicate) suppression.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      23.06.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 *
 * Scope
 * -----
 * After every frame the tracker runs `removeDuplicateTracklets`, dropping a
 * track whose box nearly coincides (IoU > 1 - duplicate_iou_thresh, i.e. > 0.85
 * by default) with another, keeping the longer-lived one. This guards against
 * one physical object carrying two identities — the classic failure when a
 * second detection momentarily splits off, or a buffered Lost track lingers as a
 * ghost beside the live one. No end-to-end test exercises it; this file does,
 * purely through the C API.
 *
 * The contract is observable: a transient duplicate detection must not leave a
 * permanently duplicated identity. The track set converges back to a single
 * track, and at no point are two heavily overlapping tracks both reported as
 * Tracked once the suppressor has run.
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
float trackIoU(const SushiTrack_Track& a, const SushiTrack_Track& b)
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
void expectNoOverlappingTrackedPair(const std::vector<SushiTrack_Track>& out, int n)
{
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j)
        {
            const SushiTrack_Track& a = out[static_cast<std::size_t>(i)];
            const SushiTrack_Track& b = out[static_cast<std::size_t>(j)];
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
    SushiTrackHdl     hdl_    = nullptr;
    SushiTrack_Params params_ = {};

    void SetUp() override
    {
        SushiTrack_GetDefaultParams(&params_);
        params_.time_aware           = 0;
        params_.enable_tentative     = 0;
        params_.primary_assoc_mode   = 1;
        params_.frame_rate           = 25;
        params_.track_buffer         = 30;
        params_.track_thresh         = 0.2f;
        params_.high_thresh          = 0.5f;
        params_.match_thresh         = 0.9f;
        params_.duplicate_iou_thresh = 0.15f;   // suppress when IoU > 0.85

        hdl_ = SushiTrack_Create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override { if (hdl_) SushiTrack_Destroy(hdl_); }
};

/**
 * @brief A confirmed track briefly split by a near-coincident second detection
 *        collapses back to one identity: the spurious twin is suppressed and a
 *        single Tracked track remains.
 */
TEST_F(DuplicateSuppressionTest, TransientDuplicateCollapsesToSingleIdentity)
{
    std::vector<SushiTrack_Track> out(8);

    // Confirm a single, long-lived track at a fixed location
    for (int i = 0; i < 6; ++i)
    {
        SushiTrack_Object d = makeDet(100.0f, 200.0f);
        int n = SushiTrack_Update(hdl_, &d, 1, out.data(), 8, 0.04f);
        ASSERT_GE(n, 1);
        expectNoOverlappingTrackedPair(out, n);
    }

    // One frame with a near-coincident twin (IoU ~0.89). The extra high-confidence detection seeds a second, younger track.
    {
        SushiTrack_Object d1 = makeDet(100.0f, 200.0f);
        SushiTrack_Object d2 = makeDet(103.0f, 200.0f);   // 3 px offset -> IoU > 0.85
        SushiTrack_Object dets[2] = { d1, d2 };
        int n = SushiTrack_Update(hdl_, dets, 2, out.data(), 8, 0.04f);
        ASSERT_GE(n, 1);
    }

    // Resume single detections. The unmatched twin falls to Lost and is removed as a duplicate of the surviving track within a few frames.
    int final_tracked = 0;
    for (int i = 0; i < 4; ++i)
    {
        SushiTrack_Object d = makeDet(100.0f, 200.0f);
        int n = SushiTrack_Update(hdl_, &d, 1, out.data(), 8, 0.04f);
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
    std::vector<SushiTrack_Track> out(8);

    // Seed with two near-identical detections, then feed a single detection.
    SushiTrack_Object s1 = makeDet(250.0f, 250.0f);
    SushiTrack_Object s2 = makeDet(253.0f, 250.0f);
    SushiTrack_Object seed[2] = { s1, s2 };
    int n0 = SushiTrack_Update(hdl_, seed, 2, out.data(), 8, 0.04f);
    ASSERT_GE(n0, 1);

    int final_tracked = 0;
    for (int i = 0; i < 6; ++i)
    {
        SushiTrack_Object d = makeDet(250.0f, 250.0f);
        int n = SushiTrack_Update(hdl_, &d, 1, out.data(), 8, 0.04f);
        ASSERT_GE(n, 1);
        expectNoOverlappingTrackedPair(out, n);
        final_tracked = countTracked(out, n);
    }

    EXPECT_EQ(final_tracked, 1)
        << "Coincident detections produced two lasting identities for one object.";
}

} // namespace
