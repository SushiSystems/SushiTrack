/**
 * @file      Logger.cpp
 * @brief     Implementation of the internal logging utility.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      29.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#include "SushiTrack/Logger.h"
#include <atomic>
#include <vector>
#include <cstdio>

namespace SushiTrack
{
    static std::atomic<SushiTrack_LogCallback> g_log_callback { nullptr };

    /**
     * @brief Registers a global callback for engine log redirection.
     * 
     * Stores the provided function pointer in an atomic variable with release 
     * memory ordering to ensure thread-safe access by the tracking engine.
     * 
     * @param callback Function pointer conforming to the SushiTrack_LogCallback signature.
     */
    void Logger::setCallback(SushiTrack_LogCallback callback)
    {
        g_log_callback.store(callback, std::memory_order_release);
    }

    /**
     * @brief Dispatches a formatted log message to the registered callback.
     * 
     * Processes variable arguments using va_list and delegates the internal 
     * formatting and delivery to the logV helper.
     * 
     * @param level  Severity level of the log message.
     * @param format Printf-style format string.
     * @param ...    Variable arguments for the format string.
     */
    void Logger::log(SushiTrack_LogLevel level, const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        logV(level, format, args);
        va_end(args);
    }

    /**
     * @brief Performs log message formatting and delivery.
     * 
     * Dynamically determines the required buffer size, formats the message string, 
     * and invokes the active callback. Employs acquire memory ordering when 
     * retrieving the callback pointer.
     * 
     * @param level  Severity level of the log message.
     * @param format Printf-style format string.
     * @param args   Argument list for formatting.
     */
    void Logger::logV(SushiTrack_LogLevel level, const char* format, va_list args)
    {
        SushiTrack_LogCallback cb = g_log_callback.load(std::memory_order_acquire);
        if (!cb) return;

        // Determine required buffer size
        va_list args_copy;
        va_copy(args_copy, args);
        int size = std::vsnprintf(nullptr, 0, format, args_copy);
        va_end(args_copy);

        if (size < 0) return;

        std::vector<char> buffer(static_cast<size_t>(size) + 1);
        std::vsnprintf(buffer.data(), buffer.size(), format, args);

        cb(level, buffer.data());
    }

    /**
     * @brief Logs an error and terminates execution flow with an exception.
     * 
     * Formats the error message, dispatches it to the logging system at the 
     * ERROR level, and throws a std::runtime_error containing the same message.
     * 
     * @param format Printf-style format string.
     * @param ...    Variable arguments for the format string.
     * @throws std::runtime_error Always thrown after logging.
     */
    void Logger::throw_runtime_error(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        logV(SUSHITRACK_LOG_ERROR, format, args);
        va_end(args);

        va_start(args, format);
        char buf[1024];
        std::vsnprintf(buf, sizeof(buf), format, args);
        va_end(args);
        throw std::runtime_error(buf);
    }
}
