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
 * @file mot_sequence.hpp
 * @brief Declares the readers of one MOTChallenge sequence, from a file or from a stream.
 * @author Mustafa Garip
 */

#pragma once

#include <fstream>
#include <istream>
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
     * @brief Returns the detections of a MOTChallenge `det.txt` read from @p rows, by frame. A
     *        row with fewer than seven numbers is skipped, and the skipped rows are counted in
     *        one WARN naming @p origin.
     */
    inline MOTFrames parse_mot_detections(std::istream& rows, const std::string& origin)
    {
        MOTFrames frames;
        std::string line;
        int count = 0;
        int skipped = 0;
        while (std::getline(rows, line))
        {
            if (line.empty() || line[0] == '#')
                continue;
            ++count;
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
                                "{}: skipped {} of {} rows with fewer than seven numbers.", origin,
                                skipped, count);
        }
        return frames;
    }

    /**
     * @brief Returns the detections of a MOTChallenge `det.txt`, by frame; empty, with one WARN,
     *        when the file cannot be opened. A row with fewer than seven numbers is skipped, and
     *        the skipped rows are counted in one WARN.
     */
    inline MOTFrames read_mot_detections(const std::string& path)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            SUSHITRACK_LOG_WARN(K_MOT_ENGINE_LOG_CATEGORY,
                                "{} cannot be opened; no detections were read.", path);
            return MOTFrames();
        }
        return parse_mot_detections(file, path);
    }

    /**
     * @brief Returns the `frameRate` of a `seqinfo.ini` read from @p lines, or @p fallback when
     *        the key is missing or not positive, with one WARN naming @p origin.
     */
    inline int parse_frame_rate(std::istream& lines, const std::string& origin, int fallback)
    {
        std::string line;
        while (std::getline(lines, line))
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
                            "{} holds no positive frameRate; using {} frames per second.", origin,
                            fallback);
        return fallback;
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
        return parse_frame_rate(file, path, fallback);
    }
} // namespace MOTEngine
