/**************************************************************************/
/* track_result.hpp                                                       */
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
 * @file track_result.hpp
 * @brief Declares TrackResult, the track every tracker adapter hands back.
 * @author Mustafa Garip
 */

#pragma once

namespace TrackerBridge
{
    /**
     * @brief Holds one confirmed track of a frame: its identity, box and score.
     */
    struct TrackResult
    {
            int track_id = 0;  /**< Track identity. */
            float x = 0.f;     /**< Left edge. */
            float y = 0.f;     /**< Top edge. */
            float w = 0.f;     /**< Width. */
            float h = 0.f;     /**< Height. */
            float score = 0.f; /**< Track confidence. */
    };
} // namespace TrackerBridge
