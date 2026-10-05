/****************************************************************/
/* SushiTrackAdapter.h                                          */
/* SushiTrack - https://github.com/SushiSystems/SushiTrack      */
/* Copyright (c) 2026 Sushi Systems                             */
/* Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.    */
/* Commercial use requires a licence from Sushi Systems.        */
/****************************************************************/

/**
 * @file SushiTrackAdapter.h
 * @brief Defines the regression test adapter for SushiTrack.
 * @author Mustafa Garip
 */


#pragma once

#include "ITrackerAdapter.h"
#include "TestInfra.h"

#include <SushiTrack/sushitrack_c.h>
#include <SushiTrack/tracker.hpp>
#include <SushiTrack/object.hpp>
#include <SushiTrack/rect.hpp>
#include <SushiTrack/tracker_config.hpp>

#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace TestInfrastructure
{
    /**
     * @brief Internal implementation structure matching sushitrack_c.cpp.
     */
    struct sushitrack_tracker_impl_t
    {
        std::unique_ptr<SushiTrack::Tracker>              tracker;
        SushiTrack::TrackerConfig                         config;
        std::vector<std::shared_ptr<SushiTrack::Tracklet>> last_results;
        std::vector<std::vector<float>>                   last_features;
    };

    /**
     * @brief Implementation of ITrackerAdapter for the SushiTrack algorithm.
     */
    class SushiTrackAdapter final : public ITrackerAdapter
    {
        public:
            /**
             * @brief Initializes the adapter and loads configuration.
             * @param config_path Optional path to a JSON configuration file.
             */
            explicit SushiTrackAdapter(const std::string& config_path = "")
            {
                sushitrack_get_default_params(&params_);

                sushitrack_load_params_from_json(
                    config_path.empty() ? nullptr : config_path.c_str(), &params_);

                rebuildTracker();
            }

            /**
             * @brief Destroys the adapter instance and releases internal handles.
             */
            ~SushiTrackAdapter() override
            {
                if (hdl_) sushitrack_destroy(hdl_);
            }

            /**
             * @brief Updates the tracker processing frequency.
             * @param fps Target frames-per-second.
             */
            void setFrameRate(int fps) override
            {
                params_.frame_rate = fps;
                rebuildTracker();
            }

            SushiTrackAdapter(const SushiTrackAdapter&)            = delete;
            SushiTrackAdapter& operator=(const SushiTrackAdapter&) = delete;

            /**
             * @brief Retrieves the tracker name.
             * @return "sushitrack"
             */
            std::string name() const override { return "sushitrack"; }

            /**
             * @brief Processes detections using the SushiTrack algorithm.
             * @param dets Collection of input detections.
             * @param dt Time interval for prediction steps.
             * @return Collection of tracking results.
             */
            std::vector<TrackResult> update(const std::vector<MOTDetection>& dets,
                                           float dt = 1.0f) override
            {
                SushiTrack::ObjectVector objects;
                objects.reserve(dets.size());
                for (const auto& d : dets)
                    objects.emplace_back(
                        SushiTrack::Rect<float>(d.x, d.y, d.w, d.h),
                        d.label, d.score, d.feature);

                auto tracklets = impl_->tracker->update(objects, dt);

                std::vector<TrackResult> results;
                results.reserve(tracklets.size());
                for (const auto& t : tracklets)
                {
                    if (!t->isActivated())
                        continue;

                    const auto& rect = t->getRect();
                    TrackResult r;
                    r.track_id = static_cast<int>(t->getTrackId());
                    r.x        = rect.x();
                    r.y        = rect.y();
                    r.w        = rect.width();
                    r.h        = rect.height();
                    r.score    = t->getScore();
                    results.push_back(r);
                }
                return results;
            }

        private:
            sushitrack_params_t        params_{};
            sushitrack_tracker_t       hdl_  = nullptr;
            sushitrack_tracker_impl_t* impl_ = nullptr;

            /**
             * @brief Re-instantiates the tracker using current parameters.
             */
            void rebuildTracker()
            {
                if (hdl_) sushitrack_destroy(hdl_);
                hdl_ = sushitrack_create(&params_);
                if (!hdl_)
                    throw std::runtime_error("sushitrack_create failed");
                impl_ = reinterpret_cast<sushitrack_tracker_impl_t*>(hdl_);
            }
    };

} // namespace TestInfrastructure
