/**************************************************************************/
/* test_profiling.cpp                                                     */
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
 * @file test_profiling.cpp
 * @brief Tests the zones, counters and instants Tracker::update and the C API record under a
 *        profiler session.
 * @author Mustafa Garip
 */

#include <SushiTrack/profiling/profiling.hpp>

#if SUSHITRACK_ENABLE_PROFILER

#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/c_api/sushitrack_c.h>

#include <c_detection_builders.hpp>
#include <profile_session.hpp>

using namespace SushiTrack;
namespace Keys = SushiTrack::ProfilerKeys;

namespace
{
    /// @brief One track's tracker settings: no tentative stage, no ReID, a low frame rate.
    sushitrack_params_t plain_params()
    {
        sushitrack_params_t params{};
        sushitrack_get_default_params(&params);
        params.enable_tentative = 0;
        params.enable_reid = 0;
        params.time_aware = 0;
        params.track_thresh = 0.2f;
        params.high_thresh = 0.5f;
        params.match_thresh = 0.9f;
        return params;
    }

    /// @brief A 50 x 80 detection at (x, y) with score @p score.
    sushitrack_object_t detection(float x, float y, float score)
    {
        sushitrack_object_t object{};
        object.x = x;
        object.y = y;
        object.width = 50.0f;
        object.height = 80.0f;
        object.score = score;
        object.label = 1;
        return object;
    }

    /**
     * @brief Runs the three-frame scenario under @p session when it is not null and returns what
     *        the last frame wrote.
     *
     * Frame 1 holds a track-worthy detection and a low one, frame 2 moves the first by two
     * pixels, and frame 3 holds none, so the track is lost.
     */
    std::vector<sushitrack_track_t> run_scenario(sushitrack_tracker_t tracker,
                                                 ProfiledFrames* session, int max_tracks = 8)
    {
        std::vector<sushitrack_track_t> out(static_cast<std::size_t>(max_tracks));
        std::vector<sushitrack_track_t> last;
        const auto step = [&](const std::vector<sushitrack_object_t>& objects)
        {
            int count = 0;
            const auto work = [&]
            {
                static_cast<void>(sushitrack_update(tracker, objects.data(),
                                                    static_cast<int>(objects.size()), out.data(),
                                                    max_tracks, 0.04f, &count));
            };
            if (session != nullptr)
                session->frame(work);
            else
                work();
            last.assign(out.begin(), out.begin() + count);
        };
        step({detection(100.f, 100.f, 0.9f), detection(400.f, 100.f, 0.1f)});
        step({detection(102.f, 100.f, 0.9f)});
        step({});
        return last;
    }

    /// @brief Returns the record of the scenario run under a session at @p level.
    SushiProfiler::RunRecord record_scenario(std::uint8_t level)
    {
        sushitrack_params_t params = plain_params();
        sushitrack_tracker_t tracker = sushitrack_create(&params);
        ProfiledFrames session(3U, level);
        EXPECT_TRUE(session.ok()) << session.error();
        static_cast<void>(run_scenario(tracker, &session));
        sushitrack_destroy(tracker);
        return session.finish();
    }
} // namespace

/**
 * @brief Every level 1 zone the plain path runs appears in each of the three periods.
 */
TEST(Medium_TrackerProfilingTest, EveryLevelOneZoneAppearsInEachFrame)
{
    const SushiProfiler::RunRecord record = record_scenario(1U);

    ASSERT_EQ(record.periods.size(), 3U);
    for (const SushiProfiler::PeriodRecord& period : record.periods)
    {
        const std::set<std::string> names = zone_names(record, period);
        for (const char* expected :
             {Keys::K_ZONE_UPDATE, Keys::K_ZONE_SPLIT, Keys::K_ZONE_PREDICT,
              Keys::K_ZONE_MATCH_HIGH, Keys::K_ZONE_MATCH_LOW, Keys::K_ZONE_UNCONFIRMED,
              Keys::K_ZONE_CLEANUP_LOST, Keys::K_ZONE_FINALIZE, Keys::K_ZONE_OUTPUT})
        {
            EXPECT_EQ(names.count(expected), 1U) << expected << " in period " << period.index;
        }
        EXPECT_EQ(names.count(Keys::K_ZONE_LOST_RECOVERY), 0U);
        EXPECT_EQ(names.count(Keys::K_ZONE_TENTATIVE), 0U);
    }
}

/**
 * @brief The tentative zone replaces the unconfirmed zone when the tentative stage is on.
 */
TEST(Medium_TrackerProfilingTest, TentativeStageNamesItsOwnZone)
{
    sushitrack_params_t params = plain_params();
    params.enable_tentative = 1;
    sushitrack_tracker_t tracker = sushitrack_create(&params);
    ProfiledFrames session(3U, 1U);
    ASSERT_TRUE(session.ok()) << session.error();
    static_cast<void>(run_scenario(tracker, &session));
    sushitrack_destroy(tracker);
    const SushiProfiler::RunRecord record = session.finish();

    ASSERT_EQ(record.periods.size(), 3U);
    const std::set<std::string> names = zone_names(record, record.periods[0]);
    EXPECT_EQ(names.count(Keys::K_ZONE_TENTATIVE), 1U);
    EXPECT_EQ(names.count(Keys::K_ZONE_UNCONFIRMED), 0U);
}

/**
 * @brief The tracks a scenario outputs are the same with the gate closed and with a session
 *        recording.
 */
TEST(Medium_TrackerProfilingTest, OutputIsEqualWithTheGateOpenAndClosed)
{
    sushitrack_params_t params = plain_params();
    sushitrack_tracker_t closed = sushitrack_create(&params);
    const std::vector<sushitrack_track_t> without = run_scenario(closed, nullptr);
    sushitrack_destroy(closed);

    sushitrack_tracker_t open = sushitrack_create(&params);
    ProfiledFrames session(3U, 2U);
    ASSERT_TRUE(session.ok()) << session.error();
    const std::vector<sushitrack_track_t> with = run_scenario(open, &session);
    sushitrack_destroy(open);
    static_cast<void>(session.finish());

    ASSERT_EQ(with.size(), without.size());
    for (std::size_t i = 0; i < with.size(); ++i)
    {
        EXPECT_EQ(with[i].track_id, without[i].track_id);
        EXPECT_EQ(with[i].state, without[i].state);
        EXPECT_FLOAT_EQ(with[i].x, without[i].x);
        EXPECT_FLOAT_EQ(with[i].y, without[i].y);
    }
}

/**
 * @brief The level 1 counters equal the counts the scenario's tracks give, frame by frame.
 */
TEST(Medium_TrackerProfilingTest, LevelOneCountersMatchTheScenario)
{
    const SushiProfiler::RunRecord record = record_scenario(1U);
    ASSERT_EQ(record.periods.size(), 3U);
    const auto value = [&](std::size_t frame, const char* name)
    {
        return counter_value(record, record.periods[frame], name);
    };

    EXPECT_EQ(value(0, Keys::K_COUNTER_DETECTIONS_IN), 2.0);
    EXPECT_EQ(value(1, Keys::K_COUNTER_DETECTIONS_IN), 1.0);
    EXPECT_EQ(value(2, Keys::K_COUNTER_DETECTIONS_IN), 0.0);

    EXPECT_EQ(value(0, Keys::K_COUNTER_TENTATIVE_CONFIRMED), 1.0);
    EXPECT_EQ(value(1, Keys::K_COUNTER_MATCHES_HIGH), 1.0);
    EXPECT_EQ(value(2, Keys::K_COUNTER_TRACKS_NEWLY_LOST), 1.0);

    EXPECT_EQ(value(0, Keys::K_COUNTER_ALIVE_TRACKED), 1.0);
    EXPECT_EQ(value(1, Keys::K_COUNTER_ALIVE_TRACKED), 1.0);
    EXPECT_EQ(value(2, Keys::K_COUNTER_ALIVE_TRACKED), 0.0);
    EXPECT_EQ(value(2, Keys::K_COUNTER_ALIVE_LOST), 1.0);

    EXPECT_EQ(value(0, Keys::K_COUNTER_TRACKS_WRITTEN), 1.0);
    EXPECT_EQ(value(2, Keys::K_COUNTER_TRACKS_WRITTEN), 1.0);
    EXPECT_EQ(value(0, Keys::K_COUNTER_TRACKS_DROPPED), 0.0);
}

/**
 * @brief A buffer smaller than the tracks the frame holds records the tracks it wrote and the
 *        ones it dropped.
 */
TEST(Medium_TrackerProfilingTest, OverflowIsCountedAsDroppedTracks)
{
    sushitrack_params_t params = plain_params();
    sushitrack_tracker_t tracker = sushitrack_create(&params);
    ProfiledFrames session(1U, 1U);
    ASSERT_TRUE(session.ok()) << session.error();
    std::vector<sushitrack_object_t> objects = {detection(100.f, 100.f, 0.9f),
                                                detection(300.f, 100.f, 0.9f)};
    std::vector<sushitrack_track_t> out(1);
    int count = 0;
    session.frame(
        [&]
        {
            static_cast<void>(sushitrack_update(tracker, objects.data(), 2, out.data(), 1, 0.04f,
                                                &count));
        });
    sushitrack_destroy(tracker);
    const SushiProfiler::RunRecord record = session.finish();

    ASSERT_EQ(record.periods.size(), 1U);
    EXPECT_EQ(counter_value(record, record.periods[0], Keys::K_COUNTER_TRACKS_WRITTEN), 1.0);
    EXPECT_EQ(counter_value(record, record.periods[0], Keys::K_COUNTER_TRACKS_DROPPED), 1.0);
}

/**
 * @brief The cost matrix zone and the level 2 counters and instants are absent from a record
 *        made at runtime level 1 and present at level 2 when the build compiled level 2 in.
 */
TEST(Medium_TrackerProfilingTest, LevelTwoRecordsAppearOnlyAtLevelTwo)
{
    const SushiProfiler::RunRecord shallow = record_scenario(1U);
    ASSERT_EQ(shallow.periods.size(), 3U);
    EXPECT_EQ(zone_names(shallow, shallow.periods[1]).count(Keys::K_ZONE_COST_MATRIX), 0U);
    EXPECT_EQ(counter_value(shallow, shallow.periods[1], Keys::K_COUNTER_COST_CELLS), -1.0);

    if (SUSHITRACK_PROFILE_LEVEL < 2)
        GTEST_SKIP() << "the build compiled level 2 out";

    const SushiProfiler::RunRecord fine = record_scenario(2U);
    ASSERT_EQ(fine.periods.size(), 3U);
    EXPECT_EQ(zone_names(fine, fine.periods[1]).count(Keys::K_ZONE_COST_MATRIX), 1U);
    EXPECT_EQ(zone_names(fine, fine.periods[1]).count(Keys::K_ZONE_SOLVE), 1U);
    EXPECT_GT(counter_value(fine, fine.periods[1], Keys::K_COUNTER_COST_CELLS), 0.0);
    EXPECT_EQ(instant_count(fine, fine.periods[2], Keys::K_INSTANT_TRACK_LOST), 1U);
}

#endif
