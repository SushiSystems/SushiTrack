/**************************************************************************/
/* test_logger.cpp                                                        */
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
 * @file test_logger.cpp
 * @brief Tests the SUSHITRACK_LOG macros and SUSHITRACK_THROW.
 * @author Mustafa Garip
 */

#include <algorithm>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/logging/logger.hpp>
#include <SushiTrack/tracking/tracking_log_category.hpp>

#include <captured_log.hpp>

using namespace SushiTrack;

namespace
{
    /** @brief Returns the message of the std::runtime_error SUSHITRACK_THROW raises. */
    std::string thrown_message()
    {
        try
        {
            SUSHITRACK_THROW("sentinel_token_{}", 99);
        }
        catch (const std::runtime_error& error)
        {
            return error.what();
        }
        return "SUSHITRACK_THROW did not throw";
    }

    /** @brief Logs @p count ERROR records "<prefix> msg <i>" for i from 0. */
    void log_numbered(const char* prefix, int count)
    {
        for (int i = 0; i < count; ++i)
            SUSHITRACK_LOG_ERROR(K_TRACKING_LOG_CATEGORY, "{} msg {}", prefix, i);
    }

    /** @brief Returns the messages "<prefix> msg <i>" for i below @p count. */
    std::vector<std::string> numbered_messages(const std::string& prefix, int count)
    {
        std::vector<std::string> messages;
        for (int i = 0; i < count; ++i)
            messages.push_back(prefix + " msg " + std::to_string(i));
        return messages;
    }

    /** @brief Returns the messages of @p records, sorted. */
    std::vector<std::string> sorted_messages(const std::vector<SushiLog::CapturedRecord>& records)
    {
        std::vector<std::string> messages;
        for (const SushiLog::CapturedRecord& record : records)
            messages.push_back(record.message);
        std::sort(messages.begin(), messages.end());
        return messages;
    }
} // namespace

/**
 * @brief A WARN call reaches the sink with SushiTrack's project, module and formatted message.
 */
TEST(Small_LoggerTest, WarnCarriesTheProjectModuleAndFormattedMessage)
{
    CapturedLog log;
    SUSHITRACK_LOG_WARN(K_TRACKING_LOG_CATEGORY, "track {} lost after {} frames", 7, 30);

    const auto records = log.records();
    ASSERT_EQ(records.size(), 1u);
    EXPECT_EQ(records[0].level, SushiLog::LogLevel::Warn);
    EXPECT_EQ(records[0].project, "sushitrack");
    EXPECT_EQ(records[0].module, "tracking");
    EXPECT_EQ(records[0].message, "track 7 lost after 30 frames");
}

/**
 * @brief A record below the category's level is not delivered.
 */
TEST(Small_LoggerTest, RecordBelowTheCategoryLevelIsDropped)
{
    CapturedLog log(SushiLog::LogLevel::Error);
    SUSHITRACK_LOG_WARN(K_TRACKING_LOG_CATEGORY, "below the level");
    SUSHITRACK_LOG_ERROR(K_TRACKING_LOG_CATEGORY, "at the level");

    EXPECT_EQ(log.count("below the level"), 0);
    EXPECT_EQ(log.count("at the level"), 1);
}

/**
 * @brief SUSHITRACK_THROW throws std::runtime_error holding the formatted message.
 */
TEST(Small_LoggerTest, ThrowCarriesTheFormattedMessage)
{
    EXPECT_EQ(thrown_message(), "sentinel_token_99");
}

/**
 * @brief SUSHITRACK_THROW only throws; the code that catches it logs it.
 */
TEST(Small_LoggerTest, ThrowLogsNothing)
{
    CapturedLog log;
    EXPECT_THROW(SUSHITRACK_THROW("thrown {}", 1), std::runtime_error);
    EXPECT_TRUE(log.records().empty());
}

/**
 * @brief Calls from two threads both reach the sink, and every record arrives whole.
 *
 * Each thread logs the messages "t1 msg 0" to "t1 msg 49" and "t2 msg 0" to "t2 msg 49". The
 * sink holds exactly those hundred messages, so none is lost, doubled or cut.
 */
TEST(Medium_LoggerConcurrencyTest, CallsFromTwoThreadsReachTheSink)
{
    CapturedLog log;
    constexpr int K_PER_THREAD = 50;
    std::thread first(log_numbered, "t1", K_PER_THREAD);
    std::thread second(log_numbered, "t2", K_PER_THREAD);
    first.join();
    second.join();

    std::vector<std::string> expected = numbered_messages("t1", K_PER_THREAD);
    const std::vector<std::string> from_second = numbered_messages("t2", K_PER_THREAD);
    expected.insert(expected.end(), from_second.begin(), from_second.end());
    std::sort(expected.begin(), expected.end());
    EXPECT_EQ(sorted_messages(log.records()), expected);
}
