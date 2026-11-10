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
/* Licensed under the Apache License, Version 2.0 (the "License"); you    */
/* may not use this file except in compliance with the License. You may   */
/* obtain a copy of the License at                                        */
/*                                                                        */
/*     https://www.apache.org/licenses/LICENSE-2.0                        */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/**************************************************************************/

/**
 * @file test_lifecycle.cpp
 * @brief Tests the lifecycle, motion and observation history of Tracklet.
 * @author Mustafa Garip
 */

#include <cstddef>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/detection/rect.hpp>
#include <SushiTrack/filtering/imm_filter.hpp>
#include <SushiTrack/filtering/kalman_filter.hpp>
#include <SushiTrack/filtering/state_predictor.hpp>
#include <SushiTrack/tracking/kinematic_state.hpp>
#include <SushiTrack/tracking/track_state.hpp>
#include <SushiTrack/tracking/tracklet.hpp>

using namespace SushiTrack;

namespace
{
    /** @brief Creates a real KalmanFilter-backed StatePredictor. */
    std::unique_ptr<StatePredictor> make_kf()
    {
        return std::make_unique<KalmanFilter>();
    }

    /** @brief Constructs a Tracklet with a KalmanFilter at (x, y, w, h), label 1, score 0.9. */
    std::unique_ptr<Tracklet> make_tracklet(float x = 100.f, float y = 200.f, float w = 50.f,
                                            float h = 80.f)
    {
        return std::make_unique<Tracklet>(make_kf(), Rect<float>(x, y, w, h), 1, 0.9f);
    }

    /** @brief Calls predict() on @p track @p times times with @p dt. */
    void predict_times(Tracklet& track, int times, float dt)
    {
        for (int i = 0; i < times; ++i)
            track.predict(dt);
    }

    /** @brief The frame an activation happens on and the flags it must leave. */
    struct ActivationRow
    {
            std::size_t frame_id;
            bool activated;
            bool confirmed;
    };

    /** @brief The dt a track is marked Lost with. */
    struct LostDtRow
    {
            float dt;
    };

    /** @brief Whether the track is Lost before predict() and the lost time it must show. */
    struct PredictRow
    {
            bool lost;
            float lost_time;
    };

    /** @brief The keep_state flag of an update and the state and activation it must leave. */
    struct KeepStateRow
    {
            bool keep_state;
            TrackState state;
            bool activated;
    };

    /** @brief The number of unobserved predict() calls. */
    struct UnobservedRow
    {
            int predicts;
    };

    /** @brief A horizontal march of the observation and the direction and speed it must give. */
    struct MarchRow
    {
            float start_x;
            float step;
            float velocity_x;
            float speed;
    };

    /** @brief The unobserved predicts before a recovery and whether ORU changes the mean. */
    struct OruRow
    {
            int predicts;
            bool differs;
    };

    /**
     * @brief Activates @p track at (start_x, 200) on frame 1, then moves its observation by
     *        @p step per frame through frames 2 to 4, one predict() and one update() each.
     */
    void march(Tracklet& track, float start_x, float step)
    {
        track.activate(make_kf(), 1, 1);
        float x = start_x;
        for (std::size_t frame = 2; frame <= 4; ++frame)
        {
            track.predict(1.0f);
            x += step;
            auto observation = make_tracklet(x, 200.f);
            track.update(*observation, frame, true, false);
        }
    }

    /**
     * @brief Returns the mean a tracklet holds after a recovery, with ORU on or off.
     *
     * The track is activated on frame 1, predicted once and marked Lost, predicted
     * @p predicts - 1 more times, and re-activated by an observation 30 px to the right, so
     * the gap ORU bridges is @p predicts frames of 1.0 s each.
     */
    KalmanTypes::StateMean recovered_mean(bool enable_oru, int predicts)
    {
        Tracklet track(make_kf(), Rect<float>(100.f, 200.f, 50.f, 80.f), 1, 0.9f, {}, 0.9f,
                       false, 100, 3, enable_oru);
        track.activate(make_kf(), 1, 1);
        track.predict(1.0f);
        track.mark_as_lost(1.0f);
        predict_times(track, predicts - 1, 1.0f);
        auto observation = make_tracklet(130.f, 200.f);
        track.re_activate(*observation, 5);
        return track.get_mean();
    }
} // namespace

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

/**
 * @brief The activation rows, run over the frame an activation happens on.
 */
class Small_TrackletActivationTest : public ::testing::TestWithParam<ActivationRow>
{
};

/**
 * @brief activate() sets Tracked and records the id and frame; only a first-frame track is
 *        activated and confirmed at once, a later one waits for a match.
 */
TEST_P(Small_TrackletActivationTest, ActivateSetsTheFlagsByFrame)
{
    auto t = make_tracklet();
    t->activate(make_kf(), GetParam().frame_id, /*track_id=*/42);

    EXPECT_EQ(t->get_track_state(), TrackState::Tracked);
    EXPECT_EQ(t->is_activated(), GetParam().activated);
    EXPECT_EQ(t->is_confirmed(), GetParam().confirmed);
    EXPECT_EQ(t->get_track_id(), static_cast<size_t>(42));
    EXPECT_EQ(t->get_frame_id(), GetParam().frame_id);
}

INSTANTIATE_TEST_SUITE_P(Frames, Small_TrackletActivationTest,
                         ::testing::Values(ActivationRow{1, true, true},
                                           ActivationRow{5, false, false}));

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

/**
 * @brief activate_tentative() starts the lost-time counter at zero.
 */
TEST(Small_TrackletTest, ActivateTentativeSetsLostTimeZero)
{
    auto t = make_tracklet();

    t->activate_tentative(make_kf(), /*frame_id=*/1, /*initial_id=*/0);

    EXPECT_EQ(t->get_track_state(), TrackState::Tentative);
    EXPECT_FLOAT_EQ(t->get_accumulated_lost_time(), 0.0f);
}

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
 * @brief The Lost-time rows, run over the dt a track is marked Lost with.
 */
class Small_TrackletLostTimeTest : public ::testing::TestWithParam<LostDtRow>
{
};

/**
 * @brief mark_as_lost() starts the accumulated lost time at the dt it is given, zero included.
 */
TEST_P(Small_TrackletLostTimeTest, MarkAsLostStartsTheLostTimeAtDt)
{
    auto t = make_tracklet();
    t->activate(make_kf(), 1, 1);

    t->mark_as_lost(GetParam().dt);

    EXPECT_EQ(t->get_track_state(), TrackState::Lost);
    EXPECT_FLOAT_EQ(t->get_accumulated_lost_time(), GetParam().dt);
}

INSTANTIATE_TEST_SUITE_P(Dts, Small_TrackletLostTimeTest,
                         ::testing::Values(LostDtRow{0.04f}, LostDtRow{0.0f}));

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
 * @brief The predict rows, run over a Lost and a Tracked track.
 */
class Small_TrackletPredictLostTimeTest : public ::testing::TestWithParam<PredictRow>
{
};

/**
 * @brief predict() adds its dt to the lost time of a Lost track, 0 + 0.04 exactly, and leaves
 *        the zero lost time of a Tracked track alone.
 */
TEST_P(Small_TrackletPredictLostTimeTest, PredictAddsDtOnlyToALostTrack)
{
    auto t = make_tracklet();
    t->activate(make_kf(), 1, 1);
    if (GetParam().lost)
        t->mark_as_lost(0.0f);

    t->predict(0.04f);

    EXPECT_FLOAT_EQ(t->get_accumulated_lost_time(), GetParam().lost_time);
}

INSTANTIATE_TEST_SUITE_P(States, Small_TrackletPredictLostTimeTest,
                         ::testing::Values(PredictRow{true, 0.04f}, PredictRow{false, 0.0f}));

/**
 * @brief predict() adds its dt to the lost time of a Tentative track, 0 + 1.0 exactly, and
 *        leaves the track Tentative.
 */
TEST(Small_TrackletTest, PredictIncrementsLostTimeForTentative)
{
    auto t = make_tracklet();
    t->activate_tentative(make_kf(), 1, 0);
    ASSERT_EQ(t->get_track_state(), TrackState::Tentative);

    t->predict(1.0f);

    EXPECT_EQ(t->get_track_state(), TrackState::Tentative);
    EXPECT_FLOAT_EQ(t->get_accumulated_lost_time(), 1.0f);
}

/**
 * @brief The keep_state rows, run over a tentative track and one update.
 */
class Small_TrackletKeepStateTest : public ::testing::TestWithParam<KeepStateRow>
{
};

/**
 * @brief update() leaves a Tentative track Tentative with keep_state, and promotes it to a
 *        Tracked, activated track without it.
 */
TEST_P(Small_TrackletKeepStateTest, KeepStateDecidesWhetherUpdatePromotes)
{
    auto t = make_tracklet();
    t->activate_tentative(make_kf(), 3, 0);
    ASSERT_EQ(t->get_track_state(), TrackState::Tentative);

    auto obs = make_tracklet(102.f, 202.f);
    t->update(*obs, /*frame_id=*/4, /*update_features=*/false, GetParam().keep_state);

    EXPECT_EQ(t->get_track_state(), GetParam().state);
    EXPECT_EQ(t->is_activated(), GetParam().activated);
}

INSTANTIATE_TEST_SUITE_P(Flags, Small_TrackletKeepStateTest,
                         ::testing::Values(KeepStateRow{true, TrackState::Tentative, false},
                                           KeepStateRow{false, TrackState::Tracked, true}));

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

/**
 * @brief The feature history keeps the newest max_feature_history features: with room for 5 and
 *        8 unit features along axes 0 to 7, it holds axes 3 to 7, the oldest first.
 */
TEST(Small_TrackletTest, FullFeatureHistoryEvictsItsOldestFeature)
{
    const int capacity = 5;
    const int updates = 8;
    auto t = std::make_unique<Tracklet>(make_kf(), Rect<float>(0.f, 0.f, 50.f, 80.f), 1, 0.9f,
                                        std::vector<float>{}, 0.9f, false, capacity);
    for (int axis = 0; axis < updates; ++axis)
    {
        std::vector<float> feature(static_cast<std::size_t>(updates), 0.f);
        feature[static_cast<std::size_t>(axis)] = 1.f;
        t->update_features(feature);
    }

    const auto& history = t->get_feature_history();

    ASSERT_EQ(history.size(), static_cast<std::size_t>(capacity));
    for (int slot = 0; slot < capacity; ++slot)
    {
        std::vector<float> expected(static_cast<std::size_t>(updates), 0.f);
        expected[static_cast<std::size_t>(slot + updates - capacity)] = 1.f;
        EXPECT_EQ(history[static_cast<std::size_t>(slot)], expected) << "slot " << slot;
    }
}

/**
 * @brief A fresh activation records the first raw observation but cannot yet derive a
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
 * @brief The unobserved-predict rows.
 */
class Small_TrackletUnobservedTest : public ::testing::TestWithParam<UnobservedRow>
{
};

/**
 * @brief Each unobserved predict() advances the frames-since-observation counter by one.
 */
TEST_P(Small_TrackletUnobservedTest, PredictAdvancesFramesSinceObservation)
{
    auto t = make_tracklet();
    t->activate(make_kf(), 1, 1);

    predict_times(*t, GetParam().predicts, 1.0f);

    EXPECT_EQ(t->get_frames_since_observation(), GetParam().predicts);
}

INSTANTIATE_TEST_SUITE_P(Predicts, Small_TrackletUnobservedTest,
                         ::testing::Values(UnobservedRow{1}, UnobservedRow{2}));

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
 * @brief The march rows: a rightward and a leftward march of the raw observation.
 */
class Small_TrackletObservationVelocityTest : public ::testing::TestWithParam<MarchRow>
{
};

/**
 * @brief The observed direction is a unit vector along the horizontal step of the raw
 *        observations and the speed equals the per-frame step.
 *
 * The window holds all four observations, the first at start_x and the last 3 steps on, so
 * dx / dist is the sign of the step, dy is 0, and dist / 3 frames is |step|; every value is a
 * small integer, so each is exact in float.
 */
TEST_P(Small_TrackletObservationVelocityTest, DirectionAndSpeedFollowTheMarch)
{
    auto t = make_tracklet(GetParam().start_x, 200.f);
    march(*t, GetParam().start_x, GetParam().step);

    ASSERT_TRUE(t->has_obs_velocity());
    EXPECT_FLOAT_EQ(t->get_obs_velocity_x(), GetParam().velocity_x);
    EXPECT_FLOAT_EQ(t->get_obs_velocity_y(), 0.0f);
    EXPECT_FLOAT_EQ(t->get_obs_speed(), GetParam().speed);
}

INSTANTIATE_TEST_SUITE_P(Marches, Small_TrackletObservationVelocityTest,
                         ::testing::Values(MarchRow{100.f, 10.f, 1.0f, 10.0f},
                                           MarchRow{300.f, -8.f, -1.0f, 8.0f}));

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

/**
 * @brief Without an IMM predictor the kinematic state reads Cruising, before and after the
 *        track is lost.
 */
TEST(Small_TrackletTest, KinematicStateReadsCruisingWithoutAnImmPredictor)
{
    auto t = make_tracklet();
    t->activate(make_kf(), 1, 1);
    const KinematicState tracked = t->get_kinematic_state();

    t->mark_as_lost(0.04f);

    EXPECT_EQ(tracked, KinematicState::Cruising);
    EXPECT_EQ(t->get_kinematic_state(), KinematicState::Cruising);
}

/**
 * @brief Without an IMM predictor the manoeuvre probability reads zero.
 */
TEST(Small_TrackletTest, ManeuverProbabilityIsZeroWithoutAnImmPredictor)
{
    auto t = make_tracklet();
    t->activate(make_kf(), 1, 1);

    EXPECT_FLOAT_EQ(t->get_maneuver_probability(), 0.0f);
}

/**
 * @brief While a track coasts, get_last_seen_state() returns the regime it had before the loss.
 *
 * An update under the IMM predictor sets the regime. mark_as_lost() stores it as the pre-coast
 * regime and switches the state to Coasting; a later predict() of the Lost track refreshes
 * Coasting without overwriting the stored regime.
 */
TEST(Small_TrackletTest, LastSeenStateIsThePreLossRegimeWhileCoasting)
{
    Tracklet track(std::make_unique<IMMFilter>(), Rect<float>(100.f, 200.f, 50.f, 80.f), 1, 0.9f);
    track.activate(std::make_unique<IMMFilter>(), 1, 1);
    track.predict(1.0f);
    auto observation = make_tracklet(106.f, 200.f);
    track.update(*observation, 2, true, false);
    const KinematicState before_loss = track.get_kinematic_state();
    ASSERT_NE(before_loss, KinematicState::Coasting);

    track.mark_as_lost(1.0f);
    track.predict(1.0f);

    EXPECT_EQ(track.get_kinematic_state(), KinematicState::Coasting);
    EXPECT_EQ(track.get_last_seen_state(), before_loss);
}

/**
 * @brief The ORU rows, run over the unobserved predicts before a recovery.
 */
class Small_TrackletRecoveryReupdateTest : public ::testing::TestWithParam<OruRow>
{
};

/**
 * @brief ORU replays the virtual trajectory from the last checkpoint over a gap of two frames
 *        or more, so the recovered mean differs from the plain update; over a gap of one frame
 *        it does not run and the two means are identical.
 *
 * The comparison is metamorphic: it states that the means differ, not by how much.
 */
TEST_P(Small_TrackletRecoveryReupdateTest, OruChangesTheRecoveredMeanOnlyOverALongGap)
{
    const KalmanTypes::StateMean with_oru = recovered_mean(true, GetParam().predicts);
    const KalmanTypes::StateMean without_oru = recovered_mean(false, GetParam().predicts);

    EXPECT_EQ((with_oru - without_oru).norm() > 0.0f, GetParam().differs);
}

INSTANTIATE_TEST_SUITE_P(Gaps, Small_TrackletRecoveryReupdateTest,
                         ::testing::Values(OruRow{1, false}, OruRow{3, true}));
