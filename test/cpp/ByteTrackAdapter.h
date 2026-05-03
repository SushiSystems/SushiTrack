/**
 * @file      ByteTrackAdapter.h
 * @brief     Adapter implementation for the ByteTrack tracking algorithm.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
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
                        const JsonSection& s = it->second;
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

            void rebuildTracker()
            {
                tracker_ = std::make_unique<byte_track::BYTETracker>(
                    frame_rate_, track_buffer_, track_thresh_, high_thresh_, match_thresh_);
            }
    };

} // namespace TestInfrastructure
