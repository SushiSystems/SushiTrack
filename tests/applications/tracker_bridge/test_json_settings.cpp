/**************************************************************************/
/* test_json_settings.cpp                                                 */
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
 * @file test_json_settings.cpp
 * @brief Tests parse_json_settings, read_json_settings, json_section and json_number.
 * @author Mustafa Garip
 */

#include <filesystem>
#include <fstream>
#include <sstream>

#include <gtest/gtest.h>

#include <captured_log.hpp>
#include <json_settings.hpp>
#include <tracker_bridge_log_category.hpp>

using namespace TrackerBridge;

/**
 * @brief A file with comments is read, and a number it holds is returned.
 */
TEST(Medium_JsonSettingsTest, ReadsANumberFromAFileWithComments)
{
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "sushitrack_json_settings_test.json";
    {
        std::ofstream file(path);
        file << "{ // the reference tracker\n  \"bytetrack\": { \"track_buffer\": 45 }\n}\n";
    }

    const nlohmann::json doc = read_json_settings(path.string());
    std::filesystem::remove(path);

    EXPECT_FLOAT_EQ(json_number(doc["bytetrack"], "track_buffer", 30.f), 45.f);
}

/**
 * @brief A missing file reads as an empty object, after one WARN naming it.
 */
TEST(Medium_JsonSettingsTest, MissingFileReadsAsAnEmptyObject)
{
    SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace, {K_TRACKER_BRIDGE_LOG_CATEGORY});

    const nlohmann::json doc = read_json_settings("this/path/does/not/exist.json");

    EXPECT_TRUE(doc.is_object());
    EXPECT_TRUE(doc.empty());
    EXPECT_EQ(log.count("this/path/does/not/exist.json cannot be opened"), 1);
}

/**
 * @brief A key that is absent or not a number gives the fallback.
 */
TEST(Small_JsonSettingsTest, AbsentOrNonNumericKeyGivesTheFallback)
{
    const nlohmann::json section = {{"name", "x"}};
    EXPECT_FLOAT_EQ(json_number(section, "name", 1.5f), 1.5f);
    EXPECT_FLOAT_EQ(json_number(section, "missing", 2.5f), 2.5f);
}

/**
 * @brief A missing section and a key that is not a number each log one WARN, on the one stream
 *        every adapter uses.
 */
TEST(Small_JsonSettingsTest, UnusableSettingsAreLogged)
{
    SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace, {K_TRACKER_BRIDGE_LOG_CATEGORY});
    std::istringstream text("{ // no section\n}\n");

    const nlohmann::json document = parse_json_settings(text, "settings.json");
    const nlohmann::json& section = json_section(document, "bytetrack", "settings.json");
    const nlohmann::json typed = {{"track_buffer", "many"}};
    const float value = json_number(typed, "track_buffer", 30.f);

    EXPECT_TRUE(section.empty());
    EXPECT_FLOAT_EQ(value, 30.f);
    EXPECT_EQ(log.count("Settings file settings.json has no \"bytetrack\" section"), 1);
    EXPECT_EQ(log.count("Setting track_buffer is not a number; keeping 30"), 1);
}
