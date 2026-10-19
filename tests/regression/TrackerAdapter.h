/**************************************************************************/
/* TrackerAdapter.h                                                       */
/**************************************************************************/
/*                          This file is part of:                         */
/*                               SushiTrack                               */
/*               https://github.com/SushiSystems/SushiTrack               */
/*                         https://sushisystems.io                        */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the PolyForm Noncommercial License 1.0.0 (the           */
/* "License"); you may not use this file except in compliance with the    */
/* License. You may obtain a copy of the License at                       */
/*                                                                        */
/*     https://polyformproject.org/licenses/noncommercial/1.0.0           */
/*                                                                        */
/* Noncommercial use is free. Commercial use requires a separate licence  */
/* from Sushi Systems; see COMMERCIAL.md. The software is provided        */
/* "as is", without warranty of any kind.                                 */
/**************************************************************************/

/**
 * @file TrackerAdapter.h
 * @brief Defines the abstract interface for regression tracker adapters.
 * @author Mustafa Garip
 */

#pragma once

#include <string>
#include <vector>

#include "TestInfra.h"

namespace TestInfrastructure
{
    /**
     * @brief Interface for tracker implementation adapters.
     *
     * Provides an abstraction layer for benchmarking different tracking
     * algorithms within the test infrastructure.
     */
    class TrackerAdapter
    {
        public:
            /**
             * @brief Destroys the TrackerAdapter instance.
             */
            virtual ~TrackerAdapter() = default;

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
            virtual std::vector<TrackResult> update(const std::vector<MOTDetection>& dets,
                                                    float dt = 1.0f) = 0;

            /**
             * @brief Configures the processing frequency for the tracker.
             *
             * Calibrates timing-dependent components such as motion models
             * and persistence buffers for data sequences.
             *
             * @param fps Target frames-per-second.
             */
            virtual void set_frame_rate(int fps) {}
    };

} // namespace TestInfrastructure
