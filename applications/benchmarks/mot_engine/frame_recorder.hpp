/**************************************************************************/
/* frame_recorder.hpp                                                     */
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
 * @file frame_recorder.hpp
 * @brief Declares FrameRecorder, which writes the frame timeline of one run as a record.
 * @author Mustafa Garip
 * @see docs/design/SUSHITRACK_PROFILING/README.md
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

namespace MOTEngine
{
    /**
     * @brief Says where a run's record went and what it lost.
     */
    struct FrameRecording
    {
            std::filesystem::path path; /**< The record file, or empty when none was written. */
            std::string error;          /**< Why no record was written, or empty. */
            std::uint64_t dropped = 0U; /**< Events the capture lost: full rings and late events. */
    };

    /**
     * @brief Starts a SushiProfiler session for one tracker and one sequence, marks one period
     *        per frame, and writes the record when the run ends.
     *
     * One session runs per process, so a recorder is made after the previous one finished. A
     * recorder with an empty folder, or in a build without the profiler, does nothing.
     */
    class FrameRecorder final
    {
        public:
            /**
             * @brief Starts recording the frames of @p sequence run by @p tracker.
             * @param folder Where `<tracker>_<sequence>.sprec.json` is written; empty records
             *        nothing.
             * @param frames How many frames the run will mark.
             */
            FrameRecorder(const std::filesystem::path& folder, const std::string& tracker,
                          const std::string& sequence, std::size_t frames);

            FrameRecorder(const FrameRecorder&) = delete;
            FrameRecorder& operator=(const FrameRecorder&) = delete;
            FrameRecorder(FrameRecorder&&) = delete;
            FrameRecorder& operator=(FrameRecorder&&) = delete;

            /// @brief Stops a session that finish() did not end, dropping its record.
            ~FrameRecorder();

            /// @brief Opens the next frame's period; call it right before the tracker's update.
            void begin_frame();

            /// @brief Closes the last period, stops the session and writes the record file.
            [[nodiscard]] FrameRecording finish();

        private:
            /// @brief Holds the SushiProfiler objects, so this header names none of them.
            struct Impl;

            /// @brief The session, its registry and the record path; null when nothing records.
            std::unique_ptr<Impl> impl_;
    };
} // namespace MOTEngine
