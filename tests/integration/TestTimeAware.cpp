/****************************************************************/
/* TestTimeAware.cpp                                            */
/* SushiTrack - https://github.com/SushiSystems/SushiTrack      */
/* Copyright (c) 2026 Sushi Systems                             */
/* Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.    */
/* Commercial use requires a licence from Sushi Systems.        */
/****************************************************************/

/**
 * @file TestTimeAware.cpp
 * @brief Integration tests for time-aware real-time tracking.
 * @author Mustafa Garip
 */

#include <gtest/gtest.h>
#include "Helpers.h"

#include <vector>

using namespace sushitrack_it;

namespace
{

/**
 * @brief Wall-clock fixture: time_aware=1, 25 fps, 30-frame buffer.
 *
 * The retirement budget is therefore track_buffer / frame_rate = 30 / 25 = 1.2 s
 * of accumulated lost time. Tests express occlusion length in seconds of `dt`
 * and compare against that budget.
 */
class TimeAwareTest : public ::testing::Test
{
protected:
    sushitrack_tracker_t     hdl_    = nullptr;
    sushitrack_params_t params_ = {};

    static constexpr float kBudgetSeconds = 30.0f / 25.0f; ///< 1.2 s expiry budget.

    void SetUp() override
    {
        sushitrack_get_default_params(&params_);
        params_.time_aware         = 1;          // wall-clock timing
        params_.enable_tentative   = 0;
        params_.primary_assoc_mode = 1;
        params_.frame_rate         = 25;
        params_.track_buffer       = 30;
        params_.track_thresh       = 0.2f;
        params_.high_thresh        = 0.5f;
        params_.match_thresh       = 0.9f;

        hdl_ = sushitrack_create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override { if (hdl_) sushitrack_destroy(hdl_); }

    /**
     * @brief Confirms a stationary track at (x, y) over @p frames frames.
     * @return The confirmed track id.
     */
    int64_t confirmStationary(std::vector<sushitrack_track_t>& out,
                              float x, float y, int frames, float dt)
    {
        int64_t id = -1;
        for (int i = 0; i < frames; ++i)
        {
            sushitrack_object_t d = makeDet(x, y);
            int n = sushitrack_update(hdl_, &d, 1, out.data(), 8, dt);
            EXPECT_GE(n, 1);
            if (n >= 1) id = out[0].track_id;
        }
        return id;
    }
};

/**
 * @brief An occlusion whose accumulated wall time stays under the budget is
 *        bridged: the object recovers its original id when it reappears, even
 *        though the gap spans only a handful of large-`dt` frames.
 */
TEST_F(TimeAwareTest, SubBudgetRealTimeGapRecoversId)
{
    std::vector<sushitrack_track_t> out(8);

    const float x = 100.0f, y = 200.0f;
    const int64_t id = confirmStationary(out, x, y, /*frames=*/6, /*dt=*/0.04f);
    ASSERT_NE(id, -1);

    // Three empty frames at 0.2 s each => 0.6 s of lost time, well under 1.2 s.
    for (int i = 0; i < 3; ++i)
        sushitrack_update(hdl_, nullptr, 0, out.data(), 8, 0.2f);

    sushitrack_object_t back = makeDet(x, y);
    int n = sushitrack_update(hdl_, &back, 1, out.data(), 8, 0.2f);
    ASSERT_GE(n, 1);

    const sushitrack_track_t* recovered = findById(out, n, id);
    ASSERT_NE(recovered, nullptr)
        << "A sub-budget real-time occlusion (0.8 s < 1.2 s) lost the identity.";
    EXPECT_EQ(recovered->state, kTracked);
    EXPECT_TRUE(geometryIsSane(*recovered));
}

/**
 * @brief An occlusion whose accumulated wall time exceeds the budget retires the
 *        track: the object returns under a brand-new id and the stale one is
 *        gone — even though the gap is only a few `Update` calls.
 */
TEST_F(TimeAwareTest, OverBudgetRealTimeGapRetiresTrack)
{
    std::vector<sushitrack_track_t> out(8);

    const float x = 100.0f, y = 200.0f;
    const int64_t id = confirmStationary(out, x, y, /*frames=*/6, /*dt=*/0.04f);
    ASSERT_NE(id, -1);

    // Four empty frames at 0.5 s each => 2.0 s of lost time, past the 1.2 s budget.
    for (int i = 0; i < 4; ++i)
        sushitrack_update(hdl_, nullptr, 0, out.data(), 8, 0.5f);

    sushitrack_object_t back = makeDet(x, y);
    int n = sushitrack_update(hdl_, &back, 1, out.data(), 8, 0.5f);
    ASSERT_GE(n, 1);

    const sushitrack_track_t* stale = findById(out, n, id);
    EXPECT_EQ(stale, nullptr)
        << "A track lost for 2.0 s (> 1.2 s budget) was resurrected under its old id.";

    bool found_new = false;
    for (int i = 0; i < n; ++i)
        if (out[static_cast<std::size_t>(i)].track_id != id)
        {
            found_new = true;
            EXPECT_TRUE(geometryIsSane(out[static_cast<std::size_t>(i)]));
        }
    EXPECT_TRUE(found_new) << "Reappeared object produced no fresh track.";
}

/**
 * @brief Irregular inter-frame intervals (simulated jitter and dropped frames)
 *        do not churn identity: a steadily moving object keeps its id and sane
 *        geometry across a run of widely varying `dt` values.
 */
TEST_F(TimeAwareTest, IrregularFrameIntervalsKeepIdentityStable)
{
    std::vector<sushitrack_track_t> out(4);

    sushitrack_object_t d0 = makeDet(100.0f, 200.0f);
    int n0 = sushitrack_update(hdl_, &d0, 1, out.data(), 4, 0.04f);
    ASSERT_GE(n0, 1);
    int64_t id = out[0].track_id;

    // Constant real-world velocity of 100 px/s; position advances by v*dt so the detection always lands on the propagated prediction regardless of the gap.
    const float vx = 100.0f;
    const float dts[] = { 0.02f, 0.09f, 0.04f, 0.12f, 0.03f, 0.08f, 0.05f, 0.15f };
    float x = 100.0f;
    int k = 1;
    for (float dt : dts)
    {
        x += vx * dt;
        sushitrack_object_t d = makeDet(x, 200.0f);
        int n = sushitrack_update(hdl_, &d, 1, out.data(), 4, dt);
        ASSERT_GE(n, 1) << "Object dropped at irregular step " << k;

        if (out[0].state == kTracked) id = out[0].track_id;
        const sushitrack_track_t* t = findById(out, n, id);
        ASSERT_NE(t, nullptr) << "Identity churned at irregular step " << k
                              << " (dt=" << dt << ")";
        EXPECT_TRUE(geometryIsSane(*t)) << "Insane geometry at irregular step " << k;
        ++k;
    }
}

/**
 * @brief A pathologically large `dt` is clamped by the timing policy so the
 *        filter cannot diverge: after a 10 s stall the track remains finite and,
 *        once normal cadence resumes, continues tracking the object.
 */
TEST_F(TimeAwareTest, ExcessiveDtIsCappedAndGeometryStaysFinite)
{
    std::vector<sushitrack_track_t> out(4);

    const float x = 100.0f, y = 200.0f;
    const int64_t id = confirmStationary(out, x, y, /*frames=*/5, /*dt=*/0.04f);
    ASSERT_NE(id, -1);

    // One enormous interval with the object still present at the same spot. The 5-KF-step cap must keep the propagated covariance/state finite.
    sushitrack_object_t stall = makeDet(x, y);
    int n = sushitrack_update(hdl_, &stall, 1, out.data(), 4, 10.0f);
    ASSERT_GE(n, 1);
    const sushitrack_track_t* t = findById(out, n, id);
    ASSERT_NE(t, nullptr) << "Object lost across a capped large-dt frame.";
    EXPECT_TRUE(geometryIsSane(*t)) << "Large dt produced non-finite geometry.";

    // Normal cadence resumes cleanly.
    for (int i = 1; i <= 4; ++i)
    {
        sushitrack_object_t d = makeDet(x, y);
        int m = sushitrack_update(hdl_, &d, 1, out.data(), 4, 0.04f);
        ASSERT_GE(m, 1);
        const sushitrack_track_t* tt = findById(out, m, id);
        ASSERT_NE(tt, nullptr) << "Identity lost after the capped stall, step " << i;
        EXPECT_TRUE(geometryIsSane(*tt));
    }
}

} // namespace
