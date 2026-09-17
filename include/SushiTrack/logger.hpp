/**************************************************************************/
/* logger.hpp                                                             */
/**************************************************************************/
/*                          This file is part of:                         */
/*                               SushiTrack                               */
/*              https://github.com/SushiSystems/SushiTrack                */
/*                          https://sushisystems.io                       */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                          */
/**************************************************************************/

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
