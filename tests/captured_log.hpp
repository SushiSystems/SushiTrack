/**************************************************************************/
/* captured_log.hpp                                                       */
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
 * @file captured_log.hpp
 * @brief Declares CapturedLog, which collects SushiTrack's log records for one test.
 * @author Mustafa Garip
 */

#pragma once

#include <array>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include <SushiLog/log/log_calls.hpp>
#include <SushiLog/log/log_level.hpp>
#include <SushiLog/log/log_record.hpp>

#include <SushiTrack/association/association_log_category.hpp>
#include <SushiTrack/c_api/c_api_log_category.hpp>
#include <SushiTrack/configuration/configuration_log_category.hpp>
#include <SushiTrack/filtering/filtering_log_category.hpp>
#include <SushiTrack/logging/log_throttle.hpp>
#include <SushiTrack/tracking/tracking_log_category.hpp>

namespace SushiTrack
{
    /**
     * @brief Holds one record a test received: its level, project, module and message.
     */
    struct CapturedRecord
    {
            SushiLog::LogLevel level; /**< The record's level. */
            std::string project;      /**< The project of its category. */
            std::string module;       /**< The module of its category. */
            std::string message;      /**< The formatted message alone. */
    };

    /**
     * @brief Collects every record from construction to destruction, delivered synchronously.
     *
     * Sets every SushiTrack category to @p level for its lifetime, makes every LogThrottle admit
     * its next event, and restores the stderr sink, the WARN level and queued delivery when
     * destroyed.
     */
    class CapturedLog final
    {
        public:
            /// @brief Starts collecting records at @p level and above, also under @p extra.
            explicit CapturedLog(SushiLog::LogLevel level = SushiLog::LogLevel::Trace,
                                 std::vector<SushiLog::LogCategory> extra = {})
                : extra_(std::move(extra))
            {
                reset_log_throttles();
                SushiLog::set_log_synchronous(true);
                for (const SushiLog::LogCategory& category : K_CATEGORIES)
                    SushiLog::set_log_level(category, level);
                for (const SushiLog::LogCategory& category : extra_)
                    SushiLog::set_log_level(category, level);
                SushiLog::set_log_sink(
                    [this](const SushiLog::LogRecord& record)
                    {
                        const std::lock_guard<std::mutex> lock(mutex_);
                        records_.push_back({record.level, std::string(record.project),
                                            std::string(record.module),
                                            std::string(record.message)});
                    });
            }

            CapturedLog(const CapturedLog&) = delete;
            CapturedLog& operator=(const CapturedLog&) = delete;
            CapturedLog(CapturedLog&&) = delete;
            CapturedLog& operator=(CapturedLog&&) = delete;

            /// @brief Restores the stderr sink, the WARN level and queued delivery.
            ~CapturedLog()
            {
                SushiLog::set_log_sink(nullptr);
                for (const SushiLog::LogCategory& category : K_CATEGORIES)
                    SushiLog::set_log_level(category, SushiLog::LogLevel::Warn);
                for (const SushiLog::LogCategory& category : extra_)
                    SushiLog::set_log_level(category, SushiLog::LogLevel::Warn);
                SushiLog::set_log_synchronous(false);
            }

            /// @brief Returns a copy of the records collected so far.
            [[nodiscard]] std::vector<CapturedRecord> records() const
            {
                const std::lock_guard<std::mutex> lock(mutex_);
                return records_;
            }

            /// @brief Returns how many records hold @p text in their message.
            [[nodiscard]] int count(const std::string& text) const
            {
                int found = 0;
                for (const CapturedRecord& record : records())
                {
                    if (record.message.find(text) != std::string::npos)
                        ++found;
                }
                return found;
            }

            /// @brief Returns how many records of module @p module hold @p text in their message.
            [[nodiscard]] int count_from(const std::string& module, const std::string& text) const
            {
                int found = 0;
                for (const CapturedRecord& record : records())
                {
                    if (record.module == module && record.message.find(text) != std::string::npos)
                        ++found;
                }
                return found;
            }

            /// @brief Returns the level of the first record holding @p text, or Off with none.
            [[nodiscard]] SushiLog::LogLevel level_of(const std::string& text) const
            {
                for (const CapturedRecord& record : records())
                {
                    if (record.message.find(text) != std::string::npos)
                        return record.level;
                }
                return SushiLog::LogLevel::Off;
            }

            /// @brief Returns the last message holding @p text, or an empty string.
            [[nodiscard]] std::string last(const std::string& text) const
            {
                std::string found;
                for (const CapturedRecord& record : records())
                {
                    if (record.message.find(text) != std::string::npos)
                        found = record.message;
                }
                return found;
            }

        private:
            /// @brief Every category SushiTrack logs under.
            static constexpr std::array<SushiLog::LogCategory, 5> K_CATEGORIES = {
                K_TRACKING_LOG_CATEGORY, K_ASSOCIATION_LOG_CATEGORY, K_FILTERING_LOG_CATEGORY,
                K_CONFIGURATION_LOG_CATEGORY, K_C_API_LOG_CATEGORY};

            /// @brief The categories of the applications under test, set beside K_CATEGORIES.
            std::vector<SushiLog::LogCategory> extra_;
            /// @brief Guards records_ against the logger thread.
            mutable std::mutex mutex_;
            /// @brief The records collected so far.
            std::vector<CapturedRecord> records_;
    };
} // namespace SushiTrack
