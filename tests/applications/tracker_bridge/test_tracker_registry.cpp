/**************************************************************************/
/* test_tracker_registry.cpp                                              */
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
 * @file test_tracker_registry.cpp
 * @brief Tests TrackerRegistry.
 * @author Mustafa Garip
 */

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <byte_track_adapter.hpp>
#include <captured_log.hpp>
#include <oc_sort_adapter.hpp>
#include <tracker_adapter.hpp>
#include <tracker_bridge_log_category.hpp>
#include <tracker_registry.hpp>

using namespace TrackerBridge;

namespace
{
    /// @brief Builds an OC-SORT adapter, which reads no file.
    std::unique_ptr<TrackerAdapter> make_ocsort()
    {
        return std::make_unique<OCSORTAdapter>();
    }
} // namespace

/**
 * @brief A name registered in one case is found in any other.
 */
TEST(Small_TrackerRegistryTest, FindsANameInAnyCase)
{
    TrackerRegistry registry;
    registry.register_tracker("RegistryProbe", make_ocsort);

    const std::unique_ptr<TrackerAdapter> adapter = registry.create("REGISTRYPROBE");
    ASSERT_NE(adapter, nullptr);
    EXPECT_EQ(adapter->name(), "ocsort");
}

/**
 * @brief A name nobody registered builds nothing.
 */
TEST(Small_TrackerRegistryTest, UnknownNameBuildsNothing)
{
    const TrackerRegistry registry;
    EXPECT_EQ(registry.create("no-such-tracker"), nullptr);
}

/**
 * @brief A second registration under one name, in any case, logs one WARN naming the folded name.
 */
TEST(Small_TrackerRegistryTest, RegisteringANameTwiceWarnsOnce)
{
    SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace, {K_TRACKER_BRIDGE_LOG_CATEGORY});
    TrackerRegistry registry;

    registry.register_tracker("DuplicateProbe", make_ocsort);
    registry.register_tracker("duplicateprobe", make_ocsort);

    EXPECT_EQ(log.count("Tracker duplicateprobe registered twice"), 1);
    EXPECT_EQ(log.level_of("registered twice"), SushiLog::LogLevel::Warn);
}

/**
 * @brief The later creator replaces the first, as the WARN says.
 */
TEST(Small_TrackerRegistryTest, TheLaterRegistrationReplacesTheFirst)
{
    TrackerRegistry registry;
    registry.register_tracker("probe", make_ocsort);
    registry.register_tracker("probe",
                              []() -> std::unique_ptr<TrackerAdapter>
                              {
                                  return std::make_unique<ByteTrackAdapter>();
                              });

    const std::unique_ptr<TrackerAdapter> adapter = registry.create("probe");

    ASSERT_NE(adapter, nullptr);
    EXPECT_EQ(adapter->name(), "bytetrack");
}

/**
 * @brief The registered names come back folded to lower case and sorted.
 */
TEST(Small_TrackerRegistryTest, NamesComeBackFoldedAndSorted)
{
    TrackerRegistry registry;

    registry.register_tracker("ZuluProbe", make_ocsort);
    registry.register_tracker("AlphaProbe", make_ocsort);

    EXPECT_EQ(registry.registered_names(), (std::vector<std::string>{"alphaprobe", "zuluprobe"}));
}
