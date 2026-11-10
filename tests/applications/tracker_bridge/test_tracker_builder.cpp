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
#include <stdexcept>

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
 * @brief Builds a tracker for each name the pipeline passes, in the case it passes it.
 */
class Small_TrackerBuilderTest : public ::testing::TestWithParam<const char*>
{
};

/**
 * @brief A registered name, in the case the pipeline passes, builds a tracker.
 */
TEST_P(Small_TrackerBuilderTest, BuildsARegisteredName)
{
    const TrackerBridge::TrackerRegistry registry = reference_registry();

    EXPECT_NE(TrackerBridge::build_tracker(registry, GetParam()), nullptr);
}

INSTANTIATE_TEST_SUITE_P(EveryReferenceName, Small_TrackerBuilderTest,
                         ::testing::Values("SushiTrack", "ByteTrack", "OCSORT"));

/**
 * @brief A name nobody registered builds nothing.
 */
TEST(Small_TrackerBuilderRefusalTest, UnknownNameBuildsNothing)
{
    const TrackerBridge::TrackerRegistry registry = reference_registry();

    EXPECT_EQ(TrackerBridge::build_tracker(registry, "no-such-tracker"), nullptr);
}

/**
 * @brief A null name builds nothing.
 */
TEST(Small_TrackerBuilderRefusalTest, NullNameBuildsNothing)
{
    const TrackerBridge::TrackerRegistry registry = reference_registry();

    EXPECT_EQ(TrackerBridge::build_tracker(registry, nullptr), nullptr);
}

/**
 * @brief An unknown name logs one ERROR that names it and lists the registered names, sorted.
 */
TEST(Small_TrackerBuilderRefusalTest, UnknownNameLogsOneErrorListingTheRegisteredNames)
{
    SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace,
                                {TrackerBridge::K_TRACKER_BRIDGE_LOG_CATEGORY});
    const TrackerBridge::TrackerRegistry registry = reference_registry();

    EXPECT_EQ(TrackerBridge::build_tracker(registry, "no-such-tracker"), nullptr);

    EXPECT_EQ(log.count("no-such-tracker is not registered (known: bytetrack, ocsort, sushitrack)"),
              1);
    EXPECT_EQ(log.level_of("is not registered"), SushiLog::LogLevel::Error);
}

/**
 * @brief A null name logs one ERROR.
 */
TEST(Small_TrackerBuilderRefusalTest, NullNameLogsOneError)
{
    SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace,
                                {TrackerBridge::K_TRACKER_BRIDGE_LOG_CATEGORY});
    const TrackerBridge::TrackerRegistry registry = reference_registry();

    EXPECT_EQ(TrackerBridge::build_tracker(registry, nullptr), nullptr);

    EXPECT_EQ(log.count("create_tracker refused: the name is null"), 1);
    EXPECT_EQ(log.level_of("the name is null"), SushiLog::LogLevel::Error);
}

/**
 * @brief A creator that throws builds nothing and logs one ERROR that carries its message.
 */
TEST(Small_TrackerBuilderRefusalTest, ThrowingCreatorBuildsNothingAndLogsOneError)
{
    SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace,
                                {TrackerBridge::K_TRACKER_BRIDGE_LOG_CATEGORY});
    TrackerBridge::TrackerRegistry registry;
    registry.register_tracker("broken",
                              []() -> std::unique_ptr<TrackerBridge::TrackerAdapter>
                              {
                                  throw std::runtime_error("settings are unreadable");
                              });

    EXPECT_EQ(TrackerBridge::build_tracker(registry, "broken"), nullptr);

    EXPECT_EQ(log.count("create_tracker failed for broken: settings are unreadable"), 1);
    EXPECT_EQ(log.level_of("create_tracker failed"), SushiLog::LogLevel::Error);
}
