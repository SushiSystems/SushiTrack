/**************************************************************************/
/* logger.hpp                                                             */
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
 * @file logger.hpp
 * @brief Declares the SUSHITRACK_LOG macros on SushiLog and SUSHITRACK_THROW.
 * @author Mustafa Garip
 */

#pragma once

#include <cstdint>
#include <stdexcept>

#include <SushiLog/log/log_front.hpp>
#include <SushiLog/log/log_level.hpp>
#include <SushiLog/text/text_format.hpp>
#include <SushiLog/text/value_text.hpp>

#include <SushiTrack/logging/sushitrack_log_project.hpp>

#ifndef SUSHITRACK_ENABLE_LOGGING
#define SUSHITRACK_ENABLE_LOGGING 1
#endif

namespace SushiTrack
{
    /** @brief Holds the most detailed level this translation unit compiles SUSHITRACK_LOG calls for. */
    constexpr SushiLog::LogLevel K_LOG_LEVEL_CEILING =
        SushiLog::log_level_ceiling(SUSHITRACK_ENABLE_LOGGING != 0);
} // namespace SushiTrack

/** @brief Logs one record at @p level in @p category through SushiLog. */
#define SUSHITRACK_LOG_AT(level, category, ...)                                                    \
    SL_LOG_FRONT_AT(SushiTrack::K_LOG_LEVEL_CEILING, level, category, __VA_ARGS__)

/** @brief Logs step-by-step flow in @p category. */
#define SUSHITRACK_LOG_TRACE(category, ...)                                                        \
    SUSHITRACK_LOG_AT(SushiLog::LogLevel::Trace, category, __VA_ARGS__)

/** @brief Logs internal state at a decision point in @p category. */
#define SUSHITRACK_LOG_DEBUG(category, ...)                                                        \
    SUSHITRACK_LOG_AT(SushiLog::LogLevel::Debug, category, __VA_ARGS__)

/** @brief Logs a lifecycle event in @p category. */
#define SUSHITRACK_LOG_INFO(category, ...)                                                         \
    SUSHITRACK_LOG_AT(SushiLog::LogLevel::Info, category, __VA_ARGS__)

/** @brief Logs a recovery or a fallback in @p category. */
#define SUSHITRACK_LOG_WARN(category, ...)                                                         \
    SUSHITRACK_LOG_AT(SushiLog::LogLevel::Warn, category, __VA_ARGS__)

/** @brief Logs a failed operation in @p category. */
#define SUSHITRACK_LOG_ERROR(category, ...)                                                        \
    SUSHITRACK_LOG_AT(SushiLog::LogLevel::Error, category, __VA_ARGS__)

/** @brief Logs, in @p category, why the process cannot continue. */
#define SUSHITRACK_LOG_FATAL(category, ...)                                                        \
    SUSHITRACK_LOG_AT(SushiLog::LogLevel::Fatal, category, __VA_ARGS__)

/**
 * @brief Writes a WARN record when @p throttle admits it, and names how many were left out.
 *
 * The caller includes log_throttle.hpp. Takes at least one format argument after @p format; the
 * count of suppressed events follows
 * them and the macro adds its own `{}` to the end of the message.
 */
#define SUSHITRACK_LOG_WARN_LIMITED(category, throttle, format, ...)                               \
    do                                                                                             \
    {                                                                                              \
        std::uint64_t sushitrack_suppressed = 0U;                                                  \
        if ((throttle).admit(sushitrack_suppressed))                                               \
        {                                                                                          \
            SUSHITRACK_LOG_WARN(category, format " ({} similar events suppressed since the "       \
                                                 "last record)",                                   \
                                __VA_ARGS__, sushitrack_suppressed);                               \
        }                                                                                          \
    } while (false)

/**
 * @brief Throws std::runtime_error with the formatted message; the code that catches it logs it.
 */
#define SUSHITRACK_THROW(...) throw std::runtime_error(SushiLog::format_text(__VA_ARGS__))
