/**
 * @file      test_track_fsm.cpp
 * @brief     Unit tests for TrackFSM: state transitions, hysteresis, coasting, and noise application.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      12.05.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 *
 * Coverage
 * --------
 * - Default state is Stationary after construction.
 * - Warmup window suppresses classification for the first kWarmupFrames frames.
 * - Stationary detection after warmup.
 * - Cruising transition above stationary threshold.
 * - Maneuvering transition when acceleration exceeds threshold for kStableFrames.
 * - Coasting assigned immediately when is_lost = true.
 * - Pre-Coasting state captured and restored on re-observation.
 * - Single-frame spike does not flip state (hysteresis).
 * - applyToPredictor sets correct multipliers for each kinematic state.
 * - applyToPredictor with null pointer is safe.
 */

#include <gtest/gtest.h>
#include "SushiTrack/TrackFSM.h"
#include "SushiTrack/TrackerConfig.h"
#include "SushiTrack/KalmanTypes.h"
#include "SushiTrack/IStatePredictor.h"

using namespace SushiTrack;

// ---------------------------------------------------------------------------
// Stub predictor — records the last setNoiseMultipliers call.
// ---------------------------------------------------------------------------

/**
 * @brief Minimal IStatePredictor stub that records noise multiplier arguments.
 */
class MockPredictor final : public IStatePredictor
{
public:
    float pos_mult  = -1.f; ///< Last position noise multiplier.
    float size_mult = -1.f; ///< Last size noise multiplier.
    float vel_mult  = -1.f; ///< Last velocity noise multiplier.

    void initiate(KalmanTypes::StateMean&, KalmanTypes::StateCov&,
                  const KalmanTypes::DetectBox&, float) override {}
    void predict(KalmanTypes::StateMean&, KalmanTypes::StateCov&, float) override {}
    void update(KalmanTypes::StateMean&, KalmanTypes::StateCov&,
                const KalmanTypes::DetectBox&, float) override {}

    void setNoiseMultipliers(float p, float s, float v) override
    {
        pos_mult  = p;
        size_mult = s;
        vel_mult  = v;
    }
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/** @brief Returns a TrackerConfig with FSM enabled, 25 fps, default thresholds. */
static TrackerConfig makeConfig()
{
    TrackerConfig cfg;
    cfg.enable_fsm                = true;
    cfg.frame_rate                = 25;
    cfg.time_aware                = false;
    cfg.fsm_stationary_vel_thresh = 40.0f;   // px/s
    cfg.fsm_maneuver_accel_thresh = 250.0f;  // px/s²
    return cfg;
}

/** @brief Returns a StateMean with zero velocity (stationary). */
static KalmanTypes::StateMean zeroMean()
{
    return KalmanTypes::StateMean::Zero();
}

/**
 * @brief Returns a StateMean with per-frame velocity components set.
 *
 * The Kalman state stores per-frame velocity. TrackFSM multiplies by frame_rate
 * to obtain per-second values. Pass pixel-per-frame values here.
 *
 * @param vx_pf  Per-frame x velocity in pixels/frame.
 * @param vy_pf  Per-frame y velocity in pixels/frame.
 */
static KalmanTypes::StateMean velMean(float vx_pf, float vy_pf = 0.f)
{
    KalmanTypes::StateMean m = KalmanTypes::StateMean::Zero();
    m(4) = vx_pf;
    m(5) = vy_pf;
    return m;
}

/**
 * @brief Drives the FSM through the 3-frame warmup window using the given mean.
 *
 * @param fsm   FSM under test.
 * @param mean  State mean to use during warmup.
 */
static void driveWarmup(TrackFSM& fsm, const KalmanTypes::StateMean& mean)
{
    for (size_t i = 1; i <= 3; ++i)
        fsm.updateState(mean, false, i, 0.04f);
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

/**
 * @brief Initial state is Stationary immediately after construction.
 */
TEST(TrackFSMTest, InitialStateIsStationary)
{
    TrackFSM fsm(makeConfig());
    EXPECT_EQ(fsm.getCurrentState(), KinematicState::Stationary);
}

// ---------------------------------------------------------------------------
// Warmup window
// ---------------------------------------------------------------------------

/**
 * @brief During warmup (tracklet_len < 3), the FSM reports Cruising regardless
 *        of velocity magnitude.
 */
TEST(TrackFSMTest, WarmupWindowForcesCruising)
{
    TrackFSM fsm(makeConfig());
    // 10 px/frame * 25 fps = 250 px/s — well above stationary threshold.
    KalmanTypes::StateMean fast = velMean(10.f);

    fsm.updateState(fast, false, 1, 0.04f);
    EXPECT_EQ(fsm.getCurrentState(), KinematicState::Cruising);

    fsm.updateState(fast, false, 2, 0.04f);
    EXPECT_EQ(fsm.getCurrentState(), KinematicState::Cruising);
}

// ---------------------------------------------------------------------------
// Stationary detection
// ---------------------------------------------------------------------------

/**
 * @brief Zero velocity (well below threshold) settles to Stationary after warmup.
 */
TEST(TrackFSMTest, StationaryAfterWarmup)
{
    TrackFSM fsm(makeConfig());
    driveWarmup(fsm, zeroMean());
    // Warmup produces Cruising; two more frames of zero velocity stabilise to Stationary.
    fsm.updateState(zeroMean(), false, 4, 0.04f);
    fsm.updateState(zeroMean(), false, 5, 0.04f);
    EXPECT_EQ(fsm.getCurrentState(), KinematicState::Stationary);
}

// ---------------------------------------------------------------------------
// Cruising detection
// ---------------------------------------------------------------------------

/**
 * @brief Velocity above stationary threshold and low acceleration transitions
 *        to Cruising within kStableFrames frames after warmup.
 */
TEST(TrackFSMTest, CruisingTransition)
{
    TrackFSM fsm(makeConfig());
    // 4 px/frame * 25 fps = 100 px/s — above stationary (40 px/s).
    KalmanTypes::StateMean cruising = velMean(4.f);
    driveWarmup(fsm, cruising);

    fsm.updateState(cruising, false, 4, 0.04f);
    fsm.updateState(cruising, false, 5, 0.04f);

    EXPECT_EQ(fsm.getCurrentState(), KinematicState::Cruising);
}

// ---------------------------------------------------------------------------
// Maneuvering detection
// ---------------------------------------------------------------------------

/**
 * @brief A large velocity step held for kStableFrames frames transitions to
 *        Maneuvering after warmup.
 *
 * Required acceleration: delta_v / dt > 250 * 1.2 px/s²
 * With dt = 0.04 s: delta_v > 12 px/s => 0.48 px/frame.
 * Using 20 px/frame for a clear margin.
 */
TEST(TrackFSMTest, ManeuveringTransition)
{
    TrackFSM fsm(makeConfig());
    KalmanTypes::StateMean slow = velMean(0.f);
    KalmanTypes::StateMean fast = velMean(20.f);  // 500 px/s

    driveWarmup(fsm, slow);

    fsm.updateState(fast, false, 4, 0.04f);
    fsm.updateState(fast, false, 5, 0.04f);

    EXPECT_EQ(fsm.getCurrentState(), KinematicState::Maneuvering);
}

// ---------------------------------------------------------------------------
// Coasting
// ---------------------------------------------------------------------------

/**
 * @brief is_lost = true immediately assigns Coasting regardless of velocity.
 */
TEST(TrackFSMTest, CoastingOnLost)
{
    TrackFSM fsm(makeConfig());
    KalmanTypes::StateMean cruising = velMean(4.f);
    driveWarmup(fsm, cruising);
    fsm.updateState(cruising, false, 4, 0.04f);
    fsm.updateState(cruising, false, 5, 0.04f);
    ASSERT_EQ(fsm.getCurrentState(), KinematicState::Cruising);

    fsm.updateState(cruising, /*is_lost=*/true, 6, 0.04f);
    EXPECT_EQ(fsm.getCurrentState(), KinematicState::Coasting);
}

// ---------------------------------------------------------------------------
// Pre-Coasting state capture and restore
// ---------------------------------------------------------------------------

/**
 * @brief getLastSeenState() returns the pre-Coasting state while Coasting.
 */
TEST(TrackFSMTest, LastSeenStateReturnedWhileCoasting)
{
    TrackFSM fsm(makeConfig());
    KalmanTypes::StateMean cruising = velMean(4.f);
    driveWarmup(fsm, cruising);
    fsm.updateState(cruising, false, 4, 0.04f);
    fsm.updateState(cruising, false, 5, 0.04f);
    ASSERT_EQ(fsm.getCurrentState(), KinematicState::Cruising);

    fsm.updateState(cruising, true, 6, 0.04f);
    EXPECT_EQ(fsm.getCurrentState(),  KinematicState::Coasting);
    EXPECT_EQ(fsm.getLastSeenState(), KinematicState::Cruising);
}

/**
 * @brief After recovery (is_lost=false while Coasting), the FSM restores the
 *        kinematic state active before occlusion.
 */
TEST(TrackFSMTest, StateRestoredAfterCoastingRecovery)
{
    TrackFSM fsm(makeConfig());
    KalmanTypes::StateMean cruising = velMean(4.f);
    driveWarmup(fsm, cruising);
    fsm.updateState(cruising, false, 4, 0.04f);
    fsm.updateState(cruising, false, 5, 0.04f);

    fsm.updateState(cruising, true,  6, 0.04f);
    ASSERT_EQ(fsm.getCurrentState(), KinematicState::Coasting);

    fsm.updateState(cruising, false, 7, 0.04f);
    EXPECT_EQ(fsm.getCurrentState(), KinematicState::Cruising);
}

/**
 * @brief Successive is_lost frames do not overwrite the captured pre-Coasting
 *        state (only the first transition into Coasting should snapshot it).
 */
TEST(TrackFSMTest, PreCoastingStateNotOverwrittenOnSuccessiveLostFrames)
{
    TrackFSM fsm(makeConfig());
    KalmanTypes::StateMean cruising = velMean(4.f);
    driveWarmup(fsm, cruising);
    fsm.updateState(cruising, false, 4, 0.04f);
    fsm.updateState(cruising, false, 5, 0.04f);
    ASSERT_EQ(fsm.getCurrentState(), KinematicState::Cruising);

    // Three consecutive lost frames
    for (size_t i = 6; i <= 8; ++i)
        fsm.updateState(cruising, true, i, 0.04f);

    // Recovering should still give back Cruising, not default Stationary.
    fsm.updateState(cruising, false, 9, 0.04f);
    EXPECT_EQ(fsm.getCurrentState(), KinematicState::Cruising);
}

// ---------------------------------------------------------------------------
// Hysteresis — single-frame spike
// ---------------------------------------------------------------------------

/**
 * @brief A single frame above the maneuver threshold does not flip a Cruising
 *        track (kStableFrames = 2 required for a committed transition).
 */
TEST(TrackFSMTest, SingleFrameSpikeDoesNotFlipState)
{
    TrackFSM fsm(makeConfig());
    KalmanTypes::StateMean cruising = velMean(4.f);
    driveWarmup(fsm, cruising);
    for (size_t i = 4; i <= 7; ++i)
        fsm.updateState(cruising, false, i, 0.04f);
    ASSERT_EQ(fsm.getCurrentState(), KinematicState::Cruising);

    KalmanTypes::StateMean spike = velMean(25.f);
    fsm.updateState(spike, false, 8, 0.04f);
    EXPECT_EQ(fsm.getCurrentState(), KinematicState::Cruising);
}

// ---------------------------------------------------------------------------
// applyToPredictor noise multipliers
// ---------------------------------------------------------------------------

/**
 * @brief Stationary state applies reduced noise multipliers (0.8, 0.6, 0.5).
 */
TEST(TrackFSMTest, ApplyToPredictorStationary)
{
    TrackFSM fsm(makeConfig());
    // Force Stationary through 5 zero-velocity frames
    for (size_t i = 1; i <= 5; ++i)
        fsm.updateState(zeroMean(), false, i, 0.04f);

    MockPredictor mock;
    fsm.applyToPredictor(&mock, 0.04f);
    EXPECT_FLOAT_EQ(mock.pos_mult,  0.8f);
    EXPECT_FLOAT_EQ(mock.size_mult, 0.6f);
    EXPECT_FLOAT_EQ(mock.vel_mult,  0.5f);
}

/**
 * @brief Cruising state applies identity noise multipliers (1.0, 1.0, 1.0).
 */
TEST(TrackFSMTest, ApplyToPredictorCruising)
{
    TrackFSM fsm(makeConfig());
    KalmanTypes::StateMean cruising = velMean(4.f);
    driveWarmup(fsm, cruising);
    for (size_t i = 4; i <= 7; ++i)
        fsm.updateState(cruising, false, i, 0.04f);
    ASSERT_EQ(fsm.getCurrentState(), KinematicState::Cruising);

    MockPredictor mock;
    fsm.applyToPredictor(&mock, 0.04f);
    EXPECT_FLOAT_EQ(mock.pos_mult,  1.0f);
    EXPECT_FLOAT_EQ(mock.size_mult, 1.0f);
    EXPECT_FLOAT_EQ(mock.vel_mult,  1.0f);
}

/**
 * @brief Maneuvering state applies elevated noise multipliers (1.5, 1.5, 2.0).
 */
TEST(TrackFSMTest, ApplyToPredictorManeuvering)
{
    TrackFSM fsm(makeConfig());
    KalmanTypes::StateMean slow = velMean(0.f);
    KalmanTypes::StateMean fast = velMean(20.f);
    driveWarmup(fsm, slow);
    fsm.updateState(fast, false, 4, 0.04f);
    fsm.updateState(fast, false, 5, 0.04f);
    ASSERT_EQ(fsm.getCurrentState(), KinematicState::Maneuvering);

    MockPredictor mock;
    fsm.applyToPredictor(&mock, 0.04f);
    EXPECT_FLOAT_EQ(mock.pos_mult,  1.5f);
    EXPECT_FLOAT_EQ(mock.size_mult, 1.5f);
    EXPECT_FLOAT_EQ(mock.vel_mult,  2.0f);
}

/**
 * @brief Coasting state applies elevated position/size noise (1.5, 1.5, 1.0).
 */
TEST(TrackFSMTest, ApplyToPredictorCoasting)
{
    TrackFSM fsm(makeConfig());
    KalmanTypes::StateMean cruising = velMean(4.f);
    driveWarmup(fsm, cruising);
    fsm.updateState(cruising, false, 4, 0.04f);
    fsm.updateState(cruising, false, 5, 0.04f);

    fsm.updateState(cruising, /*is_lost=*/true, 6, 0.04f);
    ASSERT_EQ(fsm.getCurrentState(), KinematicState::Coasting);

    MockPredictor mock;
    fsm.applyToPredictor(&mock, 0.04f);
    EXPECT_FLOAT_EQ(mock.pos_mult,  1.5f);
    EXPECT_FLOAT_EQ(mock.size_mult, 1.5f);
    EXPECT_FLOAT_EQ(mock.vel_mult,  1.0f);
}

/**
 * @brief applyToPredictor with a null pointer does not crash.
 */
TEST(TrackFSMTest, ApplyToPredictorNullIsSafe)
{
    TrackFSM fsm(makeConfig());
    EXPECT_NO_THROW(fsm.applyToPredictor(nullptr, 0.04f));
}
