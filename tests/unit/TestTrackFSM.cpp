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
#include "SushiTrack/Tracklet.h"
#include "SushiTrack/KalmanFilter.h"
#include "SushiTrack/Rect.h"

using namespace SushiTrack;

// Stub predictor — records the last setNoiseMultipliers call.

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

// Helpers

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

/**
 * @brief Reference box height used by the helpers.
 *
 * Equal to TrackerConfig::fsm_reference_height's default (100 px), so the
 * size-normalisation factor is exactly 1.0 and the px-unit thresholds in these
 * tests keep their literal meaning — i.e. the helpers exercise the FSM at the
 * reference scale where the normalised path is identical to the absolute path.
 * Tests that probe scale invariance pass an explicit, off-reference height.
 */
static constexpr float kRefHeight = 100.0f;

/** @brief Returns a StateMean with zero velocity (stationary) at the reference height. */
static KalmanTypes::StateMean zeroMean()
{
    KalmanTypes::StateMean m = KalmanTypes::StateMean::Zero();
    m(3) = kRefHeight;
    return m;
}

/**
 * @brief Returns a StateMean with per-frame velocity components and a box height.
 *
 * The Kalman state stores per-frame velocity. TrackFSM multiplies by frame_rate
 * to obtain per-second values. Pass pixel-per-frame values here. The height
 * (index 3) drives the size-normalisation factor; it defaults to the reference
 * so the thresholds keep their px meaning unless a test deliberately varies it.
 *
 * @param vx_pf  Per-frame x velocity in pixels/frame.
 * @param vy_pf  Per-frame y velocity in pixels/frame.
 * @param height Box height in pixels (state index 3).
 */
static KalmanTypes::StateMean velMean(float vx_pf, float vy_pf = 0.f, float height = kRefHeight)
{
    KalmanTypes::StateMean m = KalmanTypes::StateMean::Zero();
    m(3) = height;
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

// Construction

/**
 * @brief Initial state is Stationary immediately after construction.
 */
TEST(TrackFSMTest, InitialStateIsStationary)
{
    TrackFSM fsm(makeConfig());
    EXPECT_EQ(fsm.getCurrentState(), KinematicState::Stationary);
}

// Warmup window

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

// Stationary detection

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

// Cruising detection

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

// Maneuvering detection

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

// Coasting

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

// Pre-Coasting state capture and restore

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

// Hysteresis — single-frame spike

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

// applyToPredictor noise multipliers

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

/**
 * @brief When no FSM is attached to a Tracklet, getKinematicState() returns
 *        KinematicState::Cruising (the identity/fallback value) so that
 *        downstream policies degrade gracefully.
 */
TEST(TrackFSMTest, FsmDisabledTrackletReturnsCruisingFallback)
{
    auto t = std::make_unique<Tracklet>(
        std::make_unique<KalmanFilter>(),
        Rect<float>(100.f, 200.f, 50.f, 80.f),
        /*label=*/1, /*score=*/0.9f
    );
    ASSERT_EQ(t->getFSM(), nullptr);
    EXPECT_EQ(t->getKinematicState(), KinematicState::Cruising);
}

/**
 * @brief After transitioning to Maneuvering, driving the FSM with zero
 *        velocity for kStableFrames + 1 frames transitions back to Stationary.
 */
TEST(TrackFSMTest, ManeuveringToStationaryRecovery)
{
    TrackFSM fsm(makeConfig());
    KalmanTypes::StateMean slow = velMean(0.f);
    KalmanTypes::StateMean fast = velMean(20.f);

    driveWarmup(fsm, slow);
    fsm.updateState(fast, false, 4, 0.04f);
    fsm.updateState(fast, false, 5, 0.04f);
    ASSERT_EQ(fsm.getCurrentState(), KinematicState::Maneuvering);

    fsm.updateState(slow, false, 6, 0.04f);
    fsm.updateState(slow, false, 7, 0.04f);
    fsm.updateState(slow, false, 8, 0.04f);
    EXPECT_EQ(fsm.getCurrentState(), KinematicState::Stationary);
}

// Additional coverage requests

/**
 * @brief Verifies that the kinematic state remains unchanged when the FSM is disabled.
 *
 * Ensures that even with extreme velocity components that would normally
 * trigger a transition to Maneuvering, the state persists as the initial
 * Stationary when enable_fsm is configured to false.
 */
TEST(TrackFSMTest, DisabledFsmDoesNotChangeState)
{
    TrackerConfig config;
    config.enable_fsm = false;
    TrackFSM fsm(config);

    // Extreme velocity to normally trigger maneuvering
    KalmanTypes::StateMean mean = KalmanTypes::StateMean::Zero();
    mean(4) = 5000.0f;
    mean(5) = 5000.0f;

    fsm.updateState(mean, false, 10, 1.0f);

    EXPECT_EQ(fsm.getCurrentState(), KinematicState::Stationary);
}

/**
 * @brief Validates recovery from Maneuvering back to Stationary state.
 *
 * Simulates a high-acceleration event followed by a period of zero velocity.
 * Verifies that the FSM correctly identifies the stabilization and transitions
 * back to the Stationary state after the required stability window.
 */
TEST(TrackFSMTest, RecoversFromManeuveringToStationary) 
{
    TrackerConfig config; 
    config.enable_fsm = true;
    TrackFSM fsm(config);
    
    KalmanTypes::StateMean mean = KalmanTypes::StateMean::Zero();
    
    // Warmup: Hız geçmişini sıfır ile doldur
    fsm.updateState(mean, false, 1, 1.0f);
    fsm.updateState(mean, false, 2, 1.0f);
    fsm.updateState(mean, false, 3, 1.0f);
    fsm.updateState(mean, false, 4, 1.0f); 
    
    mean(4) = 1000.0f; // Yüksek Hız İvmelenmesi
    
    fsm.updateState(mean, false, 5, 1.0f); 
    fsm.updateState(mean, false, 6, 1.0f); 
    fsm.updateState(mean, false, 7, 1.0f); 
    
    ASSERT_EQ(fsm.getCurrentState(), KinematicState::Maneuvering);

    mean(4) = 0.0f; // Durma (Stationary'e geçiş)
    for(int i = 0; i < 15; ++i)
        fsm.updateState(mean, false, 8 + i, 1.0f);
    
    EXPECT_EQ(fsm.getCurrentState(), KinematicState::Stationary);
}

/**
 * @brief Confirms that Stationary state applies the expected noise multipliers.
 *
 * Verifies that when the FSM is in the Stationary state, the noise multipliers
 * applied to the state predictor match the configured reduction factors for
 * position, size, and velocity components.
 */
TEST(TrackFSMTest, ApplyToPredictorStationaryExactValuesVerify)
{
    TrackerConfig config = makeConfig();
    TrackFSM fsm(config);

    // Ensure state is Stationary (default on init)
    ASSERT_EQ(fsm.getCurrentState(), KinematicState::Stationary);

    MockPredictor mock;
    fsm.applyToPredictor(&mock, 1.0f);

    // Verify reduction multipliers for Stationary state (0.8, 0.6, 0.5)
    EXPECT_FLOAT_EQ(mock.pos_mult,  0.8f);
    EXPECT_FLOAT_EQ(mock.size_mult, 0.6f);
    EXPECT_FLOAT_EQ(mock.vel_mult,  0.5f);
}

// Scale-invariant classification: the FSM rescales velocity/accel by (fsm_reference_height / box height), so equal body-relative motion classifies identically regardless of pixel size. makeConfig() leaves fsm_reference_height at its 100 px kRefHeight default, so a kRefHeight box is at reference scale (factor == 1).

/**
 * @brief A slow-moving object settles to Stationary whether small or large,
 *        because the regime is decided on size-relative motion.
 *
 * Both objects move at 0.3 box-heights/s. In absolute pixels the large object
 * (120 px/s) sits well above the 40 px/s stationary threshold and would be
 * mislabelled Cruising; normalisation scales it back to the same 30 px/s as the
 * small object (vel_eff = 30 < 40*0.8), so both settle to Stationary.
 */
TEST(TrackFSMTest, ScaleInvariantSlowMotionIsStationaryRegardlessOfSize)
{
    auto settle_state = [](float vx_pf, float height) {
        TrackFSM fsm(makeConfig());
        KalmanTypes::StateMean m = velMean(vx_pf, 0.f, height);
        driveWarmup(fsm, m);
        fsm.updateState(m, false, 4, 0.04f);
        fsm.updateState(m, false, 5, 0.04f);
        return fsm.getCurrentState();
    };

    // Small: height 100, 30 px/s  -> 1.2 px/frame, vel_eff = 30.
    EXPECT_EQ(settle_state(1.2f, 100.0f), KinematicState::Stationary);
    // Large: height 400, 120 px/s -> 4.8 px/frame, vel_eff = 30 (same physical motion).
    EXPECT_EQ(settle_state(4.8f, 400.0f), KinematicState::Stationary);
}

/**
 * @brief Two objects with identical body-relative speed (1.0 height/s) land in
 *        the same regime (Cruising) across a 4x size difference.
 */
TEST(TrackFSMTest, ScaleInvariantCruisingMatchesAcrossSizes)
{
    auto settle_state = [](float vx_pf, float height) {
        TrackFSM fsm(makeConfig());
        KalmanTypes::StateMean m = velMean(vx_pf, 0.f, height);
        driveWarmup(fsm, m);
        fsm.updateState(m, false, 4, 0.04f);
        fsm.updateState(m, false, 5, 0.04f);
        return fsm.getCurrentState();
    };

    // Small: height 100, 100 px/s -> 4 px/frame,  vel_eff = 100.
    EXPECT_EQ(settle_state(4.0f, 100.0f), KinematicState::Cruising);
    // Large: height 400, 400 px/s -> 16 px/frame, vel_eff = 100 (same physical motion).
    EXPECT_EQ(settle_state(16.0f, 400.0f), KinematicState::Cruising);
}

/**
 * @brief A gentle, sustained acceleration that is sub-maneuver in body-relative
 *        terms keeps a large object in Cruising, even though its absolute pixel
 *        acceleration exceeds the maneuver threshold. The companion assertion
 *        shows the same ramp latches Maneuvering once normalisation is disabled.
 *
 * Ramp: +0.96 px/frame each frame => +24 px/s per 0.04 s step => 600 px/s^2.
 * For a 400 px box that is 150 px/s^2 effective (below 250) but 600 px/s^2
 * absolute (above 250 * 1.2).
 */
TEST(TrackFSMTest, ScaleInvariantGentleAccelStaysCruisingForLargeObject)
{
    auto run_ramp = [](float reference_height) {
        TrackerConfig cfg = makeConfig();
        cfg.fsm_reference_height = reference_height;
        TrackFSM fsm(cfg);

        const float height = 400.0f;
        float vx_pf = 8.0f;  // 200 px/s baseline (clearly Cruising)

        // Warmup at constant baseline to seed the velocity history at 200 px/s.
        KalmanTypes::StateMean base = velMean(vx_pf, 0.f, height);
        driveWarmup(fsm, base);

        // Ramp the velocity so per-frame acceleration holds at ~600 px/s^2.
        KinematicState last = fsm.getCurrentState();
        for (size_t f = 4; f <= 9; ++f)
        {
            vx_pf += 0.96f;
            KalmanTypes::StateMean m = velMean(vx_pf, 0.f, height);
            fsm.updateState(m, false, f, 0.04f);
            last = fsm.getCurrentState();
        }
        return last;
    };

    // Normalised (ref=100): 600 px/s^2 -> 150 px/s^2 effective -> stays Cruising.
    EXPECT_EQ(run_ramp(100.0f), KinematicState::Cruising);
    // Disabled (ref<=0): 600 px/s^2 absolute -> latches Maneuvering.
    EXPECT_EQ(run_ramp(0.0f), KinematicState::Maneuvering);
}

/**
 * @brief With normalisation disabled (fsm_reference_height <= 0) the large
 *        slow-mover is misclassified as Cruising — documenting the failure mode
 *        the feature corrects and the legacy-compatible escape hatch.
 */
TEST(TrackFSMTest, NormalizationDisabledMislabelsLargeSlowObjectAsCruising)
{
    TrackerConfig cfg = makeConfig();
    cfg.fsm_reference_height = 0.0f;  // disable -> absolute pixel thresholds

    TrackFSM fsm(cfg);

    // Large object, 120 px/s: above the 40 px/s stationary floor in absolute px.
    KalmanTypes::StateMean m = velMean(4.8f, 0.f, 400.0f);
    driveWarmup(fsm, m);
    fsm.updateState(m, false, 4, 0.04f);
    fsm.updateState(m, false, 5, 0.04f);

    EXPECT_EQ(fsm.getCurrentState(), KinematicState::Cruising);
}
