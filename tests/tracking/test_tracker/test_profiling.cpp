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
 * @file test_profiling.cpp
 * @brief Tests the zones, counters and instants Tracker::update and the C API record under a
 *        profiler session.
 * @author Mustafa Garip
 */

#include <SushiTrack/profiling/profiling.hpp>

#if SUSHITRACK_ENABLE_PROFILER

#include <cstddef>
#include <optional>
#include <set>
#include <string>
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
    /// @brief Returns how many periods of @p record hold a zone named @p zone.
    std::size_t periods_holding(const SushiProfiler::RunRecord& record, const char* zone)
    {
        std::size_t holding = 0U;
        for (const SushiProfiler::PeriodRecord& period : record.periods)
        {
            if (zone_names(record, period).count(zone) == 1U)
                ++holding;
        }
        return holding;
    }

    /// @brief Returns how many tracks of @p first differ from those of @p second in id, state,
    ///        x or y; the two hold the same number.
    std::size_t differing_tracks(const std::vector<sushitrack_track_t>& first,
                                 const std::vector<sushitrack_track_t>& second)
    {
        std::size_t differing = 0U;
        for (std::size_t i = 0; i < first.size(); ++i)
        {
            if (first[i].track_id != second[i].track_id || first[i].state != second[i].state ||
                first[i].x != second[i].x || first[i].y != second[i].y)
            {
                ++differing;
            }
        }
        return differing;
    }

    /// @brief A zone and the number of the three periods that must hold it.
    struct ZoneRow
    {
            const char* zone;
            std::size_t periods;
    };
} // namespace

/**
 * @brief The level 1 zones of the plain path, run over each zone and the number of periods that
 *        hold it.
 */
class Medium_TrackerLevelOneZonesTest : public ::testing::TestWithParam<ZoneRow>
{
};

/**
 * @brief Each level 1 zone the plain path runs appears in all three periods, and the zones of
 *        the stages it does not run (lost recovery, tentative) appear in none.
 */
TEST_P(Medium_TrackerLevelOneZonesTest, ZoneAppearsInTheExpectedPeriods)
{
    const SushiProfiler::RunRecord record = record_scenario(1U);

    ASSERT_EQ(record.periods.size(), 3U);
    EXPECT_EQ(periods_holding(record, GetParam().zone), GetParam().periods);
}

INSTANTIATE_TEST_SUITE_P(
    Zones, Medium_TrackerLevelOneZonesTest,
    ::testing::Values(ZoneRow{Keys::K_ZONE_UPDATE, 3U}, ZoneRow{Keys::K_ZONE_SPLIT, 3U},
                      ZoneRow{Keys::K_ZONE_PREDICT, 3U}, ZoneRow{Keys::K_ZONE_MATCH_HIGH, 3U},
                      ZoneRow{Keys::K_ZONE_MATCH_LOW, 3U}, ZoneRow{Keys::K_ZONE_UNCONFIRMED, 3U},
                      ZoneRow{Keys::K_ZONE_CLEANUP_LOST, 3U}, ZoneRow{Keys::K_ZONE_FINALIZE, 3U},
                      ZoneRow{Keys::K_ZONE_OUTPUT, 3U}, ZoneRow{Keys::K_ZONE_LOST_RECOVERY, 0U},
                      ZoneRow{Keys::K_ZONE_TENTATIVE, 0U}));

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
    EXPECT_EQ(differing_tracks(with, without), 0U);
}

/**
 * @brief Fixture of the level 1 record of the three-frame scenario.
 */
class Medium_TrackerLevelOneCountersTest : public ::testing::Test
{
    protected:
        std::optional<SushiProfiler::RunRecord> record_;

        void SetUp() override
        {
            record_ = record_scenario(1U);
            ASSERT_EQ(record_->periods.size(), 3U);
        }

        /// @brief Returns the value counter @p name folded to in period @p frame.
        double value(std::size_t frame, const char* name) const
        {
            return counter_value(*record_, record_->periods[frame], name);
        }
};

/**
 * @brief The detections counter holds the two, one and zero detections of the three frames.
 */
TEST_F(Medium_TrackerLevelOneCountersTest, DetectionsCounterFollowsTheFrames)
{
    EXPECT_EQ(value(0, Keys::K_COUNTER_DETECTIONS_IN), 2.0);
    EXPECT_EQ(value(1, Keys::K_COUNTER_DETECTIONS_IN), 1.0);
    EXPECT_EQ(value(2, Keys::K_COUNTER_DETECTIONS_IN), 0.0);
}

/**
 * @brief The lifecycle counters record the seed in frame 1, the match in frame 2 and the loss
 *        in frame 3.
 */
TEST_F(Medium_TrackerLevelOneCountersTest, LifecycleCountersFollowTheScenario)
{
    EXPECT_EQ(value(0, Keys::K_COUNTER_TENTATIVE_CONFIRMED), 1.0);
    EXPECT_EQ(value(1, Keys::K_COUNTER_MATCHES_HIGH), 1.0);
    EXPECT_EQ(value(2, Keys::K_COUNTER_TRACKS_NEWLY_LOST), 1.0);
}

/**
 * @brief The alive counters hold one Tracked track in frames 1 and 2 and one Lost track in
 *        frame 3.
 */
TEST_F(Medium_TrackerLevelOneCountersTest, AliveCountersFollowTheScenario)
{
    EXPECT_EQ(value(0, Keys::K_COUNTER_ALIVE_TRACKED), 1.0);
    EXPECT_EQ(value(1, Keys::K_COUNTER_ALIVE_TRACKED), 1.0);
    EXPECT_EQ(value(2, Keys::K_COUNTER_ALIVE_TRACKED), 0.0);
    EXPECT_EQ(value(2, Keys::K_COUNTER_ALIVE_LOST), 1.0);
}

/**
 * @brief The output counters hold the one track written in frames 1 and 3 and no dropped track.
 */
TEST_F(Medium_TrackerLevelOneCountersTest, OutputCountersFollowTheScenario)
{
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
 * @brief The cost matrix zone and the level 2 counters are absent from a record made at runtime
 *        level 1.
 */
TEST(Medium_TrackerProfilingTest, LevelTwoRecordsAreAbsentAtLevelOne)
{
    const SushiProfiler::RunRecord shallow = record_scenario(1U);

    ASSERT_EQ(shallow.periods.size(), 3U);
    EXPECT_EQ(zone_names(shallow, shallow.periods[1]).count(Keys::K_ZONE_COST_MATRIX), 0U);
    EXPECT_EQ(counter_value(shallow, shallow.periods[1], Keys::K_COUNTER_COST_CELLS), -1.0);
}

#if SUSHITRACK_PROFILE_LEVEL >= 2
/**
 * @brief The cost matrix and solve zones, the cost cells counter and the lost instant appear at
 *        runtime level 2, in a build that compiled level 2 in.
 */
TEST(Medium_TrackerProfilingTest, LevelTwoRecordsAppearAtLevelTwo)
{
    const SushiProfiler::RunRecord fine = record_scenario(2U);

    ASSERT_EQ(fine.periods.size(), 3U);
    EXPECT_EQ(zone_names(fine, fine.periods[1]).count(Keys::K_ZONE_COST_MATRIX), 1U);
    EXPECT_EQ(zone_names(fine, fine.periods[1]).count(Keys::K_ZONE_SOLVE), 1U);
    EXPECT_GT(counter_value(fine, fine.periods[1], Keys::K_COUNTER_COST_CELLS), 0.0);
    EXPECT_EQ(instant_count(fine, fine.periods[2], Keys::K_INSTANT_TRACK_LOST), 1U);
}
#endif

#endif
