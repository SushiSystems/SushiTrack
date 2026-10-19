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
#include <stdexcept>
#include <string>

#include <mot_sequence.hpp>

namespace MOTEvaluation
{
    RunResult run_mot_sequence(const std::filesystem::path& data_root, const std::string& sequence,
                               TrackerBridge::TrackerAdapter& adapter,
                               const std::filesystem::path& predictions)
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
            return {};

        const std::filesystem::path folder =
            (predictions.empty() ? base / "pred" : predictions) / adapter.name();
        std::filesystem::create_directories(folder);
        const std::filesystem::path output = folder / (sequence + ".txt");
        std::ofstream out(output);
        if (!out.is_open())
            throw std::runtime_error("cannot write " + output.string());

        RunResult result;
        for (const auto& [frame, dets] : frames)
        {
            const auto start = std::chrono::steady_clock::now();
            const std::vector<TrackerBridge::TrackResult> tracks = adapter.update(dets, dt);
            result.total_tracking_us +=
                std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start)
                    .count();
            for (const TrackerBridge::TrackResult& t : tracks)
            {
                out << frame << "," << t.track_id << "," << t.x << "," << t.y << "," << t.w << ","
                    << t.h << "," << t.score << ",-1,-1,-1\n";
            }
            ++result.frames_processed;
        }
        return result;
    }
} // namespace MOTEvaluation
