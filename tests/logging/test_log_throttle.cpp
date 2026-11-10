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

namespace
{
    /**
     * @brief Offers @p events events to @p throttle in @p generation and returns the suppressed
     *        count of each event the throttle admitted, in order.
     */
    std::vector<std::uint64_t> admitted_counts(LogThrottle& throttle, int events,
                                               std::uint64_t generation)
    {
        std::vector<std::uint64_t> admitted;
        for (int event = 0; event < events; ++event)
        {
            std::uint64_t suppressed = 99U;
            if (throttle.admit(suppressed, generation))
                admitted.push_back(suppressed);
        }
        return admitted;
    }

    /** @brief Logs events 0 to @p events - 1 through the limited WARN macro with @p throttle. */
    void log_limited_events(LogThrottle& throttle, int events)
    {
        for (int event = 0; event < events; ++event)
            SUSHITRACK_LOG_WARN_LIMITED(K_TRACKING_LOG_CATEGORY, throttle, "event {}", event);
    }
} // namespace

/**
 * @brief The first event is admitted with nothing suppressed, then one in `every` is, each
 *        naming the events left out since the last one.
 *
 * With every = 10, events 1, 11 and 21 of 25 are admitted. The first has nothing before it, so
 * its count is 0; each later one follows 9 silent events.
 */
TEST(Small_LogThrottleTest, AdmitsTheFirstEventAndOneInEvery)
{
    LogThrottle throttle(10U);

    EXPECT_EQ(admitted_counts(throttle, 25, 0U), (std::vector<std::uint64_t>{0U, 9U, 9U}));
}

/**
 * @brief A throttle given zero treats it as one, so it admits every event.
 */
TEST(Small_LogThrottleTest, ZeroEveryAdmitsEveryEvent)
{
    LogThrottle throttle(0U);

    EXPECT_EQ(admitted_counts(throttle, 3, 0U), (std::vector<std::uint64_t>{0U, 0U, 0U}));
}

/**
 * @brief A new generation makes a throttle that already admitted its first event admit again.
 */
TEST(Small_LogThrottleTest, ANewGenerationMakesTheNextEventTheFirst)
{
    LogThrottle throttle(100U);
    std::uint64_t suppressed = 0U;
    EXPECT_TRUE(throttle.admit(suppressed, 0U));
    EXPECT_FALSE(throttle.admit(suppressed, 0U));

    EXPECT_TRUE(throttle.admit(suppressed, 1U));
    EXPECT_EQ(suppressed, 0U);
}

/**
 * @brief A new generation restarts every throttle, not only the one that asks.
 */
TEST(Small_LogThrottleTest, ANewGenerationRestartsEveryThrottle)
{
    LogThrottle first(100U);
    LogThrottle second(100U);

    EXPECT_EQ(admitted_counts(first, 2, 0U), (std::vector<std::uint64_t>{0U}));
    EXPECT_EQ(admitted_counts(second, 2, 0U), (std::vector<std::uint64_t>{0U}));

    EXPECT_EQ(admitted_counts(first, 1, 1U), (std::vector<std::uint64_t>{0U}));
    EXPECT_EQ(admitted_counts(second, 1, 1U), (std::vector<std::uint64_t>{0U}));
}

/**
 * @brief The macro writes one WARN per thousand events and ends it with the suppressed count.
 *
 * Of events 0 to 2000 the throttle admits the 1st, 1001st and 2001st, which are events 0, 1000
 * and 2000; the last two follow 999 silent events each.
 */
TEST(Small_LogThrottleTest, MacroLogsOnceAThousandTimesAndNamesTheSuppressedCount)
{
    CapturedLog log;
    LogThrottle throttle;

    log_limited_events(throttle, 2001);

    const auto records = log.records();
    ASSERT_EQ(records.size(), 3U);
    EXPECT_EQ(records[0].level, SushiLog::LogLevel::Warn);
    EXPECT_EQ(records[0].message, "event 0 (0 similar events suppressed since the last record)");
    EXPECT_EQ(records[1].message,
              "event 1000 (999 similar events suppressed since the last record)");
    EXPECT_EQ(records[2].message,
              "event 2000 (999 similar events suppressed since the last record)");
}
