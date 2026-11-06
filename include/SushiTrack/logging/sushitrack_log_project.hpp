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
