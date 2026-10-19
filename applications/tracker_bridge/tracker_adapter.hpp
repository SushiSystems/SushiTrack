/**************************************************************************/
/* tracker_adapter.hpp                                                    */
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
 * @file tracker_adapter.hpp
 * @brief Declares TrackerAdapter, the one interface the benchmark and the inference drive trackers by.
 * @author Mustafa Garip
 */

#pragma once

#include <string>
#include <vector>

#include <mot_detection.hpp>
#include <track_result.hpp>

namespace TrackerBridge
{
    /**
     * @brief Runs one tracker frame by frame behind a common interface.
     */
    class TrackerAdapter
    {
        public:
            virtual ~TrackerAdapter() = default;

            /// @brief Returns the tracker's name, as its predictions folder is called.
            [[nodiscard]] virtual std::string name() const = 0;

            /**
             * @brief Runs one frame and returns the tracks to report.
             * @param dt Seconds since the previous frame.
             */
            [[nodiscard]] virtual std::vector<TrackResult>
            update(const std::vector<MOTDetection>& dets, float dt = 1.0f) = 0;

            /// @brief Adapts the timing-dependent parts to @p fps frames per second.
            virtual void set_frame_rate(int fps) = 0;
    };
} // namespace TrackerBridge
