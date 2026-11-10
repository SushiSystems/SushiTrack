/**************************************************************************/
/* compile_timing_policy.cpp                                              */
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
 * @file compile_timing_policy.cpp
 * @brief Checks at compile time that TimingPolicy is an abstract interface both policies fill.
 * @author Mustafa Garip
 */

#include <type_traits>

#include <SushiTrack/tracking/frame_timing_policy.hpp>
#include <SushiTrack/tracking/timing_policy.hpp>
#include <SushiTrack/tracking/wall_clock_timing_policy.hpp>

static_assert(std::is_abstract_v<SushiTrack::TimingPolicy>,
              "TimingPolicy leaves to_kf_dt and is_expired to the policies");
static_assert(std::has_virtual_destructor_v<SushiTrack::TimingPolicy>,
              "a policy is deleted through TimingPolicy");
static_assert(std::is_base_of_v<SushiTrack::TimingPolicy, SushiTrack::FrameTimingPolicy>,
              "the frame policy implements TimingPolicy");
static_assert(std::is_base_of_v<SushiTrack::TimingPolicy, SushiTrack::WallClockTimingPolicy>,
              "the wall-clock policy implements TimingPolicy");
static_assert(!std::is_abstract_v<SushiTrack::FrameTimingPolicy>,
              "the frame policy fills every pure function");
static_assert(!std::is_abstract_v<SushiTrack::WallClockTimingPolicy>,
              "the wall-clock policy fills every pure function");
