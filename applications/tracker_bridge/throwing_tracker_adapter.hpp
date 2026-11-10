/**************************************************************************/
/* throwing_tracker_adapter.hpp                                           */
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
 * @file throwing_tracker_adapter.hpp
 * @brief Declares ThrowingTrackerAdapter, the adapter whose update always throws.
 * @author Mustafa Garip
 */

#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include <mot_detection.hpp>
#include <track_result.hpp>
#include <tracker_adapter.hpp>

namespace TrackerBridge
{
    /**
     * @brief An in-memory adapter that tracks nothing and throws from every update.
     *
     * It stands for a tracker that fails on a frame, so a caller's handling of that failure runs
     * without a real tracker that can be made to fail.
     */
    class ThrowingTrackerAdapter final : public TrackerAdapter
    {
        public:
            /// @brief The text of the exception update throws.
            static constexpr const char* K_FAILURE_TEXT = "the throwing adapter always fails";

            /// @brief Returns the adapter's name.
            [[nodiscard]] std::string name() const override
            {
                return "throwing";
            }

            /// @brief Throws std::runtime_error carrying K_FAILURE_TEXT.
            [[nodiscard]] std::vector<TrackResult> update(const std::vector<MOTDetection>&,
                                                          float = 1.0f) override
            {
                throw std::runtime_error(K_FAILURE_TEXT);
            }

            /// @brief Ignores the rate; the adapter holds no timing.
            void set_frame_rate(int) override
            {
            }
    };
} // namespace TrackerBridge
