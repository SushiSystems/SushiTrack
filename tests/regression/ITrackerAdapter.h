/****************************************************************/
/* ITrackerAdapter.h                                            */
/* SushiTrack - https://github.com/SushiSystems/SushiTrack      */
/* Copyright (c) 2026 Sushi Systems                             */
/* Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.    */
/* Commercial use requires a licence from Sushi Systems.        */
/****************************************************************/

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
