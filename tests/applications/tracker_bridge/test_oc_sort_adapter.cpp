/**************************************************************************/
/* test_oc_sort_adapter.cpp                                               */
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
 * @file test_oc_sort_adapter.cpp
 * @brief Tests OCSORTAdapter.
 * @author Mustafa Garip
 */

#include <gtest/gtest.h>

#include <oc_sort_adapter.hpp>

using namespace TrackerBridge;

/**
 * @brief The adapter is named after the predictions folder `st eval` reads.
 */
TEST(Small_OCSORTAdapterTest, IsNamedOcsort)
{
    EXPECT_EQ(OCSORTAdapter().name(), "ocsort");
}

/**
 * @brief A frame with no detections gives no track.
 */
TEST(Small_OCSORTAdapterTest, EmptyFrameGivesNoTrack)
{
    OCSORTAdapter adapter;
    EXPECT_TRUE(adapter.update({}, 1.f).empty());
}
