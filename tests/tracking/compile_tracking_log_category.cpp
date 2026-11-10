/**************************************************************************/
/* compile_tracking_log_category.cpp                                      */
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
 * @file compile_tracking_log_category.cpp
 * @brief Checks at compile time that the tracking category belongs to `sushitrack`, module
 *        `tracking`.
 * @author Mustafa Garip
 */

#include <SushiTrack/tracking/tracking_log_category.hpp>

static_assert(SushiTrack::K_TRACKING_LOG_CATEGORY.project().name() == "sushitrack",
              "the tracking category belongs to the sushitrack project");
static_assert(SushiTrack::K_TRACKING_LOG_CATEGORY.module() == "tracking",
              "the tracking category names the tracking module");
