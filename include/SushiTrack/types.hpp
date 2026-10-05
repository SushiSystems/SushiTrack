/****************************************************************/
/* types.hpp                                                    */
/* SushiTrack - https://github.com/SushiSystems/SushiTrack      */
/* Copyright (c) 2026 Sushi Systems                             */
/* Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.    */
/* Commercial use requires a licence from Sushi Systems.        */
/****************************************************************/

/**
 * @file types.hpp
 * @brief Common status codes, log levels, and type aliases for SushiTrack.
 * @author Mustafa Garip
 */

#pragma once

#include <SushiTrack/sushitrack_c.h>

namespace SushiTrack
{
    using Status = sushitrack_status_t;
    using LogLevel = sushitrack_log_level_t;
    using LogCallback = sushitrack_log_callback_t;
} // namespace SushiTrack
