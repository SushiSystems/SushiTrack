/**************************************************************************/
/* compile_mot_detection.cpp                                              */
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
 * @file compile_mot_detection.cpp
 * @brief Checks at compile time that MOTDetection is an aggregate that carries its box, score, label and feature.
 * @author Mustafa Garip
 */

#include <mot_detection.hpp>

#include <type_traits>
#include <vector>

static_assert(std::is_aggregate_v<TrackerBridge::MOTDetection>,
              "a detection is brace-initialised field by field");
static_assert(std::is_same_v<decltype(TrackerBridge::MOTDetection::label), int>,
              "the label is an integer class");
static_assert(std::is_same_v<decltype(TrackerBridge::MOTDetection::feature), std::vector<float>>,
              "the appearance feature is a float vector");
