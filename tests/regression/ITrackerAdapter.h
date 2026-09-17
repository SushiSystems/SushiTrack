/**************************************************************************/
/* ITrackerAdapter.h                                                      */
/**************************************************************************/
/*                          This file is part of:                         */
/*                               SushiTrack                               */
/*              https://github.com/SushiSystems/SushiTrack                */
/*                          https://sushisystems.io                       */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                          */
/**************************************************************************/

/**
 * @file ITrackerAdapter.h
 * @brief Defines the abstract interface for regression tracker adapters.
 * @author Mustafa Garip
 */

#pragma once

#include "TestInfra.h"
#include <string>
#include <vector>

namespace TestInfrastructure
{
    /**
     * @brief Interface for tracker implementation adapters.
     * 
     * Provides an abstraction layer for benchmarking different tracking 
     * algorithms within the test infrastructure.
     */
    class ITrackerAdapter
    {
        public:
            /**
             * @brief Destroys the ITrackerAdapter instance.
             */
            virtual ~ITrackerAdapter() = default;

            /**
             * @brief Retrieves the identifier for the tracker implementation.
             * 
             * @return Identifier string.
             */
            virtual std::string name() const = 0;

            /**
             * @brief Processes detections and returns tracking results.
             * 
             * @param dets Collection of input detections.
             * @param dt Time interval for state propagation.
             * @return Collection of track results.
             */
            virtual std::vector<TrackResult> update(
                const std::vector<MOTDetection>& dets, float dt = 1.0f) = 0;

            /**
             * @brief Configures the processing frequency for the tracker.
             * 
             * Calibrates timing-dependent components such as motion models 
             * and persistence buffers for data sequences.
             * 
             * @param fps Target frames-per-second.
             */
            virtual void setFrameRate(int fps) {}
    };

} // namespace TestInfrastructure
