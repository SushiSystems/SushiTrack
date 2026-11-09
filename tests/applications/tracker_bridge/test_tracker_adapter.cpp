/**************************************************************************/
/* test_tracker_adapter.cpp                                               */
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
 * @file test_tracker_adapter.cpp
 * @brief Tests the TrackerAdapter contract through every adapter.
 * @author Mustafa Garip
 */

#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <byte_track_adapter.hpp>
#include <mot_detection.hpp>
#include <oc_sort_adapter.hpp>
#include <support/default_params.hpp>
#include <sushitrack_adapter.hpp>
#include <tracker_adapter.hpp>

namespace
{
    /** @brief Builds one adapter. */
    using AdapterFactory = std::function<std::unique_ptr<TrackerBridge::TrackerAdapter>()>;

    /** @brief Frames the steady object moves for. */
    constexpr int K_STEADY_FRAMES = 20;

    /** @brief Runs each adapter: SushiTrack, ByteTrack and OC-SORT with their defaults. */
    class Small_TrackerAdapterTest : public ::testing::TestWithParam<AdapterFactory>
    {
    };

    /**
     * @brief Feeds @p adapter one 50 x 80 box that moves 2 px a frame for K_STEADY_FRAMES frames
     *        and returns every track identity it reported.
     */
    std::set<int> identities_of_steady_object(TrackerBridge::TrackerAdapter& adapter)
    {
        adapter.set_frame_rate(30);
        std::set<int> identities;
        for (int frame = 0; frame < K_STEADY_FRAMES; ++frame)
        {
            TrackerBridge::MOTDetection det;
            det.x = 100.f + 2.f * frame;
            det.y = 100.f;
            det.w = 50.f;
            det.h = 80.f;
            det.score = 0.9f;
            for (const TrackerBridge::TrackResult& track : adapter.update({det}, 1.f / 30.f))
                identities.insert(track.track_id);
        }
        return identities;
    }
} // namespace

/**
 * @brief Through TrackerAdapter&, one object moving 2 px a frame is reported under one identity.
 */
TEST_P(Small_TrackerAdapterTest, SteadyObjectKeepsOneIdentity)
{
    const std::unique_ptr<TrackerBridge::TrackerAdapter> adapter = GetParam()();

    EXPECT_EQ(identities_of_steady_object(*adapter).size(), 1U);
}

/**
 * @brief A frame with no detections reports no track.
 */
TEST_P(Small_TrackerAdapterTest, EmptyFrameGivesNoTrack)
{
    const std::unique_ptr<TrackerBridge::TrackerAdapter> adapter = GetParam()();

    EXPECT_TRUE(adapter->update({}, 1.f).empty());
}

INSTANTIATE_TEST_SUITE_P(
    EveryAdapter, Small_TrackerAdapterTest,
    ::testing::Values(AdapterFactory(
                          []
                          {
                              return std::make_unique<TrackerBridge::SushiTrackAdapter>(
                                  TestSupport::default_params());
                          }),
                      AdapterFactory(
                          []
                          {
                              return std::make_unique<TrackerBridge::ByteTrackAdapter>();
                          }),
                      AdapterFactory(
                          []
                          {
                              return std::make_unique<TrackerBridge::OCSORTAdapter>();
                          })));
