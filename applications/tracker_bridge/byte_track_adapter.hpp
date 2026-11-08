/**************************************************************************/
/* byte_track_adapter.hpp                                                 */
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
 * @file byte_track_adapter.hpp
 * @brief Declares ByteTrackAdapter, which drives the ByteTrack reference tracker.
 * @author Mustafa Garip
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <ByteTrack/BYTETracker.h>
#include <ByteTrack/Object.h>
#include <ByteTrack/Rect.h>

#include <SushiTrack/logging/logger.hpp>

#include <json_settings.hpp>
#include <tracker_bridge_log_category.hpp>
#include <mot_detection.hpp>
#include <track_result.hpp>
#include <tracker_adapter.hpp>

namespace TrackerBridge
{
    /**
     * @brief Implementation of TrackerAdapter for the ByteTrack algorithm.
     *
     * Interfaces with the ByteTrack library to provide a standardized update
     * mechanism for benchmarking.
     */
    class ByteTrackAdapter final : public TrackerAdapter
    {
        public:
            /**
             * @brief Initializes the adapter and loads configuration.
             *
             * @param config_path Optional path to a JSON configuration file.
             */
            explicit ByteTrackAdapter(const std::string& config_path = "")
            {
                if (!config_path.empty())
                    apply_settings(read_json_settings(config_path), config_path);
                rebuild_tracker();
            }

            /**
             * @brief Initializes the adapter from the "bytetrack" section of @p document, read
             *        from @p origin, which the log lines name.
             */
            ByteTrackAdapter(const nlohmann::json& document, const std::string& origin)
            {
                apply_settings(document, origin);
                rebuild_tracker();
            }

            /**
             * @brief Retrieves the tracker name.
             *
             * @return "bytetrack"
             */
            [[nodiscard]] std::string name() const override { return "bytetrack"; }

            /**
             * @brief Updates the tracker processing frequency.
             *
             * @param fps Target frames-per-second.
             */
            void set_frame_rate(int fps) override
            {
                frame_rate_ = fps;
                rebuild_tracker();
            }

            /**
             * @brief Processes detections using the ByteTrack algorithm.
             *
             * @param dets Collection of input detections.
             * @param dt Time interval (unused).
             * @return Collection of tracking results.
             */
            [[nodiscard]] std::vector<TrackResult> update(const std::vector<MOTDetection>& dets,
                                                          float dt = 1.0f) override
            {
                std::vector<byte_track::Object> objects;
                objects.reserve(dets.size());
                for (const auto& d : dets)
                    objects.emplace_back(byte_track::Rect<float>(d.x, d.y, d.w, d.h), d.label,
                                         d.score);

                auto stracks = tracker_->update(objects);

                std::vector<TrackResult> results;
                results.reserve(stracks.size());
                for (const auto& st : stracks)
                {
                    const auto& rect = st->getRect();
                    TrackResult r;
                    r.track_id = static_cast<int>(st->getTrackId());
                    r.x = rect.x();
                    r.y = rect.y();
                    r.w = rect.width();
                    r.h = rect.height();
                    r.score = st->getScore();
                    results.push_back(r);
                }
                return results;
            }

        private:
            /**
             * @brief Takes every setting the "bytetrack" section of @p document holds, logging
             *        under @p origin what it cannot use.
             */
            void apply_settings(const nlohmann::json& document, const std::string& origin)
            {
                const nlohmann::json& s = json_section(document, "bytetrack", origin);
                frame_rate_ = static_cast<int>(
                    json_number(s, "frame_rate", static_cast<float>(frame_rate_)));
                track_buffer_ = static_cast<int>(
                    json_number(s, "track_buffer", static_cast<float>(track_buffer_)));
                track_thresh_ = json_number(s, "track_thresh", track_thresh_);
                high_thresh_ = json_number(s, "high_thresh", high_thresh_);
                match_thresh_ = json_number(s, "match_thresh", match_thresh_);
                SUSHITRACK_LOG_INFO(K_TRACKER_BRIDGE_LOG_CATEGORY,
                                    "ByteTrack settings read from {}.", origin);
            }

            /// @brief Frames per second the tracker assumes.
            int frame_rate_ = 30;
            /// @brief Frames a lost track is kept.
            int track_buffer_ = 30;
            /// @brief Score that splits high from low detections.
            float track_thresh_ = 0.5f;
            /// @brief Score a detection needs to start a track.
            float high_thresh_ = 0.6f;
            /// @brief Matching threshold of the first association.
            float match_thresh_ = 0.8f;

            /// @brief The tracker built from the settings.
            std::unique_ptr<byte_track::BYTETracker> tracker_;

            /**
             * @brief Re-instantiates the tracker using the current parameter configuration.
             */
            void rebuild_tracker()
            {
                tracker_ = std::make_unique<byte_track::BYTETracker>(
                    frame_rate_, track_buffer_, track_thresh_, high_thresh_, match_thresh_);
            }
    };

} // namespace TrackerBridge
