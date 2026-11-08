/**************************************************************************/
/* kalman_registry_builders.hpp                                           */
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
 * @file kalman_registry_builders.hpp
 * @brief Declares library_kalman_registry, a registry the test owns that holds the library's
 *        three Kalman filters.
 * @author Mustafa Garip
 */

#pragma once

#include <memory>

#include <SushiTrack/filtering/gated_state_predictor.hpp>
#include <SushiTrack/filtering/imm_filter.hpp>
#include <SushiTrack/filtering/kalman_filter.hpp>
#include <SushiTrack/filtering/kalman_filter_registry.hpp>
#include <SushiTrack/filtering/nsa_kalman_filter.hpp>

namespace TestSupport
{
    /**
     * @brief Returns a registry that maps the documented kalman_type values to their filters: 0
     *        linear, 1 NSA, 2 IMM.
     */
    [[nodiscard]] inline std::unique_ptr<SushiTrack::KalmanFilterRegistry> library_kalman_registry()
    {
        auto registry = std::make_unique<SushiTrack::KalmanFilterRegistry>();
        registry->register_creator(0,
                                   [](bool enable_xyah)
                                   {
                                       return std::make_unique<SushiTrack::KalmanFilter>(
                                           enable_xyah);
                                   });
        registry->register_creator(1,
                                   [](bool enable_xyah)
                                   {
                                       return std::make_unique<SushiTrack::NSAKalmanFilter>(
                                           enable_xyah);
                                   });
        registry->register_creator(2,
                                   [](bool enable_xyah)
                                   {
                                       return std::make_unique<SushiTrack::IMMFilter>(
                                           enable_xyah);
                                   });
        return registry;
    }
} // namespace TestSupport
