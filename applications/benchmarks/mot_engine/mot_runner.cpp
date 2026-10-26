/**************************************************************************/
/* mot_runner.cpp                                                         */
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
 * @file mot_runner.cpp
 * @brief Defines run_mot_sequence.
 * @author Mustafa Garip
 */

#include <mot_runner.hpp>

#include <chrono>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <SushiTrack/logging/logger.hpp>

#include <frame_recorder.hpp>
#include <mot_engine_log_category.hpp>
#include <mot_sequence.hpp>

namespace MOTEngine
{
    std::string perf_line(const std::string& tracker, const std::string& sequence,
                          const RunResult& result)
    {
        if (result.frames_processed == 0)
            return {};
        std::ostringstream line;
        line << "[ PERF " << tracker << " ] " << sequence << ": " << result.mean_us()
             << " us/frame  (" << result.frames_processed << " frames)";
        return line.str();
    }

    RunResult run_mot_sequence(const std::filesystem::path& data_root, const std::string& sequence,
                               TrackerBridge::TrackerAdapter& adapter,
                               const std::filesystem::path& predictions,
                               const std::filesystem::path& profiles)
    {
        const std::filesystem::path base = data_root / sequence;
        const std::filesystem::path detections = base / "det" / "det.txt";
        if (!std::filesystem::exists(detections))
            throw std::runtime_error("det.txt not found: " + detections.string());

        const int fps = read_frame_rate((base / "seqinfo.ini").string(), 30);
        const float dt = 1.0f / static_cast<float>(fps);
        adapter.set_frame_rate(fps);

        const MOTFrames frames = read_mot_detections(detections.string());
        if (frames.empty())
        {
            SUSHITRACK_LOG_WARN(K_MOT_ENGINE_LOG_CATEGORY,
                                "{}: {} holds no detections; the sequence is skipped.", sequence,
                                detections.string());
            return {};
        }

        const std::filesystem::path folder =
            (predictions.empty() ? base / "pred" : predictions) / adapter.name();
        std::filesystem::create_directories(folder);
        const std::filesystem::path output = folder / (sequence + ".txt");
        std::ofstream out(output);
        if (!out.is_open())
            throw std::runtime_error("cannot write " + output.string());

        RunResult result;
        FrameRecorder recorder(profiles, adapter.name(), sequence, frames.size());
        std::vector<std::pair<int, std::vector<TrackerBridge::TrackResult>>> reported;
        reported.reserve(frames.size());
        for (const auto& [frame, dets] : frames)
        {
            recorder.begin_frame();
            const auto start = std::chrono::steady_clock::now();
            std::vector<TrackerBridge::TrackResult> tracks = adapter.update(dets, dt);
            result.total_tracking_us +=
                std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start)
                    .count();
            reported.emplace_back(frame, std::move(tracks));
            ++result.frames_processed;
        }

        const FrameRecording recording = recorder.finish();
        result.profile = recording.path;
        result.profile_dropped = recording.dropped;

        for (const auto& [frame, tracks] : reported)
        {
            for (const TrackerBridge::TrackResult& t : tracks)
            {
                out << frame << "," << t.track_id << "," << t.x << "," << t.y << "," << t.w << ","
                    << t.h << "," << t.score << ",-1,-1,-1\n";
            }
        }
        return result;
    }
} // namespace MOTEngine
