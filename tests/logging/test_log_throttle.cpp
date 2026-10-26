/**************************************************************************/
/* test_log_throttle.cpp                                                  */
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
 * @file test_log_throttle.cpp
 * @brief Tests LogThrottle and the SUSHITRACK_LOG_WARN_LIMITED macro built on it.
 * @author Mustafa Garip
 */

#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/logging/log_throttle.hpp>
#include <SushiTrack/logging/logger.hpp>
#include <SushiTrack/tracking/tracking_log_category.hpp>

#include <captured_log.hpp>

using namespace SushiTrack;

/**
 * @brief The first event is admitted with nothing suppressed, then one in `every` is, each
 *        naming the events left out since the last one.
 */
TEST(Small_LogThrottleTest, AdmitsTheFirstEventAndOneInEvery)
{
    reset_log_throttles();
    LogThrottle throttle(10U);
    std::vector<std::uint64_t> admitted;
    for (int event = 1; event <= 25; ++event)
    {
        std::uint64_t suppressed = 99U;
        if (throttle.admit(suppressed))
            admitted.push_back(suppressed);
    }

    ASSERT_EQ(admitted.size(), 3U);
    EXPECT_EQ(admitted[0], 0U);
    EXPECT_EQ(admitted[1], 9U);
    EXPECT_EQ(admitted[2], 9U);
}

/**
 * @brief A throttle given zero treats it as one, so it admits every event.
 */
TEST(Small_LogThrottleTest, ZeroEveryAdmitsEveryEvent)
{
    reset_log_throttles();
    LogThrottle throttle(0U);
    std::uint64_t suppressed = 0U;
    EXPECT_TRUE(throttle.admit(suppressed));
    EXPECT_TRUE(throttle.admit(suppressed));
    EXPECT_TRUE(throttle.admit(suppressed));
}

/**
 * @brief reset_log_throttles() makes a throttle that already admitted its first event admit again.
 */
TEST(Small_LogThrottleTest, ResetMakesTheNextEventTheFirst)
{
    reset_log_throttles();
    LogThrottle throttle(100U);
    std::uint64_t suppressed = 0U;
    EXPECT_TRUE(throttle.admit(suppressed));
    EXPECT_FALSE(throttle.admit(suppressed));

    reset_log_throttles();

    EXPECT_TRUE(throttle.admit(suppressed));
    EXPECT_EQ(suppressed, 0U);
}

/**
 * @brief The macro writes one WARN per thousand events and ends it with the suppressed count.
 */
TEST(Small_LogThrottleTest, MacroLogsOnceAThousandTimesAndNamesTheSuppressedCount)
{
    CapturedLog log;
    static LogThrottle throttle;
    for (int event = 0; event < 2001; ++event)
        SUSHITRACK_LOG_WARN_LIMITED(K_TRACKING_LOG_CATEGORY, throttle, "event {}", event);

    const auto records = log.records();
    ASSERT_EQ(records.size(), 3U);
    EXPECT_EQ(records[0].level, SushiLog::LogLevel::Warn);
    EXPECT_EQ(records[0].message, "event 0 (0 similar events suppressed since the last record)");
    EXPECT_EQ(records[1].message,
              "event 1000 (999 similar events suppressed since the last record)");
    EXPECT_EQ(records[2].message,
              "event 2000 (999 similar events suppressed since the last record)");
}
