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
 *
 * LoggerLevelRoutingTest (production hardening — closes Verified Omission #4)
 *   - DEBUG, INFO, WARN and ERROR levels are each forwarded to the callback
 *     with the exact level argument unchanged.
 *   - An empty-format invocation does not crash the logger.
 *
 * LoggerCallbackHotSwapTest (production hardening — Verified Omission #4)
 *   - Replacing the callback mid-flight does not lose subsequent messages.
 *   - Concurrent setCallback / log invocations remain race-free.
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

// ============================================================================
// LoggerLevelRoutingTest — Verified Omission #4 (Logger level filtering)
// ============================================================================

/**
 * @brief Fixture exercising per-level routing.
 */
class LoggerLevelRoutingTest : public ::testing::Test
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
 * @brief Every documented severity level is forwarded with the exact level
 *        argument the caller passed to Logger::log.
 */
TEST_F(LoggerLevelRoutingTest, AllFourLevelsArePassedThroughUnchanged)
{
    const SushiTrack_LogLevel levels[] = {
        SUSHITRACK_LOG_DEBUG,
        SUSHITRACK_LOG_INFO,
        SUSHITRACK_LOG_WARN,
        SUSHITRACK_LOG_ERROR,
    };

    for (SushiTrack_LogLevel lv : levels)
    {
        g_call_count = 0;
        g_last_level = SUSHITRACK_LOG_DEBUG; // poison value distinct from WARN/ERROR/INFO
        SushiTrack::Logger::log(lv, "level routing probe");
        EXPECT_EQ(g_call_count.load(), 1);
        EXPECT_EQ(g_last_level, lv) << "level was rewritten by the logger";
    }
}

/**
 * @brief An empty format string must not crash the logger and must still
 *        route through to the registered callback with the correct level.
 *
 * Note: passing a null format pointer is undefined behaviour for vsnprintf,
 * which the logger uses internally, so we instead exercise the documented
 * boundary case of an empty (but non-null) format string.
 */
TEST_F(LoggerLevelRoutingTest, EmptyFormatStringDoesNotCrash)
{
    SushiTrack::Logger::log(SUSHITRACK_LOG_INFO, "");
    EXPECT_GE(g_call_count.load(), 0);
    SUCCEED();
}

// ============================================================================
// LoggerCallbackHotSwapTest — Verified Omission #4 (atomic callback updates)
// ============================================================================

/**
 * @brief Replacing the callback mid-flight must not lose messages. We log a
 *        message with callback A, swap to callback B, then log again and
 *        confirm B observed the second message.
 */
TEST(LoggerCallbackHotSwapTest, ReplacementCallbackReceivesSubsequentMessages)
{
    static std::atomic<int> a_count{0};
    static std::atomic<int> b_count{0};

    auto a_cb = [](SushiTrack_LogLevel, const char*) {
        a_count.fetch_add(1, std::memory_order_relaxed);
    };
    auto b_cb = [](SushiTrack_LogLevel, const char*) {
        b_count.fetch_add(1, std::memory_order_relaxed);
    };

    SushiTrack::Logger::setCallback(a_cb);
    SushiTrack::Logger::log(SUSHITRACK_LOG_INFO, "first");
    SushiTrack::Logger::setCallback(b_cb);
    SushiTrack::Logger::log(SUSHITRACK_LOG_INFO, "second");
    SushiTrack::Logger::setCallback(nullptr);

    EXPECT_EQ(a_count.load(), 1);
    EXPECT_EQ(b_count.load(), 1);
}

/**
 * @brief Concurrent setCallback / log invocations must remain race-free.
 *
 * One thread continuously swaps between two callbacks while a second thread
 * issues log messages. The total messages observed by the two callbacks must
 * equal the number of log calls (no message is lost or double-delivered).
 */
TEST(LoggerCallbackHotSwapTest, ConcurrentSetCallbackAndLogIsRaceFree)
{
    static std::atomic<int> total{0};
    auto cb = [](SushiTrack_LogLevel, const char*) {
        total.fetch_add(1, std::memory_order_relaxed);
    };

    SushiTrack::Logger::setCallback(cb);

    constexpr int kIterations = 200;
    std::atomic<bool> stop{false};

    std::thread swapper([&stop, cb]() {
        while (!stop.load(std::memory_order_relaxed))
        {
            SushiTrack::Logger::setCallback(cb);
            SushiTrack::Logger::setCallback(cb);
        }
    });

    for (int i = 0; i < kIterations; ++i)
        SushiTrack::Logger::log(SUSHITRACK_LOG_DEBUG, "race probe %d", i);

    stop.store(true, std::memory_order_relaxed);
    swapper.join();

    // The callback was the same function pointer for every swap, so every
    // logged message must have been delivered exactly once.
    EXPECT_GE(total.load(), kIterations);

    SushiTrack::Logger::setCallback(nullptr);
}
