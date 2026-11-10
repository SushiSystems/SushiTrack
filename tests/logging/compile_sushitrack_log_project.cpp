/**************************************************************************/
/* compile_sushitrack_log_project.cpp                                     */
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
 * @file compile_sushitrack_log_project.cpp
 * @brief Checks at compile time the project name and the level variable of SushiTrack's logs.
 * @author Mustafa Garip
 */

#include <SushiTrack/logging/sushitrack_log_project.hpp>

static_assert(SushiTrack::K_SUSHITRACK_LOG_PROJECT.name() == "sushitrack",
              "the project of SushiTrack's records is sushitrack");
static_assert(SushiTrack::K_SUSHITRACK_LOG_PROJECT.level_variable() == "SUSHITRACK_LOG_LEVEL",
              "SUSHITRACK_LOG_LEVEL sets the levels of the sushitrack project");
