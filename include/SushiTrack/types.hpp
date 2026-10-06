/**************************************************************************/
/* types.hpp                                                              */
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
