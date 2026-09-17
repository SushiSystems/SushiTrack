/**************************************************************************/
/* logger.cpp                                                             */
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
 * @file logger.cpp
 * @brief Implements internal logging utilities and callback dispatch.
 * @author Mustafa Garip
 */

#include <SushiTrack/logger.hpp>

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <stdexcept>
#include <vector>

namespace SushiTrack
{
    static std::atomic<sushitrack_log_callback_t> g_log_callback{nullptr};

    /**
     * @brief Registers a global callback for engine log redirection.
     * @param callback Function pointer conforming to sushitrack_log_callback_t signature.
     */
    void Logger::setCallback(sushitrack_log_callback_t callback)
    {
        g_log_callback.store(callback, std::memory_order_release);
    }

    /**
     * @brief Dispatches a formatted log message to the registered callback.
     * @param level Severity level of the log message.
     * @param format Printf-style format string.
     */
    void Logger::log(sushitrack_log_level_t level, const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        logV(level, format, args);
        va_end(args);
    }

    /**
     * @brief Performs log message formatting and delivery.
     * @param level Severity level of the log message.
     * @param format Printf-style format string.
     * @param args Argument list for formatting.
     */
    void Logger::logV(sushitrack_log_level_t level, const char* format, va_list args)
    {
        sushitrack_log_callback_t cb = g_log_callback.load(std::memory_order_acquire);
        if (!cb)
        {
            return;
        }

        va_list args_copy;
        va_copy(args_copy, args);
        int size = std::vsnprintf(nullptr, 0, format, args_copy);
        va_end(args_copy);

        if (size < 0)
        {
            return;
        }

        std::vector<char> buffer(static_cast<size_t>(size) + 1);
        std::vsnprintf(buffer.data(), buffer.size(), format, args);

        cb(level, buffer.data());
    }

    /**
     * @brief Logs an error and terminates execution flow with an exception.
     * @param format Printf-style format string.
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

} // namespace SushiTrack
