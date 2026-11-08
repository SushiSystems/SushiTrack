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

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <captured_log.hpp>
#include <oc_sort_adapter.hpp>
#include <tracker_adapter.hpp>
#include <tracker_bridge_log_category.hpp>
#include <tracker_registry.hpp>

using namespace TrackerBridge;

/**
 * @brief A name registered in one case is found in any other.
 */
TEST(Small_TrackerRegistryTest, FindsANameInAnyCase)
{
    TrackerRegistry registry;
    registry.register_tracker("RegistryProbe",
                              []
                              {
                                  return std::make_unique<OCSORTAdapter>();
                              });

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
 * @brief A second registration under one name logs one WARN, and the names come back sorted.
 */
TEST(Small_TrackerRegistryTest, RegisteringANameTwiceWarnsAndNamesAreSorted)
{
    SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace, {K_TRACKER_BRIDGE_LOG_CATEGORY});
    TrackerRegistry registry;
    const auto make = []
    {
        return std::make_unique<OCSORTAdapter>();
    };

    registry.register_tracker("ZuluProbe", make);
    registry.register_tracker("DuplicateProbe", make);
    registry.register_tracker("duplicateprobe", make);

    EXPECT_EQ(log.count("Tracker duplicateprobe registered twice"), 1);
    EXPECT_EQ(registry.registered_names(),
              (std::vector<std::string>{"duplicateprobe", "zuluprobe"}));
}
