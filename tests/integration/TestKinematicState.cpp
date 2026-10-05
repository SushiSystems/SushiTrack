/****************************************************************/
/* TestKinematicState.cpp                                       */
/* SushiTrack - https://github.com/SushiSystems/SushiTrack      */
/* Copyright (c) 2026 Sushi Systems                             */
/* Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.    */
/* Commercial use requires a licence from Sushi Systems.        */
/****************************************************************/

/**
 * @file TestKinematicState.cpp
 * @brief Integration tests for kinematic state classification.
 * @author Mustafa Garip
 */

#include <gtest/gtest.h>
#include "Helpers.h"

#include <algorithm>
#include <vector>

using namespace sushitrack_it;

namespace
{

/** @brief Kinematic regime codes as reported in sushitrack_track_t::kinematic_state. */
enum KinematicCode
{
    kStationary  = 0,
    kCruising    = 1,
    kManeuvering = 2,
    kCoasting    = 3
};

/**
 * @brief IMM fixture: kalman_type=2 on the otherwise-core path.
 *
 * Frame-counted timing; the IMM mode bank produces the regime that is mapped to
 * the kinematic_state output (NCP mass -> Stationary, CA+CT mass -> Maneuvering,
 * otherwise Cruising, Lost -> Coasting).
 */
class KinematicStateTest : public ::testing::Test
{
protected:
    sushitrack_tracker_t     hdl_    = nullptr;
    sushitrack_params_t params_ = {};

    void SetUp() override
    {
        sushitrack_get_default_params(&params_);
        params_.time_aware         = 0;
        params_.enable_tentative   = 0;
        params_.kalman_type        = 2;   // IMM multi-model estimator
        params_.primary_assoc_mode = 1;
        params_.frame_rate         = 25;
        params_.track_buffer       = 30;
        params_.track_thresh       = 0.1f;   // keep fluctuating scores in the primary pool
        params_.high_thresh        = 0.5f;
        params_.match_thresh       = 0.9f;

        hdl_ = sushitrack_create(&params_);
        ASSERT_NE(hdl_, nullptr);
    }

    void TearDown() override { if (hdl_) sushitrack_destroy(hdl_); }
};

/**
 * @brief An object held at a fixed position settles to the Stationary regime
 *        once the estimator settles.
 */
TEST_F(KinematicStateTest, StationaryObjectReportsStationary)
{
    std::vector<sushitrack_track_t> out(4);

    int64_t id = -1;
    int last_state = -1;
    for (int i = 0; i < 10; ++i)
    {
        sushitrack_object_t d = makeDet(100.0f, 200.0f);  // never moves
        int n = sushitrack_update(hdl_, &d, 1, out.data(), 4, 0.04f);
        ASSERT_GE(n, 1);
        id = out[0].track_id;
        last_state = out[0].kinematic_state;
    }
    ASSERT_NE(id, -1);
    EXPECT_EQ(last_state, kStationary)
        << "A motionless object did not settle to the Stationary regime.";
}

/**
 * @brief An object that accelerates gently to a steady cruising speed and holds
 *        it reports the Cruising regime — moving faster than the stationary
 *        floor without ever crossing the maneuver ceiling.
 *
 * The velocity is ramped in small steps rather than applied as a single jump:
 */
TEST_F(KinematicStateTest, SteadyMotionReportsCruising)
{
    std::vector<sushitrack_track_t> out(4);

    // Ramp up to 5 px/frame (125 px/s) at 0.25 px/frame per frame, then hold. The ramp acceleration (~156 px/s^2) stays below the 250 px/s^2 maneuver threshold; the hold speed sits well above the 40 px/s stationary floor.
    float x = 100.0f, v = 0.0f;
    int64_t id = -1;
    int last_state = -1;
    for (int i = 0; i < 45; ++i)
    {
        v = std::min(5.0f, v + 0.25f);
        x += v;
        sushitrack_object_t d = makeDet(x, 200.0f);
        int n = sushitrack_update(hdl_, &d, 1, out.data(), 4, 0.04f);
        ASSERT_GE(n, 1);
        id = out[0].track_id;
        last_state = out[0].kinematic_state;
    }
    ASSERT_NE(id, -1);
    EXPECT_EQ(last_state, kCruising)
        << "A gently accelerating, then steady, object did not settle into Cruising.";
}

/**
 * @brief A stationary object that abruptly jerks into high-speed motion passes
 *        through the Maneuvering regime while the acceleration spike persists.
 */
TEST_F(KinematicStateTest, AbruptAccelerationReportsManeuvering)
{
    std::vector<sushitrack_track_t> out(4);

    // Settle stationary so the estimator has a zero-velocity baseline.
    for (int i = 0; i < 6; ++i)
    {
        sushitrack_object_t d = makeDet(100.0f, 200.0f);
        int n = sushitrack_update(hdl_, &d, 1, out.data(), 4, 0.04f);
        ASSERT_GE(n, 1);
    }

    // Sudden onset of motion at 15 px/frame (375 px/s). The step is small enough that the detection keeps overlapping the (in past tests stationary) predicted box, so the track stays associated rather than being dropped and reseeded; the Kalman velocity ramps up over several frames, sustaining an acceleration well above the 250 px/s^2 maneuver threshold long enough for the hysteresis window to commit the transition.
    float x = 100.0f;
    bool saw_maneuver = false;
    for (int i = 0; i < 8; ++i)
    {
        x += 15.0f;
        sushitrack_object_t d = makeDet(x, 200.0f);
        int n = sushitrack_update(hdl_, &d, 1, out.data(), 4, 0.04f);
        ASSERT_GE(n, 1);
        if (out[0].kinematic_state == kManeuvering)
            saw_maneuver = true;
    }
    EXPECT_TRUE(saw_maneuver)
        << "An abrupt high-acceleration event never entered the Maneuvering regime.";
}

/**
 * @brief A confirmed track that loses its detection enters the Coasting regime:
 *        the estimator reports Coasting on the Lost output track while it is propagated
 *        through the buffer without observations.
 */
TEST_F(KinematicStateTest, OccludedTrackReportsCoasting)
{
    std::vector<sushitrack_track_t> out(8);

    // Confirm a moving object so the pre-coast regime is a genuine motion state.
    float x = 100.0f;
    int64_t id = -1;
    for (int i = 0; i < 8; ++i)
    {
        sushitrack_object_t d = makeDet(x, 200.0f);
        int n = sushitrack_update(hdl_, &d, 1, out.data(), 8, 0.04f);
        ASSERT_GE(n, 1);
        id = out[0].track_id;
        x += 4.0f;
    }
    ASSERT_NE(id, -1);

    // Withhold detections; the track goes Lost and is propagated as Coasting.
    int coasting_state = -1;
    for (int i = 0; i < 3; ++i)
    {
        int n = sushitrack_update(hdl_, nullptr, 0, out.data(), 8, 0.04f);
        const sushitrack_track_t* t = findById(out, n, id);
        ASSERT_NE(t, nullptr) << "Lost track dropped from output during occlusion.";
        coasting_state = t->kinematic_state;
    }
    EXPECT_EQ(coasting_state, kCoasting)
        << "An occluded (Lost) track did not report the Coasting regime.";
}

} // namespace
