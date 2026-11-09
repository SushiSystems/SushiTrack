/**************************************************************************/
/* mot_runner.hpp                                                         */
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
 * @file mot_runner.hpp
 * @brief Declares run_mot_sequence, which runs one tracker over one sequence, and its RunResult.
 * @author Mustafa Garip
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <ostream>
#include <string>
#include <vector>

#include <track_result.hpp>
#include <tracker_adapter.hpp>

namespace MOTEngine
{
    /**
     * @brief Holds how many frames a run processed and how long the tracker took on them.
     */
    struct RunResult
    {
            int frames_processed = 0;       /**< Frames handed to the tracker. */
            double total_tracking_us = 0.0; /**< Microseconds spent in the tracker's update. */
            std::filesystem::path profile;  /**< The profile record written, or empty. */
            std::uint64_t profile_dropped = 0U; /**< Events the profile lost. */

            /// @brief Returns the microseconds per frame, or 0 with no frame.
            [[nodiscard]] double mean_us() const
            {
                return frames_processed ? total_tracking_us / frames_processed : 0.0;
            }
    };

    /**
     * @brief Runs @p adapter over `<data_root>/<sequence>/det/det.txt` and writes its tracks to
     *        `<predictions>/<adapter name>/<sequence>.txt` in MOTChallenge form.
     * @param predictions The predictions folder; `<data_root>/<sequence>/pred` when empty.
     * @param profiles Where the profile record goes, one period per frame; none when empty.
     * @return The run's frames and time; zero frames, after a WARN, for an empty sequence.
     * @throws std::runtime_error The detections are missing or the predictions cannot be written.
     */
    [[nodiscard]] RunResult run_mot_sequence(const std::filesystem::path& data_root,
                                             const std::string& sequence,
                                             TrackerBridge::TrackerAdapter& adapter,
                                             const std::filesystem::path& predictions = {},
                                             const std::filesystem::path& profiles = {});

    /**
     * @brief Writes the @p tracks of @p frame to @p out, one MOTChallenge row each:
     *        `frame,id,x,y,w,h,score,-1,-1,-1`.
     */
    void write_mot_tracks(std::ostream& out, int frame,
                          const std::vector<TrackerBridge::TrackResult>& tracks);

    /**
     * @brief Returns the `[ PERF <tracker> ]` line of a run, or an empty string for a run that
     *        processed no frame, whose mean would be a zero that reads as a measurement.
     */
    [[nodiscard]] std::string perf_line(const std::string& tracker, const std::string& sequence,
                                        const RunResult& result);
} // namespace MOTEngine
