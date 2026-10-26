/**************************************************************************/
/* sushitrack_log_project.hpp                                             */
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
 * @file sushitrack_log_project.hpp
 * @brief Declares K_SUSHITRACK_LOG_PROJECT, the project every SushiTrack log category belongs to.
 * @author Mustafa Garip
 */

#pragma once

#include <SushiLog/log/log_project.hpp>

namespace SushiTrack
{
    /** @brief Names SushiTrack's records, whose levels SUSHITRACK_LOG_LEVEL sets. */
    inline constexpr SushiLog::LogProject K_SUSHITRACK_LOG_PROJECT{"sushitrack",
                                                                   "SUSHITRACK_LOG_LEVEL"};
} // namespace SushiTrack
