/**************************************************************************/
/* test_config_host.cpp                                                   */
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
 * @file test_config_host.cpp
 * @brief Tests InMemoryConfigHost, the adapter that describes a machine to the configuration
 *        search.
 * @author Mustafa Garip
 */

#include <istream>
#include <iterator>
#include <memory>
#include <string>
#include <system_error>

#include <gtest/gtest.h>

#include <SushiTrack/configuration/config_host.hpp>

/**
 * @brief A file added is found by its path, opens with the text it was added with, and a path
 *        never added neither exists nor opens.
 */
TEST(Small_InMemoryConfigHostTest, AddedFileExistsAndOpensWithItsText)
{
    SushiTrack::InMemoryConfigHost host;
    host.add_file("/etc/track.json", "{\"basic\": {}}");

    EXPECT_TRUE(host.exists("/etc/track.json"));
    EXPECT_FALSE(host.exists("/etc/other.json"));
    EXPECT_EQ(host.open("/etc/other.json"), nullptr);
    const std::unique_ptr<std::istream> file = host.open("/etc/track.json");
    ASSERT_NE(file, nullptr);
    EXPECT_EQ(std::string(std::istreambuf_iterator<char>(*file), {}), "{\"basic\": {}}");
}

/**
 * @brief A folder exists when a file lies below it, and a name that only starts like the folder
 *        does not.
 */
TEST(Small_InMemoryConfigHostTest, FolderExistsWhenAFileLiesBelowIt)
{
    SushiTrack::InMemoryConfigHost host;
    host.add_file("/work/a/sushitrack.json", "{}");

    EXPECT_TRUE(host.exists("/work/a"));
    EXPECT_TRUE(host.exists("/work"));
    EXPECT_FALSE(host.exists("/wor"));
    EXPECT_FALSE(host.exists("/work/ab"));
}

/**
 * @brief An empty variable reads as unset, and a variable set reads back.
 */
TEST(Small_InMemoryConfigHostTest, EmptyVariableReadsAsUnset)
{
    SushiTrack::InMemoryConfigHost host;
    host.set_variable("SET", "value");
    host.set_variable("EMPTY", "");

    EXPECT_EQ(host.variable("SET"), "value");
    EXPECT_TRUE(host.variable("EMPTY").empty());
    EXPECT_TRUE(host.variable("NEVER_SET").empty());
}

/**
 * @brief The working directory reads back with no error.
 */
TEST(Small_InMemoryConfigHostTest, WorkingDirectoryReadsBack)
{
    SushiTrack::InMemoryConfigHost host;
    host.set_working_directory("/work/a");
    std::error_code ec = std::make_error_code(std::errc::io_error);

    EXPECT_EQ(host.working_directory(ec), "/work/a");
    EXPECT_FALSE(ec);
}
