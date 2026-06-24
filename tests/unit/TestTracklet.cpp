/**
 * @file      test_tracklet.cpp
 * @brief     Unit tests for Tracklet lifecycle, state transitions, ReID, and Tentative logic.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      12.05.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 *
 * Coverage
 * --------
 * Lifecycle
 *   - New state on construction.
 *   - activate() transitions to Tracked; isActivated=true only on frame 1.
 *   - activateTentative() produces Tentative state, is_activated=false.
 *   - confirm() promotes Tentative to Tracked and assigns final track id.
 *   - markAsLost() transitions to Lost, resets is_activated.
 *   - markAsRemoved() transitions to Removed.
 *   - reActivate() recovers Lost track back to Tracked.
 *
 * Prediction
 *   - predict() increments accumulated_lost_time for Lost/Tentative tracks.
 *   - predict() does not modify accumulated_lost_time for Tracked tracks.
 *
 * ReID
 *   - updateFeatures() normalises the feature vector (unit length).
 *   - getMaxFeatureSimilarity() returns 1.0 for identical feature.
 *   - getMaxFeatureSimilarity() returns < 1.0 for orthogonal feature.
 *   - Feature history is capped at max_feature_history.
 *
 * Tentative fast-path
 *   - update() with keep_state=true does not change TrackState.
 *   - update() with keep_state=false promotes to Tracked.
 */

#include <gtest/gtest.h>

#include "SushiTrack/Tracklet.h"
#include "SushiTrack/KalmanFilter.h"
#include "SushiTrack/KalmanTypes.h"
#include "SushiTrack/TrackerConfig.h"

#include <memory>
#include <vector>
#include <cmath>

using namespace SushiTrack;

// Helpers

/** @brief Creates a real KalmanFilter-backed IStatePredictor. */
static std::unique_ptr<IStatePredictor> makeKF()
{
    return std::make_unique<KalmanFilter>();
}

/**
 * @brief Constructs a Tracklet at (x, y, w, h) with given label and score.
 *
 * @param x      Top-left x.
 * @param y      Top-left y.
 * @param w      Width.
 * @param h      Height.
 * @param label  Class label.
 * @param score  Detection confidence.
 */
static std::unique_ptr<Tracklet> makeTracklet(
    float x = 100.f, float y = 200.f,
    float w = 50.f,  float h = 80.f,
    int label = 1,   float score = 0.9f)
{
    return std::make_unique<Tracklet>(
        makeKF(),
        Rect<float>(x, y, w, h),
        label, score
    );
}

// Construction

/**
 * @brief A newly constructed Tracklet has state New, is not activated, and
 *        is not confirmed.
 */
TEST(TrackletTest, ConstructionStateIsNew)
{
    auto t = makeTracklet();
    EXPECT_EQ(t->getTrackState(), TrackState::New);
    EXPECT_FALSE(t->isActivated());
    EXPECT_FALSE(t->isConfirmed());
}

// activate()

/**
 * @brief activate() at frame_id=1 sets Tracked, isActivated=true, isConfirmed=true.
 */
TEST(TrackletTest, ActivateOnFrameOne)
{
    auto t = makeTracklet();
    t->activate(makeKF(), /*frame_id=*/1, /*track_id=*/42);

    EXPECT_EQ(t->getTrackState(), TrackState::Tracked);
    EXPECT_TRUE(t->isActivated());
    EXPECT_TRUE(t->isConfirmed());
    EXPECT_EQ(t->getTrackId(), static_cast<size_t>(42));
    EXPECT_EQ(t->getFrameId(), static_cast<size_t>(1));
}

/**
 * @brief activate() at frame_id=5 sets Tracked but isActivated=false
 *        (implicit one-frame validation for non-first-frame tracks).
 */
TEST(TrackletTest, ActivateOnLaterFrameIsNotActivated)
{
    auto t = makeTracklet();
    t->activate(makeKF(), /*frame_id=*/5, /*track_id=*/7);

    EXPECT_EQ(t->getTrackState(), TrackState::Tracked);
    EXPECT_FALSE(t->isActivated());
}

// activateTentative()

/**
 * @brief activateTentative() sets state to Tentative with isActivated=false
 *        and isConfirmed=false.
 */
TEST(TrackletTest, ActivateTentativeState)
{
    auto t = makeTracklet();
    t->activateTentative(makeKF(), /*frame_id=*/3, /*initial_id=*/0);

    EXPECT_EQ(t->getTrackState(), TrackState::Tentative);
    EXPECT_FALSE(t->isActivated());
    EXPECT_FALSE(t->isConfirmed());
}

// confirm()

/**
 * @brief confirm() promotes a Tentative track to Tracked with isActivated=true
 *        and assigns the final track id.
 */
TEST(TrackletTest, ConfirmPromotesTentative)
{
    auto t = makeTracklet();
    t->activateTentative(makeKF(), 3, 0);
    t->confirm(/*frame_id=*/5, /*final_id=*/99);

    EXPECT_EQ(t->getTrackState(), TrackState::Tracked);
    EXPECT_TRUE(t->isActivated());
    EXPECT_TRUE(t->isConfirmed());
    EXPECT_EQ(t->getTrackId(), static_cast<size_t>(99));
}

// markAsLost()

/**
 * @brief markAsLost() transitions the track to Lost and clears isActivated.
 */
TEST(TrackletTest, MarkAsLostSetsLostState)
{
    auto t = makeTracklet();
    t->activate(makeKF(), 1, 10);
    ASSERT_TRUE(t->isActivated());

    t->markAsLost(0.04f);

    EXPECT_EQ(t->getTrackState(), TrackState::Lost);
    EXPECT_FALSE(t->isActivated());
}

/**
 * @brief markAsLost() initialises accumulated_lost_time with the dt argument.
 */
TEST(TrackletTest, MarkAsLostInitialisesAccumulatedTime)
{
    auto t = makeTracklet();
    t->activate(makeKF(), 1, 1);
    t->markAsLost(0.04f);

    EXPECT_FLOAT_EQ(t->getAccumulatedLostTime(), 0.04f);
}

// markAsRemoved()

/**
 * @brief markAsRemoved() transitions the track to Removed.
 */
TEST(TrackletTest, MarkAsRemovedSetsRemovedState)
{
    auto t = makeTracklet();
    t->activate(makeKF(), 1, 1);
    t->markAsRemoved();

    EXPECT_EQ(t->getTrackState(), TrackState::Removed);
    EXPECT_FALSE(t->isActivated());
}

// reActivate()

/**
 * @brief reActivate() recovers a Lost track back to Tracked with
 *        isActivated=true and resets accumulated_lost_time to 0.
 */
TEST(TrackletTest, ReActivateRestoresTrackedState)
{
    auto t = makeTracklet();
    t->activate(makeKF(), 1, 10);
    t->markAsLost(0.04f);
    ASSERT_EQ(t->getTrackState(), TrackState::Lost);

    auto obs = makeTracklet(105.f, 205.f);
    t->reActivate(*obs, /*frame_id=*/8);

    EXPECT_EQ(t->getTrackState(), TrackState::Tracked);
    EXPECT_TRUE(t->isActivated());
    EXPECT_TRUE(t->isConfirmed());
    EXPECT_FLOAT_EQ(t->getAccumulatedLostTime(), 0.f);
}

// predict() — accumulated_lost_time

/**
 * @brief predict() increments accumulated_lost_time for a Lost track.
 */
TEST(TrackletTest, PredictIncrementsLostTimeForLostTrack)
{
    auto t = makeTracklet();
    t->activate(makeKF(), 1, 1);
    t->markAsLost(0.0f);

    const float dt = 0.04f;
    t->predict(dt);

    EXPECT_NEAR(t->getAccumulatedLostTime(), dt, 1e-5f);
}

/**
 * @brief predict() does not change accumulated_lost_time for a Tracked track.
 */
TEST(TrackletTest, PredictDoesNotIncrementLostTimeForTrackedTrack)
{
    auto t = makeTracklet();
    t->activate(makeKF(), 1, 1);
    ASSERT_EQ(t->getAccumulatedLostTime(), 0.f);

    t->predict(0.04f);

    EXPECT_FLOAT_EQ(t->getAccumulatedLostTime(), 0.f);
}

// update() keep_state flag

/**
 * @brief update() with keep_state=true does not change the TrackState.
 */
TEST(TrackletTest, UpdateKeepStatePreservesTrackState)
{
    auto t = makeTracklet();
    t->activateTentative(makeKF(), 3, 0);
    ASSERT_EQ(t->getTrackState(), TrackState::Tentative);

    auto obs = makeTracklet(102.f, 202.f);
    t->update(*obs, /*frame_id=*/4, /*update_features=*/false, /*keep_state=*/true);

    EXPECT_EQ(t->getTrackState(), TrackState::Tentative);
    EXPECT_FALSE(t->isActivated());
}

/**
 * @brief update() with keep_state=false promotes the track to Tracked.
 */
TEST(TrackletTest, UpdateNoKeepStatePromotesToTracked)
{
    auto t = makeTracklet();
    t->activate(makeKF(), 3, 5);

    auto obs = makeTracklet(102.f, 202.f);
    t->update(*obs, /*frame_id=*/4, /*update_features=*/false, /*keep_state=*/false);

    EXPECT_EQ(t->getTrackState(), TrackState::Tracked);
    EXPECT_TRUE(t->isActivated());
}

// ReID — feature normalisation and similarity

/**
 * @brief updateFeatures() L2-normalises the stored feature vector.
 */
TEST(TrackletTest, UpdateFeaturesNormalisesVector)
{
    auto t = makeTracklet();
    std::vector<float> feat = {3.f, 4.f};  // L2 norm = 5
    t->updateFeatures(feat);

    const auto& sf = t->getSmoothFeat();
    ASSERT_EQ(sf.size(), static_cast<size_t>(2));
    float norm = std::sqrt(sf[0]*sf[0] + sf[1]*sf[1]);
    EXPECT_NEAR(norm, 1.f, 1e-5f);
}

/**
 * @brief getMaxFeatureSimilarity() returns ~1.0 for the same feature that was
 *        stored, and <0.5 for an orthogonal feature.
 */
TEST(TrackletTest, FeatureSimilarityIdenticalVsOrthogonal)
{
    auto t = makeTracklet();
    std::vector<float> feat_a = {1.f, 0.f, 0.f};
    t->updateFeatures(feat_a);

    // Normalise manually to compare
    std::vector<float> same = {1.f, 0.f, 0.f};
    float norm = std::sqrt(same[0]*same[0] + same[1]*same[1] + same[2]*same[2]);
    for (auto& v : same) v /= norm;

    EXPECT_NEAR(t->getMaxFeatureSimilarity(same), 1.f, 1e-4f);

    std::vector<float> ortho = {0.f, 1.f, 0.f};
    float dot = t->getMaxFeatureSimilarity(ortho);
    EXPECT_LT(dot, 0.1f);
}

/**
 * @brief Feature history is capped at max_feature_history; the oldest entry
 *        is evicted when the buffer is full.
 */
TEST(TrackletTest, FeatureHistoryCapIsRespected)
{
    const int max_hist = 5;
    auto t = std::make_unique<Tracklet>(
        makeKF(),
        Rect<float>(0.f, 0.f, 50.f, 80.f),
        /*label=*/1, /*score=*/0.9f,
        /*feat=*/std::vector<float>{},
        /*alpha=*/0.9f,
        /*enable_xyah=*/false,
        /*max_feature_history=*/max_hist
    );

    for (int i = 0; i < max_hist + 3; ++i)
    {
        std::vector<float> f = {static_cast<float>(i), 0.f, 0.f};
        t->updateFeatures(f);
    }

    EXPECT_LE(static_cast<int>(t->getFeatureHistory().size()), max_hist);
}

// Tier 3 edge cases

/**
 * @brief predict() increments accumulated_lost_time for a Tentative track.
 */
TEST(TrackletTest, PredictIncrementsLostTimeForTentativeTrack)
{
    auto t = makeTracklet();
    t->activateTentative(makeKF(), 1, 0);
    ASSERT_EQ(t->getTrackState(), TrackState::Tentative);

    const float dt = 0.04f;
    t->predict(dt);

    EXPECT_NEAR(t->getAccumulatedLostTime(), dt, 1e-5f);
}

/**
 * @brief activateTentative() initialises accumulated_lost_time to zero.
 */
TEST(TrackletTest, ActivateTentativeInitialisesLostTimeToZero)
{
    auto t = makeTracklet();
    t->activateTentative(makeKF(), 3, 0);
    EXPECT_FLOAT_EQ(t->getAccumulatedLostTime(), 0.f);
}

/**
 * @brief markAsLost() called with dt=0 initialises accumulated_lost_time
 *        to zero rather than a garbage value.
 */
TEST(TrackletTest, MarkAsLostWithZeroDtInitialisesToZero)
{
    auto t = makeTracklet();
    t->activate(makeKF(), 1, 1);
    t->markAsLost(0.0f);
    EXPECT_FLOAT_EQ(t->getAccumulatedLostTime(), 0.f);
}

/**
 * @brief reActivate() preserves the original track_id assigned at activation.
 */
TEST(TrackletTest, ReActivatePreservesTrackId)
{
    auto t = makeTracklet();
    t->activate(makeKF(), 1, 77);
    ASSERT_EQ(t->getTrackId(), static_cast<size_t>(77));

    t->markAsLost(0.04f);
    auto obs = makeTracklet(105.f, 205.f);
    t->reActivate(*obs, 5);

    EXPECT_EQ(t->getTrackId(), static_cast<size_t>(77));
}

/**
 * @brief hits_ increments on each matched update call.
 */
TEST(TrackletTest, HitsIncrementOnConsecutiveUpdates)
{
    auto t = makeTracklet();
    t->activate(makeKF(), 1, 1);
    ASSERT_EQ(t->getHits(), 1);

    auto obs = makeTracklet(102.f, 202.f);
    t->update(*obs, 2, false, false);
    EXPECT_EQ(t->getHits(), 2);

    auto obs2 = makeTracklet(104.f, 204.f);
    t->update(*obs2, 3, false, false);
    EXPECT_EQ(t->getHits(), 3);
}

// Lost-time and State Transitions

/**
 * @brief Verifies that prediction increments the accumulated lost time for tracks 
 *        in the Tentative state.
 *
 * Ensures that the temporal tracking of missing observations correctly accumulates
 * for non-activated tracks during prediction cycles.
 */
TEST(TrackletTest, PredictIncrementsLostTimeForTentative)
{
    auto t = makeTracklet();
    t->activateTentative(makeKF(), 1, 0);
    ASSERT_EQ(t->getTrackState(), TrackState::Tentative);

    const float dt = 1.0f;
    t->predict(dt);

    EXPECT_EQ(t->getTrackState(), TrackState::Tentative);
    EXPECT_NEAR(t->getAccumulatedLostTime(), dt, 1e-5f);
}

/**
 * @brief Confirms that the accumulated lost time is reset to zero upon 
 * initial activation.
 *
 * Validates that transitioning from a tentative or newly created state to
 * an active tracking state clears any prior temporal accumulation of lost cycles.
 */
TEST(TrackletTest, ActivateTentativeSetsLostTimeZero) 
{
    auto t = makeTracklet();

    t->activateTentative(makeKF(), /*frame_id=*/1, /*initial_id=*/0);
    
    EXPECT_EQ(t->getTrackState(), TrackState::Tentative);
    
    EXPECT_FLOAT_EQ(t->getAccumulatedLostTime(), 0.0f);
}

/**
 * @brief Verifies that marking a track as lost with a zero time interval 
 *        correctly initializes the accumulated lost time.
 *
 * Ensures that boundary conditions for the time interval are handled correctly
 * during state transitions into the Lost state.
 */
TEST(TrackletTest, MarkAsLostWithZeroDtInitialisesToZeroOverride)
{
    auto t = makeTracklet();
    t->activate(makeKF(), 1, 100);

    // Transition to lost with zero dt
    t->markAsLost(0.0f);

    EXPECT_EQ(t->getTrackState(), TrackState::Lost);
    EXPECT_FLOAT_EQ(t->getAccumulatedLostTime(), 0.0f);
}

// Observation-Centric Momentum: observation channel

/**
 * @brief Activation records the first raw observation but cannot yet derive a
 *        direction of travel from a single sample.
 */
TEST(TrackletTest, ActivateRecordsObservationWithoutVelocity)
{
    auto t = makeTracklet(100.f, 200.f);
    t->activate(makeKF(), 1, 1);

    EXPECT_TRUE(t->hasLastObservation());
    EXPECT_FALSE(t->hasObsVelocity());
    EXPECT_EQ(t->getFramesSinceObservation(), 0);
}

/**
 * @brief Each unobserved predict() advances the frames-since-observation counter.
 */
TEST(TrackletTest, PredictIncrementsFramesSinceObservation)
{
    auto t = makeTracklet();
    t->activate(makeKF(), 1, 1);
    ASSERT_EQ(t->getFramesSinceObservation(), 0);

    t->predict(1.0f);
    EXPECT_EQ(t->getFramesSinceObservation(), 1);
    t->predict(1.0f);
    EXPECT_EQ(t->getFramesSinceObservation(), 2);
}

/**
 * @brief A matched update() resets the frames-since-observation counter to zero.
 */
TEST(TrackletTest, UpdateResetsFramesSinceObservation)
{
    auto t = makeTracklet();
    t->activate(makeKF(), 1, 1);
    t->predict(1.0f);
    ASSERT_EQ(t->getFramesSinceObservation(), 1);

    auto obs = makeTracklet(110.f, 200.f);
    t->update(*obs, 2, true, false);
    EXPECT_EQ(t->getFramesSinceObservation(), 0);
}

/**
 * @brief The observed direction tracks horizontal motion: the unit vector points
 *        along +x and the speed equals the per-frame step.
 */
TEST(TrackletTest, ObservationVelocityFollowsHorizontalMotion)
{
    auto t = makeTracklet(100.f, 200.f);
    t->activate(makeKF(), 1, 1);

    float x = 100.f;
    for (size_t f = 2; f <= 4; ++f)
    {
        t->predict(1.0f);
        x += 10.f;
        auto obs = makeTracklet(x, 200.f);
        t->update(*obs, f, true, false);
    }

    ASSERT_TRUE(t->hasObsVelocity());
    EXPECT_NEAR(t->getObsVelocityX(), 1.0f, 1e-3f);
    EXPECT_NEAR(t->getObsVelocityY(), 0.0f, 1e-3f);
    EXPECT_NEAR(t->getObsSpeed(), 10.0f, 1e-3f);
}

/**
 * @brief The observed direction is computed from raw observations, so reversing
 *        the motion reverses the sign of the direction vector.
 */
TEST(TrackletTest, ObservationVelocitySignTracksDirection)
{
    auto t = makeTracklet(300.f, 200.f);
    t->activate(makeKF(), 1, 1);

    float x = 300.f;
    for (size_t f = 2; f <= 4; ++f)
    {
        t->predict(1.0f);
        x -= 8.f;
        auto obs = makeTracklet(x, 200.f);
        t->update(*obs, f, true, false);
    }

    ASSERT_TRUE(t->hasObsVelocity());
    EXPECT_LT(t->getObsVelocityX(), 0.0f);
    EXPECT_NEAR(t->getObsVelocityY(), 0.0f, 1e-3f);
}

/**
 * @brief getLastObservation() is the unfiltered detection box, not the
 *        Kalman-blended estimate; a detection placed far from the prediction is
 *        stored verbatim.
 */
TEST(TrackletTest, LastObservationIsRawDetection)
{
    auto t = makeTracklet(100.f, 200.f);
    t->activate(makeKF(), 1, 1);
    t->predict(1.0f);

    auto obs = makeTracklet(140.f, 260.f);
    t->update(*obs, 2, true, false);

    const auto& lo = t->getLastObservation();
    EXPECT_FLOAT_EQ(lo.x(), 140.f);
    EXPECT_FLOAT_EQ(lo.y(), 260.f);
}

