/**************************************************************************/
/* sushitrack_adapter.hpp                                                 */
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
 * @file sushitrack_adapter.hpp
 * @brief Declares SushiTrackAdapter, which drives SushiTrack through its C API.
 * @author Mustafa Garip
 */

#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include <SushiTrack/c_api/sushitrack_c.h>
#include <SushiTrack/logging/logger.hpp>

#include <mot_detection.hpp>
#include <track_result.hpp>
#include <tracker_adapter.hpp>
#include <tracker_bridge_log_category.hpp>

namespace TrackerBridge
{
    /**
     * @brief Drives SushiTrack through its public C API and reports its activated tracks.
     */
    class SushiTrackAdapter final : public TrackerAdapter
    {
        public:
            /**
             * @brief Builds a tracker from the defaults overlaid with @p config_path, or with the
             *        file the library resolves when the path is empty.
             */
            explicit SushiTrackAdapter(const std::string& config_path = "")
            {
                sushitrack_get_default_params(&params_);
                const sushitrack_status_t status = sushitrack_load_params_from_json(
                    config_path.empty() ? nullptr : config_path.c_str(), &params_);
                if (status != SUSHITRACK_SUCCESS)
                {
                    sushitrack_get_default_params(&params_);
                    SUSHITRACK_LOG_WARN(K_TRACKER_BRIDGE_LOG_CATEGORY,
                                        "SushiTrack parameters from {} did not load (status {}); "
                                        "using the default parameters.",
                                        config_path.empty() ? "the default search" : config_path,
                                        static_cast<int>(status));
                }
                rebuild_tracker();
            }

            SushiTrackAdapter(const SushiTrackAdapter&) = delete;
            SushiTrackAdapter& operator=(const SushiTrackAdapter&) = delete;
            SushiTrackAdapter(SushiTrackAdapter&&) = delete;
            SushiTrackAdapter& operator=(SushiTrackAdapter&&) = delete;

            /// @brief Releases the tracker.
            ~SushiTrackAdapter() override { sushitrack_destroy(handle_); }

            /// @brief Returns "sushitrack".
            [[nodiscard]] std::string name() const override { return "sushitrack"; }

            /// @brief Rebuilds the tracker for @p fps frames per second.
            void set_frame_rate(int fps) override
            {
                params_.frame_rate = fps;
                rebuild_tracker();
            }

            /// @brief Runs one frame and returns the activated tracks.
            [[nodiscard]] std::vector<TrackResult> update(const std::vector<MOTDetection>& dets,
                                                          float dt = 1.0f) override
            {
                std::vector<sushitrack_object_t> objects;
                objects.reserve(dets.size());
                for (const MOTDetection& d : dets)
                {
                    objects.push_back({d.x, d.y, d.w, d.h, d.score, d.label,
                                       d.feature.empty() ? nullptr : d.feature.data(),
                                       static_cast<int>(d.feature.size())});
                }

                int count = 0;
                const sushitrack_status_t status =
                    sushitrack_update(handle_, objects.data(), static_cast<int>(objects.size()),
                                      tracks_.data(), static_cast<int>(tracks_.size()), dt, &count);
                if (status != SUSHITRACK_SUCCESS)
                {
                    throw std::runtime_error("sushitrack_update returned status " +
                                             std::to_string(status) + " for " +
                                             std::to_string(dets.size()) + " detections");
                }

                std::vector<TrackResult> results;
                for (int i = 0; i < count; ++i)
                {
                    const sushitrack_track_t& t = tracks_[static_cast<size_t>(i)];
                    if (t.is_activated)
                    {
                        results.push_back(
                            {static_cast<int>(t.track_id), t.x, t.y, t.width, t.height, t.score});
                    }
                }
                return results;
            }

        private:
            /// @brief Replaces the tracker with one built from params_.
            void rebuild_tracker()
            {
                sushitrack_destroy(handle_);
                handle_ = sushitrack_create(&params_);
                if (handle_ == nullptr)
                    throw std::runtime_error("sushitrack_create failed");
            }

            /// @brief The tracker's parameters.
            sushitrack_params_t params_{};
            /// @brief The tracker.
            sushitrack_tracker_t handle_ = nullptr;
            /// @brief The output buffer, larger than any frame of the benchmark sequences.
            std::vector<sushitrack_track_t> tracks_ = std::vector<sushitrack_track_t>(4096);
    };
} // namespace TrackerBridge
