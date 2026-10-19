/**************************************************************************/
/* TestLogger.cpp                                                         */
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
 * @file TestLogger.cpp
 * @brief Tests the SUSHITRACK_LOG macros and SUSHITRACK_THROW on SushiLog.
 * @author Mustafa Garip
 */

#include <stdexcept>
#include <string>
#include <thread>

#include <gtest/gtest.h>

#include <SushiTrack/logging/logger.hpp>

#include "captured_log.hpp"

using namespace SushiTrack;

/**
 * @brief A WARN call reaches the sink with SushiTrack's project, module and formatted message.
 */
TEST(LoggerTest, WarnCarriesTheProjectModuleAndFormattedMessage)
{
    CapturedLog log;
    SUSHITRACK_LOG_WARN(K_SUSHITRACK_LOG_CATEGORY, "track {} lost after {} frames", 7, 30);

    const auto records = log.records();
    ASSERT_EQ(records.size(), 1u);
    EXPECT_EQ(records[0].level, SushiLog::LogLevel::Warn);
    EXPECT_EQ(records[0].project, "sushitrack");
    EXPECT_EQ(records[0].module, "sushitrack");
    EXPECT_EQ(records[0].message, "track 7 lost after 30 frames");
}

/**
 * @brief A record below the category's level is not delivered.
 */
TEST(LoggerTest, RecordBelowTheCategoryLevelIsDropped)
{
    CapturedLog log(SushiLog::LogLevel::Error);
    SUSHITRACK_LOG_WARN(K_SUSHITRACK_LOG_CATEGORY, "below the level");
    SUSHITRACK_LOG_ERROR(K_SUSHITRACK_LOG_CATEGORY, "at the level");

    EXPECT_EQ(log.count("below the level"), 0);
    EXPECT_EQ(log.count("at the level"), 1);
}

/**
 * @brief SUSHITRACK_THROW throws std::runtime_error holding the formatted message.
 */
TEST(LoggerTest, ThrowCarriesTheFormattedMessage)
{
    try
    {
        SUSHITRACK_THROW("sentinel_token_{}", 99);
        FAIL() << "SUSHITRACK_THROW did not throw";
    }
    catch (const std::runtime_error& error)
    {
        EXPECT_NE(std::string(error.what()).find("sentinel_token_99"), std::string::npos);
    }
}

/**
 * @brief SUSHITRACK_THROW only throws; the code that catches it logs it.
 */
TEST(LoggerTest, ThrowLogsNothing)
{
    CapturedLog log;
    EXPECT_THROW(SUSHITRACK_THROW("thrown {}", 1), std::runtime_error);
    EXPECT_TRUE(log.records().empty());
}

/**
 * @brief Calls from two threads both reach the sink, every record intact.
 */
TEST(LoggerTest, ConcurrentCallsReachTheSink)
{
    CapturedLog log;
    static constexpr int K_PER_THREAD = 50;
    std::thread first(
        []
        {
            for (int i = 0; i < K_PER_THREAD; ++i)
                SUSHITRACK_LOG_ERROR(K_SUSHITRACK_LOG_CATEGORY, "t1 msg {}", i);
        });
    std::thread second(
        []
        {
            for (int i = 0; i < K_PER_THREAD; ++i)
                SUSHITRACK_LOG_ERROR(K_SUSHITRACK_LOG_CATEGORY, "t2 msg {}", i);
        });
    first.join();
    second.join();

    EXPECT_EQ(log.count("t1 msg"), K_PER_THREAD);
    EXPECT_EQ(log.count("t2 msg"), K_PER_THREAD);
}
