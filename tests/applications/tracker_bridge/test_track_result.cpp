/**************************************************************************/
/* test_track_result.cpp                                                  */
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
 * @file test_track_result.cpp
 * @brief Tests that TrackResult keeps its fields in identity, box, score order.
 * @author Mustafa Garip
 */

#include <gtest/gtest.h>

#include <track_result.hpp>

/**
 * @brief Brace initialisation fills identity, box and score in that order, as the bridge does.
 */
TEST(Small_TrackResultTest, BraceInitialisationFollowsTheFieldOrder)
{
    const TrackerBridge::TrackResult result{7, 1.f, 2.f, 3.f, 4.f, 0.5f};
    EXPECT_EQ(result.track_id, 7);
    EXPECT_FLOAT_EQ(result.x, 1.f);
    EXPECT_FLOAT_EQ(result.h, 4.f);
    EXPECT_FLOAT_EQ(result.score, 0.5f);
}
