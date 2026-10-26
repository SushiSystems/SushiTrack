/**************************************************************************/
/* log_throttle.hpp                                                       */
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
 * @file log_throttle.hpp
 * @brief Declares LogThrottle, which lets a recurring event write one record per thousand.
 * @author Mustafa Garip
 */

#pragma once

#include <atomic>
#include <cstdint>

#include <SushiTrack/c_api/sushitrack_export.hpp>

namespace SushiTrack
{
    /**
     * @brief Returns the generation that reset_log_throttles() last started.
     *
     * Every LogThrottle compares its own generation with this one, so a reset reaches the
     * throttles of every translation unit and of both the shared and the static library.
     */
    [[nodiscard]] SUSHITRACK_API std::uint64_t log_throttle_generation() noexcept;

    /**
     * @brief Makes every LogThrottle admit its next event as if it had never been called.
     *
     * Tests call it so a record's presence does not depend on which test ran first.
     */
    SUSHITRACK_API void reset_log_throttles() noexcept;

    /**
     * @brief Counts the events of one call site and admits the first and every `every`-th.
     *
     * Holds the place of a SushiLog rate limit until SushiLog has one; a call site keeps one
     * `static LogThrottle` and logs only when admit() returns true.
     */
    class LogThrottle final
    {
        public:
            /// @brief Events between two admitted ones unless a throttle is given its own count.
            static constexpr std::uint64_t K_DEFAULT_EVERY = 1000U;

            /// @brief Makes a throttle that admits one event in @p every.
            explicit constexpr LogThrottle(std::uint64_t every = K_DEFAULT_EVERY) noexcept
                : every_(every == 0U ? 1U : every)
            {

            }

            /**
             * @brief Counts one event and returns whether it writes a record.
             * @param suppressed Receives how many events since the last record stayed silent.
             */
            [[nodiscard]] bool admit(std::uint64_t& suppressed) noexcept
            {
                const std::uint64_t generation = log_throttle_generation();
                if (generation_.load(std::memory_order_relaxed) != generation)
                {
                    generation_.store(generation, std::memory_order_relaxed);
                    seen_.store(0U, std::memory_order_relaxed);
                }
                const std::uint64_t before = seen_.fetch_add(1U, std::memory_order_relaxed);
                if (before % every_ != 0U)
                    return false;
                suppressed = before == 0U ? 0U : every_ - 1U;
                return true;
            }

        private:
            /// @brief Events between two admitted ones.
            std::uint64_t every_;
            /// @brief Events counted since the generation began.
            std::atomic<std::uint64_t> seen_{0U};
            /// @brief The generation seen_ belongs to.
            std::atomic<std::uint64_t> generation_{0U};
    };
} // namespace SushiTrack
