/**************************************************************************/
/* MOTRunner.h                                                            */
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
 * @file MOTRunner.h
 * @brief Executes MOT challenge benchmark sequences for regression.
 * @author Mustafa Garip
 */

#pragma once

#include "TestInfra.h"
#include "ITrackerAdapter.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace TestInfrastructure
{
    /**
     * @brief Performance metrics for a completed benchmark run.
     */
    struct RunResult
    {
        int    frames_processed  = 0;  ///< Count of processed frames.
        double total_tracking_us = 0.0; ///< Total execution time in microseconds.

        /**
         * @brief Calculates mean execution time per frame.
         * 
         * @return Mean time in microseconds.
         */
        double mean_us() const { return frames_processed ? total_tracking_us / frames_processed : 0.0; }
        
        /**
         * @brief Calculates processing frequency.
         * 
         * @return Frames-per-second.
         */
        double fps()     const { return total_tracking_us > 0.0
                                        ? frames_processed / (total_tracking_us * 1e-6)
                                        : 0.0; }
    };

    /**
     * @brief Orchestrates a benchmark run for a specific data sequence.
     * 
     * Loads detections, initializes the tracker adapter, executes tracking 
     * on each frame, and writes results to an output file.
     * 
     */
    inline RunResult run_mot_sequence(
        const std::string& base_data_dir,
        const std::string& scenario,
        ITrackerAdapter&   adapter,
        const std::string& out_dir = "")
    {
        namespace fs = std::filesystem;

        fs::path base          = fs::path(base_data_dir) / scenario;
        fs::path det_path      = base / "det" / "det.txt";
        fs::path seq_info_path = base / "seqinfo.ini";

        if (!fs::exists(det_path))
            throw std::runtime_error("det.txt not found: " + det_path.string());

        int   seq_fps = 30; 
        float dt      = 1.0f / 30.0f;
        if (fs::exists(seq_info_path))
        {
            std::ifstream ini(seq_info_path);
            std::string   line;
            while (std::getline(ini, line))
            {
                if (line.rfind("frameRate=", 0) == 0)
                {
                    float fps = std::stof(line.substr(10));
                    if (fps > 0.0f)
                    {
                        seq_fps = static_cast<int>(fps);
                        dt      = 1.0f / fps;
                    }
                    break;
                }
            }
        }

        adapter.setFrameRate(seq_fps);

        auto sequence = load_mot_detections(det_path.string());
        if (sequence.empty())
            return RunResult{};

        fs::path pred_dir = out_dir.empty() ? base / "pred" : fs::path(out_dir);
        fs::path tracker_pred_dir = pred_dir / adapter.name();
        fs::create_directories(tracker_pred_dir);

        fs::path pred_path = tracker_pred_dir / (scenario + ".txt");

        std::ofstream out(pred_path);
        if (!out.is_open())
            throw std::runtime_error("Cannot open output: " + pred_path.string());

        RunResult     result;
        PerformanceTimer frame_timer;

        for (const auto& [frame_id, frame_dets] : sequence)
        {
            frame_timer.reset();
            auto tracks = adapter.update(frame_dets, dt);
            result.total_tracking_us += frame_timer.elapsed_us();

            for (const auto& t : tracks)
            {
                out << frame_id   << ","
                    << t.track_id << ","
                    << t.x        << ","
                    << t.y        << ","
                    << t.w        << ","
                    << t.h        << ","
                    << t.score    << ",-1,-1,-1\n";
            }

            ++result.frames_processed;
        }

        std::cout << "[ PERF " << adapter.name() << " ] "
                  << scenario << ": "
                  << result.mean_us() << " us/frame  ("
                  << result.frames_processed << " frames, "
                  << static_cast<int>(result.fps()) << " fps)\n";

        return result;
    }

} // namespace TestInfrastructure
