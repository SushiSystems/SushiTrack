/**************************************************************************/
/* mot_sequence.hpp                                                       */
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
 * @file mot_sequence.hpp
 * @brief Declares read_mot_detections and read_frame_rate, which read one MOTChallenge sequence.
 * @author Mustafa Garip
 */

#pragma once

#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <SushiTrack/logging/logger.hpp>

#include <mot_detection.hpp>
#include <mot_engine_log_category.hpp>

namespace MOTEngine
{
    /// @brief The detections of a sequence, by frame number.
    using MOTFrames = std::map<int, std::vector<TrackerBridge::MOTDetection>>;

    /**
     * @brief Returns the detections of a MOTChallenge `det.txt`, by frame; empty, with one WARN,
     *        when the file cannot be opened. A row with fewer than seven numbers is skipped, and
     *        the skipped rows are counted in one WARN.
     */
    inline MOTFrames read_mot_detections(const std::string& path)
    {
        MOTFrames frames;
        std::ifstream file(path);
        if (!file.is_open())
        {
            SUSHITRACK_LOG_WARN(K_MOT_ENGINE_LOG_CATEGORY,
                                "{} cannot be opened; no detections were read.", path);
            return frames;
        }
        std::string line;
        int rows = 0;
        int skipped = 0;
        while (std::getline(file, line))
        {
            if (line.empty() || line[0] == '#')
                continue;
            ++rows;
            std::stringstream fields(line);
            std::string token;
            std::vector<float> row;
            while (std::getline(fields, token, ','))
            {
                std::stringstream number(token);
                float value = 0.f;
                if (!(number >> value))
                    break;
                row.push_back(value);
            }
            if (row.size() < 7)
            {
                ++skipped;
                continue;
            }
            TrackerBridge::MOTDetection det;
            det.x = row[2];
            det.y = row[3];
            det.w = row[4];
            det.h = row[5];
            det.score = row[6];
            det.label = row.size() > 7 ? static_cast<int>(row[7]) : 1;
            frames[static_cast<int>(row[0])].push_back(det);
        }
        if (skipped > 0)
        {
            SUSHITRACK_LOG_WARN(K_MOT_ENGINE_LOG_CATEGORY,
                                "{}: skipped {} of {} rows with fewer than seven numbers.", path,
                                skipped, rows);
        }
        return frames;
    }

    /**
     * @brief Returns the `frameRate` of a `seqinfo.ini`, or @p fallback when the file or the key
     *        is missing or not positive, with one WARN naming the reason.
     */
    inline int read_frame_rate(const std::string& path, int fallback)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            SUSHITRACK_LOG_WARN(K_MOT_ENGINE_LOG_CATEGORY,
                                "{} cannot be opened; using {} frames per second.", path,
                                fallback);
            return fallback;
        }
        std::string line;
        while (std::getline(file, line))
        {
            if (line.rfind("frameRate=", 0) != 0)
                continue;
            std::stringstream number(line.substr(10));
            float fps = 0.f;
            if (number >> fps && fps > 0.f)
                return static_cast<int>(fps);
            break;
        }
        SUSHITRACK_LOG_WARN(K_MOT_ENGINE_LOG_CATEGORY,
                            "{} holds no positive frameRate; using {} frames per second.", path,
                            fallback);
        return fallback;
    }
} // namespace MOTEngine
