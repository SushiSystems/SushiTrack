/**************************************************************************/
/* test_throwing_tracker_adapter.cpp                                      */
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
 * @file test_throwing_tracker_adapter.cpp
 * @brief Tests ThrowingTrackerAdapter, the adapter whose update always throws.
 * @author Mustafa Garip
 */

#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

#include <throwing_tracker_adapter.hpp>

/**
 * @brief Update throws std::runtime_error carrying the adapter's failure text, for any frame.
 */
TEST(Small_ThrowingTrackerAdapterTest, UpdateThrowsRuntimeErrorWithTheFailureText)
{
    TrackerBridge::ThrowingTrackerAdapter adapter;

    try
    {
        (void)adapter.update({});
        FAIL() << "update returned instead of throwing";
    }
    catch (const std::runtime_error& error)
    {
        EXPECT_EQ(std::string(error.what()),
                  std::string(TrackerBridge::ThrowingTrackerAdapter::K_FAILURE_TEXT));
    }
}
