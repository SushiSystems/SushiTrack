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
