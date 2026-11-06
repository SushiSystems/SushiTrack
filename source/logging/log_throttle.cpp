/**************************************************************************/
/* log_throttle.cpp                                                       */
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
 * @file log_throttle.cpp
 * @brief Implements the generation counter that resets every LogThrottle at once.
 * @author Mustafa Garip
 */

#include <SushiTrack/logging/log_throttle.hpp>

namespace SushiTrack
{
    namespace
    {
        /// @brief The generation reset_log_throttles() last started.
        std::atomic<std::uint64_t> generation_counter{0U};
    } // namespace

    std::uint64_t log_throttle_generation() noexcept
    {
        return generation_counter.load(std::memory_order_relaxed);
    }

    void reset_log_throttles() noexcept
    {
        generation_counter.fetch_add(1U, std::memory_order_relaxed);
    }
} // namespace SushiTrack
