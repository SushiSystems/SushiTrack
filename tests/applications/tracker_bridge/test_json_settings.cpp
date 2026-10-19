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
/* Licensed under the PolyForm Noncommercial License 1.0.0 (the           */
/* "License"); you may not use this file except in compliance with the    */
/* License. You may obtain a copy of the License at                       */
/*                                                                        */
/*     https://polyformproject.org/licenses/noncommercial/1.0.0           */
/*                                                                        */
/* Noncommercial use is free. Commercial use requires a separate licence  */
/* from Sushi Systems; see COMMERCIAL.md. The software is provided        */
/* "as is", without warranty of any kind.                                 */
/**************************************************************************/

/**
 * @file test_json_settings.cpp
 * @brief Tests read_json_settings and json_number.
 * @author Mustafa Garip
 */

#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include <json_settings.hpp>

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
 * @brief A missing file reads as an empty object.
 */
TEST(Small_JsonSettingsTest, MissingFileReadsAsAnEmptyObject)
{
    const nlohmann::json doc = read_json_settings("this/path/does/not/exist.json");
    EXPECT_TRUE(doc.is_object());
    EXPECT_TRUE(doc.empty());
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
