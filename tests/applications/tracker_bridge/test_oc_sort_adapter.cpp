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
 * @brief Tests OCSORTAdapter: the defaults and the settings that replace them.
 * @author Mustafa Garip
 */

#include <vector>

#include <gtest/gtest.h>

#include <oc_sort_adapter.hpp>

using namespace TrackerBridge;

namespace
{
    /// @brief A score under the default det_thresh (0.4) and over 0.3.
    constexpr float K_SCORE_BELOW_DET_THRESH = 0.35f;

    /// @brief One detection with K_SCORE_BELOW_DET_THRESH.
    std::vector<MOTDetection> one_detection_below_det_thresh()
    {
        MOTDetection det;
        det.x = 100.f;
        det.y = 100.f;
        det.w = 50.f;
        det.h = 80.f;
        det.score = K_SCORE_BELOW_DET_THRESH;
        return {det};
    }
} // namespace

/**
 * @brief With no settings the default det_thresh of 0.4 refuses a box that scores 0.35, so no
 *        track starts.
 */
TEST(Small_OCSORTAdapterTest, MissingSettingsKeepTheDefaults)
{
    OCSORTAdapter adapter(nlohmann::json::object(), "settings.json");

    EXPECT_TRUE(adapter.update(one_detection_below_det_thresh(), 1.f).empty());
}

/**
 * @brief A det_thresh of 0.3 in the "ocsort" section replaces the default: the box that scores
 *        0.35 then starts a track.
 */
TEST(Small_OCSORTAdapterTest, SettingsReplaceTheDefaults)
{
    const nlohmann::json document = {{"ocsort", {{"det_thresh", 0.3}}}};
    OCSORTAdapter adapter(document, "settings.json");

    EXPECT_EQ(adapter.update(one_detection_below_det_thresh(), 1.f).size(), 1U);
}
