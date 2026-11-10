/**************************************************************************/
/* test_sushitrack_adapter.cpp                                            */
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
 * @file test_sushitrack_adapter.cpp
 * @brief Tests SushiTrackAdapter: the box it reports and the fallback of an unloadable file.
 * @author Mustafa Garip
 */

#include <vector>

#include <gtest/gtest.h>

#include <captured_log.hpp>
#include <mot_detection.hpp>
#include <support/default_params.hpp>
#include <support/tolerance.hpp>
#include <sushitrack_adapter.hpp>
#include <tracker_bridge_log_category.hpp>

using namespace TrackerBridge;
using TestSupport::default_params;

namespace
{
    /// @brief Frames the object stands still before its track is read.
    constexpr int K_STILL_FRAMES = 10;

    /// @brief Rounded operations a frame puts on the reported box: predict, gain, update and the
    ///        conversion back to a corner box. An upper count, so the bound errs on the safe side.
    constexpr std::size_t K_OPERATIONS_PER_FRAME = 4;

    /// @brief The box every still frame holds.
    MOTDetection still_detection()
    {
        MOTDetection det;
        det.x = 200.f;
        det.y = 100.f;
        det.w = 40.f;
        det.h = 90.f;
        det.score = 0.95f;
        return det;
    }

    /// @brief Feeds @p adapter K_STILL_FRAMES frames of still_detection() and returns the tracks of
    ///        the last frame.
    std::vector<TrackResult> tracks_after_still_frames(SushiTrackAdapter& adapter)
    {
        std::vector<TrackResult> tracks;
        for (int frame = 0; frame < K_STILL_FRAMES; ++frame)
            tracks = adapter.update({still_detection()}, 1.f / 30.f);
        return tracks;
    }

    /// @brief Asserts that @p computed equals @p reference within the rounding of the still frames.
    void expect_within_still_rounding(float computed, float reference)
    {
        EXPECT_TRUE(TestSupport::is_within(TestSupport::measure_relative_error<float>(
            computed, reference, K_STILL_FRAMES * K_OPERATIONS_PER_FRAME)));
    }
} // namespace

/**
 * @brief An object that stands still is reported on the box it was detected at, to the rounding
 *        of the frames it was filtered over.
 */
TEST(Small_SushiTrackAdapterTest, ReportedTrackFollowsItsDetection)
{
    SushiTrackAdapter adapter(default_params());

    const std::vector<TrackResult> tracks = tracks_after_still_frames(adapter);

    ASSERT_EQ(tracks.size(), 1U);
    expect_within_still_rounding(tracks[0].x, 200.f);
    expect_within_still_rounding(tracks[0].y, 100.f);
    expect_within_still_rounding(tracks[0].w, 40.f);
    expect_within_still_rounding(tracks[0].h, 90.f);
}

/**
 * @brief A parameter file that names a missing path logs one WARN naming it, and the adapter
 *        still builds on the default parameters: it confirms the still object like the adapter
 *        built from default_params().
 */
TEST(Medium_SushiTrackAdapterTest, UnloadableFileFallsBackToTheDefaultsWithOneWarn)
{
    const SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace, {K_TRACKER_BRIDGE_LOG_CATEGORY});

    SushiTrackAdapter adapter("this/path/does/not/exist.json");

    EXPECT_EQ(log.count("this/path/does/not/exist.json did not load"), 1);
    EXPECT_EQ(log.level_of("did not load"), SushiLog::LogLevel::Warn);
    EXPECT_EQ(tracks_after_still_frames(adapter).size(), 1U);
}
