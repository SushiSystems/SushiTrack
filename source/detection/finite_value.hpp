/**************************************************************************/
/* finite_value.hpp                                                       */
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
 * @file finite_value.hpp
 * @brief Declares is_non_finite, a NaN and infinity test that -ffast-math cannot fold away.
 * @author Mustafa Garip
 */

#pragma once

#include <cstdint>
#include <cstring>

namespace SushiTrack
{
    /**
     * @brief Returns whether @p value is NaN or infinite, read from its bits so -ffast-math
     *        cannot fold the test away.
     */
    [[nodiscard]] inline bool is_non_finite(float value) noexcept
    {
        std::uint32_t bits = 0U;
        std::memcpy(&bits, &value, sizeof(bits));
        return (bits & 0x7F800000U) == 0x7F800000U;
    }
} // namespace SushiTrack
