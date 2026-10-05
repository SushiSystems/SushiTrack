/****************************************************************/
/* logger.hpp                                                   */
/* SushiTrack - https://github.com/SushiSystems/SushiTrack      */
/* Copyright (c) 2026 Sushi Systems                             */
/* Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.    */
/* Commercial use requires a licence from Sushi Systems.        */
/****************************************************************/

/**
 * @file logger.hpp
 * @brief Internal logging utility for the SushiTrack engine.
 * @author Mustafa Garip
 */

#pragma once

#include <SushiTrack/sushitrack_c.h>

#include <cstdarg>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace SushiTrack
{
    /**
     * @brief Utility for handling system logs.
     */
    class Logger
    {
    public:
        /**
         * @brief Sets the global log callback.
         * @param callback Function pointer to the logging implementation.
         */
        static void setCallback(sushitrack_log_callback_t callback);

        /**
         * @brief Emits a log message with formatting support.
         * @param level   Severity level.
         * @param format  Printf-style format string.
         * @param ...     Variable arguments for formatting.
         */
        static void log(sushitrack_log_level_t level, const char* format, ...);

        /**
         * @brief Internal helper for va_list logging.
         */
        static void logV(sushitrack_log_level_t level, const char* format, va_list args);

        /**
         * @brief Logs an error and throws a std::runtime_error.
         * @param format Printf-style format string.
         * @param ...    Variable arguments.
         */
        static void throw_runtime_error(const char* format, ...);
    };
} // namespace SushiTrack

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
