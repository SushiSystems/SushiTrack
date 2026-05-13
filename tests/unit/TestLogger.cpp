/**
 * @file      TestLogger.cpp
 * @brief     Unit tests for the SushiTrack Logger singleton utility.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      13.05.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 *
 * Coverage
 * --------
 * LoggerCallbackTest
 *   - A registered callback receives every message emitted by Logger::log.
 *   - The level value delivered to the callback matches the level passed to log.
 *   - Replacing the callback with nullptr suppresses all output (no-op path).
 *   - Logger::throw_runtime_error throws std::runtime_error.
 *   - The thrown exception message contains the format string text.
 *
 * LoggerThreadSafetyTest
 *   - Concurrent calls from two threads both deliver messages to the callback
 *     without data races or missed invocations.
 */

#include <gtest/gtest.h>
#include "SushiTrack/Logger.h"
#include "SushiTrack/SushiTrackTypes.h"
#include <atomic>
#include <string>
#include <thread>

namespace
{

static std::atomic<int>   g_call_count{0};
static SushiTrack_LogLevel g_last_level{SUSHITRACK_LOG_DEBUG};
static std::string        g_last_message;

/**
 * @brief Test callback that records invocation metadata into module-level state.
 * @param level   Severity level forwarded from the logger.
 * @param message Formatted message string forwarded from the logger.
 */
void recordingCallback(SushiTrack_LogLevel level, const char* message)
{
    g_last_level   = level;
    g_last_message = message ? message : "";
    g_call_count.fetch_add(1, std::memory_order_relaxed);
}

}

/**
 * @brief Fixture that installs and tears down the recording callback.
 */
class LoggerCallbackTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        g_call_count = 0;
        g_last_level = SUSHITRACK_LOG_DEBUG;
        g_last_message.clear();
        SushiTrack::Logger::setCallback(recordingCallback);
    }

    void TearDown() override
    {
        SushiTrack::Logger::setCallback(nullptr);
    }
};

/**
 * @brief A registered callback receives a message emitted via Logger::log.
 */
TEST_F(LoggerCallbackTest, CallbackReceivesLoggedMessage)
{
    SushiTrack::Logger::log(SUSHITRACK_LOG_INFO, "hello %s", "world");
    EXPECT_EQ(g_call_count.load(), 1);
}

/**
 * @brief The level delivered to the callback matches the level passed to log.
 */
TEST_F(LoggerCallbackTest, CallbackReceivesCorrectLevel)
{
    SushiTrack::Logger::log(SUSHITRACK_LOG_WARN, "warning message");
    EXPECT_EQ(g_last_level, SUSHITRACK_LOG_WARN);
}

/**
 * @brief Setting the callback to nullptr suppresses output; the callback is not invoked.
 */
TEST_F(LoggerCallbackTest, NullCallbackSuppressesOutput)
{
    SushiTrack::Logger::setCallback(nullptr);
    SushiTrack::Logger::log(SUSHITRACK_LOG_ERROR, "should not arrive");
    EXPECT_EQ(g_call_count.load(), 0);
}

/**
 * @brief Logger::throw_runtime_error throws a std::runtime_error.
 */
TEST_F(LoggerCallbackTest, ThrowRuntimeErrorThrows)
{
    EXPECT_THROW(
        SushiTrack::Logger::throw_runtime_error("fatal error %d", 42),
        std::runtime_error
    );
}

/**
 * @brief The exception thrown by throw_runtime_error contains the formatted text.
 */
TEST_F(LoggerCallbackTest, ThrowRuntimeErrorMessageContainsFormattedText)
{
    try
    {
        SushiTrack::Logger::throw_runtime_error("sentinel_token_%d", 99);
        FAIL() << "Expected std::runtime_error was not thrown";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_NE(std::string(e.what()).find("sentinel_token_"), std::string::npos);
    }
}

/**
 * @brief Concurrent Logger::log calls from two threads both reach the callback
 *        without data races or missed invocations.
 */
TEST(LoggerThreadSafetyTest, ConcurrentCallsDeliverAllMessages)
{
    g_call_count.store(0);
    SushiTrack::Logger::setCallback(recordingCallback);

    constexpr int kMessagesPerThread = 50;

    std::thread t1([kMessagesPerThread]() {
        for (int i = 0; i < kMessagesPerThread; ++i)
            SushiTrack::Logger::log(SUSHITRACK_LOG_DEBUG, "t1 msg %d", i);
    });
    std::thread t2([kMessagesPerThread]() {
        for (int i = 0; i < kMessagesPerThread; ++i)
            SushiTrack::Logger::log(SUSHITRACK_LOG_INFO, "t2 msg %d", i);
    });

    t1.join();
    t2.join();

    EXPECT_EQ(g_call_count.load(), 2 * kMessagesPerThread);

    SushiTrack::Logger::setCallback(nullptr);
}
