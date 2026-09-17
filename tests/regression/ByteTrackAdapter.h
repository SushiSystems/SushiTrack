/**************************************************************************/
/* ByteTrackAdapter.h                                                     */
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
 * @file ByteTrackAdapter.h
 * @brief Defines the regression test adapter for ByteTrack.
 * @author Mustafa Garip
 */

#pragma once

#include "ITrackerAdapter.h"
#include "TestInfra.h"

#include "ByteTrack/BYTETracker.h"
#include "ByteTrack/Object.h"
#include "ByteTrack/Rect.h"

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace TestInfrastructure
{
    /**
     * @brief Implementation of ITrackerAdapter for the ByteTrack algorithm.
     * 
     * Interfaces with the ByteTrack library to provide a standardized update 
     * mechanism for benchmarking.
     */
    class ByteTrackAdapter final : public ITrackerAdapter
    {
        public:
            /**
             * @brief Initializes the adapter and loads configuration.
             * 
             * @param config_path Optional path to a JSON configuration file.
             */
            explicit ByteTrackAdapter(const std::string& config_path = "")
                : frame_rate_(30), track_buffer_(30),
                  track_thresh_(0.5f), high_thresh_(0.6f), match_thresh_(0.8f)
            {
                if (!config_path.empty() && std::filesystem::exists(config_path))
                {
                    JsonDoc doc = parse_json(config_path);
                    auto it = doc.find("bytetrack");
                    if (it != doc.end())
                    {
                        const JsonSection& s = it.value();
                        frame_rate_   = static_cast<int>(json_get(s, "frame_rate",   static_cast<float>(frame_rate_)));
                        track_buffer_ = static_cast<int>(json_get(s, "track_buffer", static_cast<float>(track_buffer_)));
                        track_thresh_ = json_get(s, "track_thresh", track_thresh_);
                        high_thresh_  = json_get(s, "high_thresh",  high_thresh_);
                        match_thresh_ = json_get(s, "match_thresh", match_thresh_);
                        std::cout << "[ ByteTrackAdapter ] config loaded from " << config_path << "\n";
                    }
                }
                else if (!config_path.empty())
                {
                    std::cout << "[ ByteTrackAdapter ] config not found at " << config_path
                            << " -- using defaults\n";
                }

                rebuildTracker();
            }

            /**
             * @brief Retrieves the tracker name.
             * 
             * @return "bytetrack"
             */
            std::string name() const override { return "bytetrack"; }

            /**
             * @brief Updates the tracker processing frequency.
             * 
             * @param fps Target frames-per-second.
             */
            void setFrameRate(int fps) override
            {
                frame_rate_ = fps;
                rebuildTracker();
            }

            /**
             * @brief Processes detections using the ByteTrack algorithm.
             * 
             * @param dets Collection of input detections.
             * @param dt Time interval (unused).
             * @return Collection of tracking results.
             */
            std::vector<TrackResult> update(const std::vector<MOTDetection>& dets, float dt = 1.0f) override
            {
                std::vector<byte_track::Object> objects;
                objects.reserve(dets.size());
                for (const auto& d : dets)
                    objects.emplace_back(
                        byte_track::Rect<float>(d.x, d.y, d.w, d.h),
                        d.label, d.score);

                auto stracks = tracker_->update(objects);

                std::vector<TrackResult> results;
                results.reserve(stracks.size());
                for (const auto& st : stracks)
                {
                    const auto& rect = st->getRect();
                    TrackResult r;
                    r.track_id = static_cast<int>(st->getTrackId());
                    r.x        = rect.x();
                    r.y        = rect.y();
                    r.w        = rect.width();
                    r.h        = rect.height();
                    r.score    = st->getScore();
                    results.push_back(r);
                }
                return results;
            }

        private:
            int   frame_rate_;
            int   track_buffer_;
            float track_thresh_;
            float high_thresh_;
            float match_thresh_;

            std::unique_ptr<byte_track::BYTETracker> tracker_;

            /**
             * @brief Re-instantiates the tracker using the current parameter configuration.
             */
            void rebuildTracker()
            {
                tracker_ = std::make_unique<byte_track::BYTETracker>(
                    frame_rate_, track_buffer_, track_thresh_, high_thresh_, match_thresh_);
            }
    };

} // namespace TestInfrastructure
