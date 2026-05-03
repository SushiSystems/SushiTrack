/**
 * @file      Logger.h
 * @brief     Internal logging utility for the SushiTrack engine.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      29.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#pragma once

#include "SushiTrack/SushiTrackTypes.h"
#include <string>
#include <cstdarg>
#include <stdexcept>
#include <cstdio>

namespace SushiTrack
{
    /**
     * @brief Singleton-like utility for handling system logs.
     */
    class Logger
    {
    public:
        /**
         * @brief Sets the global log callback.
         * @param callback Function pointer to the logging implementation.
         */
        static void setCallback(SushiTrack_LogCallback callback);

        /**
         * @brief Emits a log message with formatting support.
         * @param level   Severity level.
         * @param format  Printf-style format string.
         * @param ...     Variable arguments for formatting.
         */
        static void log(SushiTrack_LogLevel level, const char* format, ...);

        /**
         * @brief Internal helper for va_list logging.
         */
        static void logV(SushiTrack_LogLevel level, const char* format, va_list args);

        /**
         * @brief Logs an error and throws a std::runtime_error.
         * @param format Printf-style format string.
         * @param ...    Variable arguments.
         */
        static void throw_runtime_error(const char* format, ...);
    };
}

/**
 * @brief Convenience macros for logging.
 */
#define SUSHITRACK_LOG_DEBUG(...) SushiTrack::Logger::log(SUSHITRACK_LOG_DEBUG, __VA_ARGS__)
#define SUSHITRACK_LOG_INFO(...)  SushiTrack::Logger::log(SUSHITRACK_LOG_INFO,  __VA_ARGS__)
#define SUSHITRACK_LOG_WARN(...)  SushiTrack::Logger::log(SUSHITRACK_LOG_WARN,  __VA_ARGS__)
#define SUSHITRACK_LOG_ERROR(...) SushiTrack::Logger::log(SUSHITRACK_LOG_ERROR, __VA_ARGS__)

/**
 * @brief Logs an error and throws a std::runtime_error.
 */
#define SUSHITRACK_RUNTIME_ERROR(...) SushiTrack::Logger::throw_runtime_error(__VA_ARGS__)
