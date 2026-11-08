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
 * @file captured_log.hpp
 * @brief Declares CapturedLog, which collects SushiTrack's log records for one test through
 *        SushiLog's scoped in-memory sink.
 * @author Mustafa Garip
 */

#pragma once

#include <array>
#include <string>
#include <utility>
#include <vector>

#include <SushiLog/log/log_calls.hpp>
#include <SushiLog/log/log_capture.hpp>
#include <SushiLog/log/log_category.hpp>
#include <SushiLog/log/log_level.hpp>

#include <SushiTrack/association/association_log_category.hpp>
#include <SushiTrack/c_api/c_api_log_category.hpp>
#include <SushiTrack/configuration/configuration_log_category.hpp>
#include <SushiTrack/filtering/filtering_log_category.hpp>
#include <SushiTrack/logging/log_throttle.hpp>
#include <SushiTrack/tracking/tracking_log_category.hpp>

namespace SushiTrack
{
    /**
     * @brief Collects every record from construction to destruction, delivered synchronously,
     *        through a SushiLog::LogCapture.
     *
     * Sets every SushiTrack category, and each one in @p extra, to @p level for its lifetime and
     * makes every LogThrottle admit its next event. On exit it puts back the levels it found, and
     * the capture puts back the sink and the delivery mode.
     */
    class CapturedLog final
    {
        public:
            /// @brief Starts collecting records at @p level and above, also under @p extra.
            explicit CapturedLog(SushiLog::LogLevel level = SushiLog::LogLevel::Trace,
                                 const std::vector<SushiLog::LogCategory>& extra = {})
            {
                reset_log_throttles();
                for (const SushiLog::LogCategory& category : K_CATEGORIES)
                    set_level(category, level);
                for (const SushiLog::LogCategory& category : extra)
                    set_level(category, level);
            }

            CapturedLog(const CapturedLog&) = delete;
            CapturedLog& operator=(const CapturedLog&) = delete;
            CapturedLog(CapturedLog&&) = delete;
            CapturedLog& operator=(CapturedLog&&) = delete;

            /// @brief Puts back the level each category had on entry, the last one set first.
            ~CapturedLog()
            {
                for (auto found = found_levels_.rbegin(); found != found_levels_.rend(); ++found)
                    SushiLog::set_log_level(found->first, found->second);
            }

            /// @brief Returns a copy of the records collected so far.
            [[nodiscard]] std::vector<SushiLog::CapturedRecord> records() const
            {
                return capture_.records();
            }

            /// @brief Returns how many records hold @p text in their message.
            [[nodiscard]] int count(const std::string& text) const
            {
                int found = 0;
                for (const SushiLog::CapturedRecord& record : records())
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
                for (const SushiLog::CapturedRecord& record : records())
                {
                    if (record.module == module && record.message.find(text) != std::string::npos)
                        ++found;
                }
                return found;
            }

            /// @brief Returns the level of the first record holding @p text, or Off with none.
            [[nodiscard]] SushiLog::LogLevel level_of(const std::string& text) const
            {
                for (const SushiLog::CapturedRecord& record : records())
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
                for (const SushiLog::CapturedRecord& record : records())
                {
                    if (record.message.find(text) != std::string::npos)
                        found = record.message;
                }
                return found;
            }

        private:
            /// @brief Keeps the level @p category has, then sets it to @p level.
            void set_level(const SushiLog::LogCategory& category, SushiLog::LogLevel level)
            {
                found_levels_.emplace_back(category, SushiLog::log_level(category));
                SushiLog::set_log_level(category, level);
            }

            /// @brief Every category SushiTrack logs under.
            static constexpr std::array<SushiLog::LogCategory, 5> K_CATEGORIES = {
                K_TRACKING_LOG_CATEGORY, K_ASSOCIATION_LOG_CATEGORY, K_FILTERING_LOG_CATEGORY,
                K_CONFIGURATION_LOG_CATEGORY, K_C_API_LOG_CATEGORY};

            /// @brief Each category this object set, with the level it had on entry, in order.
            std::vector<std::pair<SushiLog::LogCategory, SushiLog::LogLevel>> found_levels_;
            /// @brief The scoped in-memory sink, installed for this object's lifetime.
            SushiLog::LogCapture capture_;
    };
} // namespace SushiTrack
