/**************************************************************************/
/* create_tracker.hpp                                                     */
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
 * @file create_tracker.hpp
 * @brief Declares create_tracker, which builds a tracker from a Kalman filter registry the
 *        caller owns.
 * @author Mustafa Garip
 */

#pragma once

#include <SushiTrack/c_api/sushitrack_c.h>
#include <SushiTrack/c_api/sushitrack_export.hpp>

namespace SushiTrack
{
    class KalmanFilterRegistry;

    /**
     * @brief Builds a tracker as sushitrack_create_ex does, with its Kalman filters from
     *        @p registry in place of KalmanFilterRegistry::get_instance().
     * @param registry The registry the tracker creates its filters from; it must outlive the
     *        tracker.
     * @return The status sushitrack_create_ex gives for the same parameters.
     */
    [[nodiscard]] SUSHITRACK_API sushitrack_status_t
    create_tracker(const sushitrack_params_t* params, const KalmanFilterRegistry& registry,
                   sushitrack_tracker_t* out_tracker);
} // namespace SushiTrack
