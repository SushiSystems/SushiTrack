/**************************************************************************/
/* test_lifecycle.cpp                                                     */
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
 * @file test_lifecycle.cpp
 * @brief Tests the lifecycle, motion and observation history of Tracklet.
 * @author Mustafa Garip
 */

#include <cmath>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/detection/rect.hpp>
#include <SushiTrack/filtering/kalman_filter.hpp>
#include <SushiTrack/filtering/state_predictor.hpp>
#include <SushiTrack/tracking/track_state.hpp>
#include <SushiTrack/tracking/tracklet.hpp>

using namespace SushiTrack;

// Helpers

/** @brief Creates a real KalmanFilter-backed StatePredictor. */
static std::unique_ptr<StatePredictor> make_kf()
{
    return std::make_unique<KalmanFilter>();
}

/**
 * @brief Constructs a Tracklet at (x, y, w, h) with given label and score.
 *
 * @param x      Top-left x.
 * @param y      Top-left y.
 * @param w      Width.
 */
static std::unique_ptr<Tracklet> make_tracklet(float x = 100.f, float y = 200.f, float w = 50.f,
                                               float h = 80.f, int label = 1, float score = 0.9f)
{
    return std::make_unique<Tracklet>(make_kf(), Rect<float>(x, y, w, h), label, score);
}

// Construction

/**
 * @brief A newly constructed Tracklet has state New, is not activated, and
 *        is not confirmed.
 */
TEST(Small_TrackletTest, ConstructionStateIsNew)
{
    auto t = make_tracklet();
    EXPECT_EQ(t->get_track_state(), TrackState::New);
    EXPECT_FALSE(t->is_activated());
    EXPECT_FALSE(t->is_confirmed());
}

// activate()

/**
 * @brief activate() at frame_id=1 sets Tracked, is_activated=true, is_confirmed=true.
 */
TEST(Small_TrackletTest, ActivateOnFrameOne)
{
    auto t = make_tracklet();
    t->activate(make_kf(), /*frame_id=*/1, /*track_id=*/42);

    EXPECT_EQ(t->get_track_state(), TrackState::Tracked);
    EXPECT_TRUE(t->is_activated());
    EXPECT_TRUE(t->is_confirmed());
    EXPECT_EQ(t->get_track_id(), static_cast<size_t>(42));
    EXPECT_EQ(t->get_frame_id(), static_cast<size_t>(1));
}

/**
 * @brief activate() at frame_id=5 sets Tracked but is_activated=false
 *        (implicit one-frame validation for non-first-frame tracks).
 */
TEST(Small_TrackletTest, ActivateOnLaterFrameIsNotActivated)
{
    auto t = make_tracklet();
    t->activate(make_kf(), /*frame_id=*/5, /*track_id=*/7);

    EXPECT_EQ(t->get_track_state(), TrackState::Tracked);
    EXPECT_FALSE(t->is_activated());
}

// activate_tentative()

/**
 * @brief activate_tentative() sets state to Tentative with is_activated=false
 *        and is_confirmed=false.
 */
TEST(Small_TrackletTest, ActivateTentativeState)
{
    auto t = make_tracklet();
    t->activate_tentative(make_kf(), /*frame_id=*/3, /*initial_id=*/0);

    EXPECT_EQ(t->get_track_state(), TrackState::Tentative);
    EXPECT_FALSE(t->is_activated());
    EXPECT_FALSE(t->is_confirmed());
}

// confirm()

/**
 * @brief confirm() promotes a Tentative track to Tracked with is_activated=true
 *        and assigns the final track id.
 */
TEST(Small_TrackletTest, ConfirmPromotesTentative)
{
    auto t = make_tracklet();
    t->activate_tentative(make_kf(), 3, 0);
    t->confirm(/*frame_id=*/5, /*final_id=*/99);

    EXPECT_EQ(t->get_track_state(), TrackState::Tracked);
    EXPECT_TRUE(t->is_activated());
    EXPECT_TRUE(t->is_confirmed());
    EXPECT_EQ(t->get_track_id(), static_cast<size_t>(99));
}

// mark_as_lost()

/**
 * @brief mark_as_lost() transitions the track to Lost and clears is_activated.
 */
TEST(Small_TrackletTest, MarkAsLostSetsLostState)
{
    auto t = make_tracklet();
    t->activate(make_kf(), 1, 10);
    ASSERT_TRUE(t->is_activated());

    t->mark_as_lost(0.04f);

    EXPECT_EQ(t->get_track_state(), TrackState::Lost);
    EXPECT_FALSE(t->is_activated());
}

/**
 * @brief mark_as_lost() initialises accumulated_lost_time with the dt argument.
 */
TEST(Small_TrackletTest, MarkAsLostInitialisesAccumulatedTime)
{
    auto t = make_tracklet();
    t->activate(make_kf(), 1, 1);
    t->mark_as_lost(0.04f);

    EXPECT_FLOAT_EQ(t->get_accumulated_lost_time(), 0.04f);
}

// mark_as_removed()

/**
 * @brief mark_as_removed() transitions the track to Removed.
 */
TEST(Small_TrackletTest, MarkAsRemovedSetsRemovedState)
{
    auto t = make_tracklet();
    t->activate(make_kf(), 1, 1);
    t->mark_as_removed();

    EXPECT_EQ(t->get_track_state(), TrackState::Removed);
    EXPECT_FALSE(t->is_activated());
}

// re_activate()

/**
 * @brief re_activate() recovers a Lost track back to Tracked with
 *        is_activated=true and resets accumulated_lost_time to 0.
 */
TEST(Small_TrackletTest, ReActivateRestoresTrackedState)
{
    auto t = make_tracklet();
    t->activate(make_kf(), 1, 10);
    t->mark_as_lost(0.04f);
    ASSERT_EQ(t->get_track_state(), TrackState::Lost);

    auto obs = make_tracklet(105.f, 205.f);
    t->re_activate(*obs, /*frame_id=*/8);

    EXPECT_EQ(t->get_track_state(), TrackState::Tracked);
    EXPECT_TRUE(t->is_activated());
    EXPECT_TRUE(t->is_confirmed());
    EXPECT_FLOAT_EQ(t->get_accumulated_lost_time(), 0.f);
}

// predict() — accumulated_lost_time

/**
 * @brief predict() increments accumulated_lost_time for a Lost track.
 */
TEST(Small_TrackletTest, PredictIncrementsLostTimeForLostTrack)
{
    auto t = make_tracklet();
    t->activate(make_kf(), 1, 1);
    t->mark_as_lost(0.0f);

    const float dt = 0.04f;
    t->predict(dt);

    EXPECT_NEAR(t->get_accumulated_lost_time(), dt, 1e-5f);
}

/**
 * @brief predict() does not change accumulated_lost_time for a Tracked track.
 */
TEST(Small_TrackletTest, PredictDoesNotIncrementLostTimeForTrackedTrack)
{
    auto t = make_tracklet();
    t->activate(make_kf(), 1, 1);
    ASSERT_EQ(t->get_accumulated_lost_time(), 0.f);

    t->predict(0.04f);

    EXPECT_FLOAT_EQ(t->get_accumulated_lost_time(), 0.f);
}

// update() keep_state flag

/**
 * @brief update() with keep_state=true does not change the TrackState.
 */
TEST(Small_TrackletTest, UpdateKeepStatePreservesTrackState)
{
    auto t = make_tracklet();
    t->activate_tentative(make_kf(), 3, 0);
    ASSERT_EQ(t->get_track_state(), TrackState::Tentative);

    auto obs = make_tracklet(102.f, 202.f);
    t->update(*obs, /*frame_id=*/4, /*update_features=*/false, /*keep_state=*/true);

    EXPECT_EQ(t->get_track_state(), TrackState::Tentative);
    EXPECT_FALSE(t->is_activated());
}

/**
 * @brief update() with keep_state=false promotes the track to Tracked.
 */
TEST(Small_TrackletTest, UpdateNoKeepStatePromotesToTracked)
{
    auto t = make_tracklet();
    t->activate(make_kf(), 3, 5);

    auto obs = make_tracklet(102.f, 202.f);
    t->update(*obs, /*frame_id=*/4, /*update_features=*/false, /*keep_state=*/false);

    EXPECT_EQ(t->get_track_state(), TrackState::Tracked);
    EXPECT_TRUE(t->is_activated());
}

// ReID — feature normalisation and similarity

/**
 * @brief update_features() L2-normalises the stored feature vector.
 */
TEST(Small_TrackletTest, UpdateFeaturesNormalisesVector)
{
    auto t = make_tracklet();
    std::vector<float> feat = {3.f, 4.f}; // L2 norm = 5
    t->update_features(feat);

    const auto& sf = t->get_smooth_feat();
    ASSERT_EQ(sf.size(), static_cast<size_t>(2));
    float norm = std::sqrt(sf[0] * sf[0] + sf[1] * sf[1]);
    EXPECT_NEAR(norm, 1.f, 1e-5f);
}

/**
 * @brief get_max_feature_similarity() returns ~1.0 for the same feature that was
 *        stored, and <0.5 for an orthogonal feature.
 */
TEST(Small_TrackletTest, FeatureSimilarityIdenticalVsOrthogonal)
{
    auto t = make_tracklet();
    std::vector<float> feat_a = {1.f, 0.f, 0.f};
    t->update_features(feat_a);

    // Normalise manually to compare
    std::vector<float> same = {1.f, 0.f, 0.f};
    float norm = std::sqrt(same[0] * same[0] + same[1] * same[1] + same[2] * same[2]);
    for (auto& v : same)
        v /= norm;

    EXPECT_NEAR(t->get_max_feature_similarity(same), 1.f, 1e-4f);

    std::vector<float> ortho = {0.f, 1.f, 0.f};
    float dot = t->get_max_feature_similarity(ortho);
    EXPECT_LT(dot, 0.1f);
}

/**
 * @brief Feature history is capped at max_feature_history; the oldest entry
 *        is evicted when the buffer is full.
 */
TEST(Small_TrackletTest, FeatureHistoryCapIsRespected)
{
    const int max_hist = 5;
    auto t = std::make_unique<Tracklet>(make_kf(), Rect<float>(0.f, 0.f, 50.f, 80.f),
                                        /*label=*/1, /*score=*/0.9f,
                                        /*feat=*/std::vector<float>{},
                                        /*alpha=*/0.9f,
                                        /*enable_xyah=*/false,
                                        /*max_feature_history=*/max_hist);

    for (int i = 0; i < max_hist + 3; ++i)
    {
        std::vector<float> f = {static_cast<float>(i), 0.f, 0.f};
        t->update_features(f);
    }

    EXPECT_LE(static_cast<int>(t->get_feature_history().size()), max_hist);
}

// Tier 3 edge cases

/**
 * @brief predict() increments accumulated_lost_time for a Tentative track.
 */
TEST(Small_TrackletTest, PredictIncrementsLostTimeForTentativeTrack)
{
    auto t = make_tracklet();
    t->activate_tentative(make_kf(), 1, 0);
    ASSERT_EQ(t->get_track_state(), TrackState::Tentative);

    const float dt = 0.04f;
    t->predict(dt);

    EXPECT_NEAR(t->get_accumulated_lost_time(), dt, 1e-5f);
}

/**
 * @brief activate_tentative() initialises accumulated_lost_time to zero.
 */
TEST(Small_TrackletTest, ActivateTentativeInitialisesLostTimeToZero)
{
    auto t = make_tracklet();
    t->activate_tentative(make_kf(), 3, 0);
    EXPECT_FLOAT_EQ(t->get_accumulated_lost_time(), 0.f);
}

/**
 * @brief mark_as_lost() called with dt=0 initialises accumulated_lost_time
 *        to zero rather than a garbage value.
 */
TEST(Small_TrackletTest, MarkAsLostWithZeroDtInitialisesToZero)
{
    auto t = make_tracklet();
    t->activate(make_kf(), 1, 1);
    t->mark_as_lost(0.0f);
    EXPECT_FLOAT_EQ(t->get_accumulated_lost_time(), 0.f);
}

/**
 * @brief re_activate() preserves the original track_id assigned at activation.
 */
TEST(Small_TrackletTest, ReActivatePreservesTrackId)
{
    auto t = make_tracklet();
    t->activate(make_kf(), 1, 77);
    ASSERT_EQ(t->get_track_id(), static_cast<size_t>(77));

    t->mark_as_lost(0.04f);
    auto obs = make_tracklet(105.f, 205.f);
    t->re_activate(*obs, 5);

    EXPECT_EQ(t->get_track_id(), static_cast<size_t>(77));
}

/**
 * @brief hits_ increments on each matched update call.
 */
TEST(Small_TrackletTest, HitsIncrementOnConsecutiveUpdates)
{
    auto t = make_tracklet();
    t->activate(make_kf(), 1, 1);
    ASSERT_EQ(t->get_hits(), 1);

    auto obs = make_tracklet(102.f, 202.f);
    t->update(*obs, 2, false, false);
    EXPECT_EQ(t->get_hits(), 2);

    auto obs2 = make_tracklet(104.f, 204.f);
    t->update(*obs2, 3, false, false);
    EXPECT_EQ(t->get_hits(), 3);
}

// Lost-time and State Transitions

/**
 * @brief Verifies that prediction increments the accumulated lost time for tracks
 *        in the Tentative state.
 *
 * Ensures that the temporal tracking of missing observations correctly accumulates
 * for non-activated tracks during prediction cycles.
 */
TEST(Small_TrackletTest, PredictIncrementsLostTimeForTentative)
{
    auto t = make_tracklet();
    t->activate_tentative(make_kf(), 1, 0);
    ASSERT_EQ(t->get_track_state(), TrackState::Tentative);

    const float dt = 1.0f;
    t->predict(dt);

    EXPECT_EQ(t->get_track_state(), TrackState::Tentative);
    EXPECT_NEAR(t->get_accumulated_lost_time(), dt, 1e-5f);
}

/**
 * @brief Confirms that the accumulated lost time is reset to zero upon
 * initial activation.
 *
 * Validates that transitioning from a tentative or newly created state to
 * an active tracking state clears any prior temporal accumulation of lost cycles.
 */
TEST(Small_TrackletTest, ActivateTentativeSetsLostTimeZero)
{
    auto t = make_tracklet();

    t->activate_tentative(make_kf(), /*frame_id=*/1, /*initial_id=*/0);

    EXPECT_EQ(t->get_track_state(), TrackState::Tentative);

    EXPECT_FLOAT_EQ(t->get_accumulated_lost_time(), 0.0f);
}

/**
 * @brief Verifies that marking a track as lost with a zero time interval
 *        correctly initializes the accumulated lost time.
 *
 * Ensures that boundary conditions for the time interval are handled correctly
 * during state transitions into the Lost state.
 */
TEST(Small_TrackletTest, MarkAsLostWithZeroDtInitialisesToZeroOverride)
{
    auto t = make_tracklet();
    t->activate(make_kf(), 1, 100);

    // Transition to lost with zero dt
    t->mark_as_lost(0.0f);

    EXPECT_EQ(t->get_track_state(), TrackState::Lost);
    EXPECT_FLOAT_EQ(t->get_accumulated_lost_time(), 0.0f);
}

// Observation-Centric Momentum: observation channel

/**
 * @brief Activation records the first raw observation but cannot yet derive a
 *        direction of travel from a single sample.
 */
TEST(Small_TrackletTest, ActivateRecordsObservationWithoutVelocity)
{
    auto t = make_tracklet(100.f, 200.f);
    t->activate(make_kf(), 1, 1);

    EXPECT_TRUE(t->has_last_observation());
    EXPECT_FALSE(t->has_obs_velocity());
    EXPECT_EQ(t->get_frames_since_observation(), 0);
}

/**
 * @brief Each unobserved predict() advances the frames-since-observation counter.
 */
TEST(Small_TrackletTest, PredictIncrementsFramesSinceObservation)
{
    auto t = make_tracklet();
    t->activate(make_kf(), 1, 1);
    ASSERT_EQ(t->get_frames_since_observation(), 0);

    t->predict(1.0f);
    EXPECT_EQ(t->get_frames_since_observation(), 1);
    t->predict(1.0f);
    EXPECT_EQ(t->get_frames_since_observation(), 2);
}

/**
 * @brief A matched update() resets the frames-since-observation counter to zero.
 */
TEST(Small_TrackletTest, UpdateResetsFramesSinceObservation)
{
    auto t = make_tracklet();
    t->activate(make_kf(), 1, 1);
    t->predict(1.0f);
    ASSERT_EQ(t->get_frames_since_observation(), 1);

    auto obs = make_tracklet(110.f, 200.f);
    t->update(*obs, 2, true, false);
    EXPECT_EQ(t->get_frames_since_observation(), 0);
}

/**
 * @brief The observed direction tracks horizontal motion: the unit vector points
 *        along +x and the speed equals the per-frame step.
 */
TEST(Small_TrackletTest, ObservationVelocityFollowsHorizontalMotion)
{
    auto t = make_tracklet(100.f, 200.f);
    t->activate(make_kf(), 1, 1);

    float x = 100.f;
    for (size_t f = 2; f <= 4; ++f)
    {
        t->predict(1.0f);
        x += 10.f;
        auto obs = make_tracklet(x, 200.f);
        t->update(*obs, f, true, false);
    }

    ASSERT_TRUE(t->has_obs_velocity());
    EXPECT_NEAR(t->get_obs_velocity_x(), 1.0f, 1e-3f);
    EXPECT_NEAR(t->get_obs_velocity_y(), 0.0f, 1e-3f);
    EXPECT_NEAR(t->get_obs_speed(), 10.0f, 1e-3f);
}

/**
 * @brief The observed direction is computed from raw observations, so reversing
 *        the motion reverses the sign of the direction vector.
 */
TEST(Small_TrackletTest, ObservationVelocitySignTracksDirection)
{
    auto t = make_tracklet(300.f, 200.f);
    t->activate(make_kf(), 1, 1);

    float x = 300.f;
    for (size_t f = 2; f <= 4; ++f)
    {
        t->predict(1.0f);
        x -= 8.f;
        auto obs = make_tracklet(x, 200.f);
        t->update(*obs, f, true, false);
    }

    ASSERT_TRUE(t->has_obs_velocity());
    EXPECT_LT(t->get_obs_velocity_x(), 0.0f);
    EXPECT_NEAR(t->get_obs_velocity_y(), 0.0f, 1e-3f);
}

/**
 * @brief get_last_observation() is the unfiltered detection box, not the
 *        Kalman-blended estimate; a detection placed far from the prediction is
 *        stored verbatim.
 */
TEST(Small_TrackletTest, LastObservationIsRawDetection)
{
    auto t = make_tracklet(100.f, 200.f);
    t->activate(make_kf(), 1, 1);
    t->predict(1.0f);

    auto obs = make_tracklet(140.f, 260.f);
    t->update(*obs, 2, true, false);

    const auto& lo = t->get_last_observation();
    EXPECT_FLOAT_EQ(lo.x(), 140.f);
    EXPECT_FLOAT_EQ(lo.y(), 260.f);
}
