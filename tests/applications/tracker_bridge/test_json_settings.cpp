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
 * @brief Tests the settings helpers of the reference trackers on streams and on the file door.
 * @author Mustafa Garip
 */

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

#include <captured_log.hpp>
#include <json_settings.hpp>
#include <tracker_bridge_log_category.hpp>

using namespace TrackerBridge;

namespace
{
    /** @brief Parses @p text as a settings file named `settings.json`. */
    nlohmann::json parse(const std::string& text)
    {
        std::istringstream stream(text);
        return parse_json_settings(stream, "settings.json");
    }

    /** @brief Writes @p text to a file named after the running test and returns its path. */
    std::filesystem::path write_temp(const std::string& text)
    {
        const std::filesystem::path path =
            std::filesystem::temp_directory_path() /
            (std::string("sushitrack_json_settings_") +
             ::testing::UnitTest::GetInstance()->current_test_info()->name() + ".json");
        std::ofstream(path) << text;
        return path;
    }
} // namespace

/**
 * @brief A document with comments parses to its content, and a number it holds is returned.
 */
TEST(Small_JsonSettingsTest, ADocumentWithCommentsParsesToItsContent)
{
    const nlohmann::json doc =
        parse("{ // the reference tracker\n  \"bytetrack\": { \"track_buffer\": 45 }\n}\n");

    EXPECT_EQ(json_number(doc["bytetrack"], "track_buffer", 30.f), 45.f);
}

/**
 * @brief A document that is not valid JSON reads as an empty object and logs one ERROR naming it.
 */
TEST(Small_JsonSettingsTest, InvalidJsonReadsAsAnEmptyObjectAndLogsError)
{
    const SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace, {K_TRACKER_BRIDGE_LOG_CATEGORY});

    const nlohmann::json doc = parse("{ \"bytetrack\": ");

    EXPECT_TRUE(doc.is_object());
    EXPECT_TRUE(doc.empty());
    EXPECT_EQ(log.count("Settings file settings.json is not valid JSON"), 1);
    EXPECT_EQ(log.level_of("is not valid JSON"), SushiLog::LogLevel::Error);
}

/**
 * @brief read_json_settings opens the file it is given and returns its document.
 */
TEST(Medium_JsonSettingsTest, ReadsTheDocumentOfAFile)
{
    const std::filesystem::path path = write_temp("{ \"bytetrack\": { \"track_buffer\": 45 } }\n");

    const nlohmann::json doc = read_json_settings(path.string());
    std::filesystem::remove(path);

    EXPECT_EQ(json_number(doc["bytetrack"], "track_buffer", 30.f), 45.f);
}

/**
 * @brief A missing file reads as an empty object, after one WARN naming it.
 */
TEST(Medium_JsonSettingsTest, MissingFileReadsAsAnEmptyObject)
{
    const SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace, {K_TRACKER_BRIDGE_LOG_CATEGORY});

    const nlohmann::json doc = read_json_settings("this/path/does/not/exist.json");

    EXPECT_TRUE(doc.is_object());
    EXPECT_TRUE(doc.empty());
    EXPECT_EQ(log.count("this/path/does/not/exist.json cannot be opened"), 1);
}

/**
 * @brief The named section is returned when the document holds it.
 */
TEST(Small_JsonSettingsTest, ThePresentSectionIsReturned)
{
    const nlohmann::json document = {{"bytetrack", {{"track_buffer", 45}}}};

    EXPECT_EQ(json_section(document, "bytetrack", "settings.json"), document["bytetrack"]);
}

/**
 * @brief A key that is absent gives the fallback, and says nothing.
 */
TEST(Small_JsonSettingsTest, AnAbsentKeyGivesTheFallbackSilently)
{
    const SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace, {K_TRACKER_BRIDGE_LOG_CATEGORY});
    const nlohmann::json section = {{"name", "x"}};

    EXPECT_EQ(json_number(section, "missing", 2.5f), 2.5f);
    EXPECT_EQ(log.records().size(), 0U);
}

/**
 * @brief A key that is not a number gives the fallback after one WARN naming the key and the
 *        value kept.
 */
TEST(Small_JsonSettingsTest, ANonNumericKeyGivesTheFallbackAfterOneWarn)
{
    const SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace, {K_TRACKER_BRIDGE_LOG_CATEGORY});
    const nlohmann::json section = {{"track_buffer", "many"}};

    EXPECT_EQ(json_number(section, "track_buffer", 30.f), 30.f);
    EXPECT_EQ(log.count("Setting track_buffer is not a number; keeping 30"), 1);
    EXPECT_EQ(log.level_of("is not a number"), SushiLog::LogLevel::Warn);
}

/**
 * @brief A document without the asked section gives an empty one after one WARN naming the file
 *        and the section.
 */
TEST(Small_JsonSettingsTest, AMissingSectionGivesAnEmptyOneAfterOneWarn)
{
    const SushiTrack::CapturedLog log(SushiLog::LogLevel::Trace, {K_TRACKER_BRIDGE_LOG_CATEGORY});
    const nlohmann::json document = parse("{ // no section\n}\n");

    const nlohmann::json& section = json_section(document, "bytetrack", "settings.json");

    EXPECT_TRUE(section.empty());
    EXPECT_EQ(log.count("Settings file settings.json has no \"bytetrack\" section"), 1);
    EXPECT_EQ(log.level_of("has no \"bytetrack\" section"), SushiLog::LogLevel::Warn);
}
