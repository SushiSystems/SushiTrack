/**************************************************************************/
/* test_tracker_builder.cpp                                               */
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
 * @file test_tracker_builder.cpp
 * @brief Tests build_tracker, which builds the adapter a registry holds under a name.
 * @author Mustafa Garip
 */

#include <memory>

#include <gtest/gtest.h>

#include <byte_track_adapter.hpp>
#include <captured_log.hpp>
#include <oc_sort_adapter.hpp>
#include <support/default_params.hpp>
#include <sushitrack_adapter.hpp>
#include <tracker_bridge_log_category.hpp>
#include <tracker_builder.hpp>
#include <tracker_registry.hpp>

using TestSupport::default_params;

namespace
{
    /**
     * @brief Returns a registry that holds the three reference trackers, none of which reads a
     *        file.
     */
    TrackerBridge::TrackerRegistry reference_registry()
    {
        TrackerBridge::TrackerRegistry registry;
        registry.register_tracker("sushitrack",
                                  []
                                  {
                                      return std::make_unique<TrackerBridge::SushiTrackAdapter>(
                                          default_params());
                                  });
        registry.register_tracker("bytetrack",
                                  []
                                  {
                                      return std::make_unique<TrackerBridge::ByteTrackAdapter>();
                                  });
        registry.register_tracker("ocsort",
                                  []
                                  {
                                      return std::make_unique<TrackerBridge::OCSORTAdapter>();
                                  });
        return registry;
    }
} // namespace

/**
 * @brief Each registered name, in the case the pipeline passes, builds a tracker.
 */
TEST(Small_TrackerBuilderTest, BuildsEveryRegisteredTracker)
{
    const TrackerBridge::TrackerRegistry registry = reference_registry();
    for (const char* name : {"SushiTrack", "ByteTrack", "OCSORT"})
        EXPECT_NE(TrackerBridge::build_tracker(registry, name), nullptr) << name;
    EXPECT_EQ(TrackerBridge::build_tracker(registry, "no-such-tracker"), nullptr);
    EXPECT_EQ(TrackerBridge::build_tracker(registry, nullptr), nullptr);
}

/**
 * @brief An unknown or null tracker name logs one ERROR that lists the registered names.
 */
TEST(Small_TrackerBuilderTest, RefusedNamesAreLogged)
{
    SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace,
                                {TrackerBridge::K_TRACKER_BRIDGE_LOG_CATEGORY});
    const TrackerBridge::TrackerRegistry registry = reference_registry();

    EXPECT_EQ(TrackerBridge::build_tracker(registry, "no-such-tracker"), nullptr);
    EXPECT_EQ(TrackerBridge::build_tracker(registry, nullptr), nullptr);

    EXPECT_EQ(log.count("no-such-tracker is not registered (known: "), 1);
    EXPECT_EQ(log.count("create_tracker refused: the name is null"), 1);
    EXPECT_EQ(log.level_of("is not registered"), SushiLog::LogLevel::Error);
}
