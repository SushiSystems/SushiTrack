/**************************************************************************/
/* memory_profile_sink.hpp                                                */
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
 * @file memory_profile_sink.hpp
 * @brief Declares MemoryProfileSink, the in-memory ProfileSink that groups events by frame.
 * @author Mustafa Garip
 * @see docs/design/SUSHITRACK_PROFILING/README.md
 */

#pragma once

#include <algorithm>
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <SushiTrack/profiling/profile_sink.hpp>

namespace SushiTrack
{
    /**
     * @brief Keeps every zone, counter sample and instant it receives, grouped by the frame that
     *        was open when it arrived.
     *
     * A frame opens with begin_frame(); what arrives before the first call goes to a first frame.
     * Counters fold per frame the way SushiProfiler folds them per period.
     */
    class MemoryProfileSink final : public ProfileSink
    {
        public:
            /// @brief Opens the next frame; the events that follow belong to it.
            void begin_frame()
            {
                frames_.emplace_back();
            }

            void zone(const char* name, const char*) override
            {
                ++current().zones[name];
            }

            void counter(const char* name, ProfileCounterOp op, double value) override
            {
                Counter& counter = current().counters[name];
                if (counter.samples.empty())
                    counter.op = op;
                counter.samples.push_back(value);
            }

            void instant(const char* name) override
            {
                ++current().instants[name];
            }

            /// @brief Returns how many frames hold events or were opened.
            [[nodiscard]] std::size_t frame_count() const noexcept
            {
                return frames_.size();
            }

            /// @brief Returns how many frames entered a zone named @p name at least once.
            [[nodiscard]] std::size_t frames_holding_zone(const std::string& name) const
            {
                return static_cast<std::size_t>(std::count_if(
                    frames_.begin(), frames_.end(),
                    [&](const Frame& frame) { return frame.zones.count(name) == 1U; }));
            }

            /// @brief Returns how many instants named @p name frame @p frame holds.
            [[nodiscard]] std::size_t instant_count(std::size_t frame,
                                                    const std::string& name) const
            {
                const auto found = frames_.at(frame).instants.find(name);
                return found == frames_.at(frame).instants.end() ? 0U : found->second;
            }

            /**
             * @brief Returns what counter @p name folded to in frame @p frame, or nothing when it
             *        took no sample there.
             */
            [[nodiscard]] std::optional<double> counter_value(std::size_t frame,
                                                              const std::string& name) const
            {
                const auto found = frames_.at(frame).counters.find(name);
                if (found == frames_.at(frame).counters.end())
                    return std::nullopt;
                return fold(found->second);
            }

        private:
            /// @brief The samples of one counter in one frame and how they fold.
            struct Counter
            {
                    ProfileCounterOp op = ProfileCounterOp::Set;
                    std::vector<double> samples;
            };

            /// @brief What one frame received.
            struct Frame
            {
                    std::map<std::string, std::size_t> zones;
                    std::map<std::string, Counter> counters;
                    std::map<std::string, std::size_t> instants;
            };

            /// @brief Returns the open frame, opening the first one when none is.
            Frame& current()
            {
                if (frames_.empty())
                    frames_.emplace_back();
                return frames_.back();
            }

            /// @brief Folds the samples of @p counter by its operation.
            static double fold(const Counter& counter)
            {
                const std::vector<double>& samples = counter.samples;
                switch (counter.op)
                {
                    case ProfileCounterOp::Add:
                    {
                        double sum = 0.0;
                        for (const double sample : samples)
                            sum += sample;
                        return sum;
                    }
                    case ProfileCounterOp::Max:
                        return *std::max_element(samples.begin(), samples.end());
                    case ProfileCounterOp::Min:
                        return *std::min_element(samples.begin(), samples.end());
                    case ProfileCounterOp::Set:
                        break;
                }
                return samples.back();
            }

            /// @brief The frames, oldest first.
            std::vector<Frame> frames_;
    };
} // namespace SushiTrack
