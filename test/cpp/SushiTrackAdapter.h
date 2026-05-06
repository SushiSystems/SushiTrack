/**
 * @file      SushiTrackAdapter.h
 * @brief     Adapter implementation for the SushiTrack tracking algorithm.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#pragma once

#include "ITrackerAdapter.h"
#include "TestInfra.h"

#include "SushiTrackC.h"

#include "SushiTrack/SushiTrackTracker.h"
#include "SushiTrack/Object.h"
#include "SushiTrack/Rect.h"
#include "SushiTrack/TrackerConfig.h"

#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace TestInfrastructure
{
    /**
     * @brief Internal implementation structure for the SushiTrack handle.
     */
    struct SushiTrackHdlImpl
    {
        std::unique_ptr<SushiTrack::SushiTrackTracker> tracker; ///< Tracker instance.
        SushiTrack::TrackerConfig                     config;  ///< Configuration parameters.
    };

    /**
     * @brief Implementation of ITrackerAdapter for the SushiTrack algorithm.
     * 
     * Encapsulates the SushiTrack tracker and provides the interface required 
     * for evaluation within the benchmarking framework.
     */
    class SushiTrackAdapter final : public ITrackerAdapter
    {
        public:
            /**
             * @brief Initializes the adapter and loads configuration.
             * 
             * @param config_path Optional path to a JSON configuration file.
             */
            explicit SushiTrackAdapter(const std::string& config_path = "")
            {
                SushiTrack_GetDefaultParams(&params_);

                if (!config_path.empty() && std::filesystem::exists(config_path))
                    applyJson(config_path, params_);
                else if (!config_path.empty())
                    std::cout << "[ SushiTrackAdapter ] config not found at "
                            << config_path << " -- using defaults\n";

                rebuildTracker();
            }

            /**
             * @brief Destroys the SushiTrackAdapter instance and releases internal handles.
             */
            ~SushiTrackAdapter() override
            {
                if (hdl_) SushiTrack_Destroy(hdl_);
            }

            /**
             * @brief Updates the tracker processing frequency.
             * 
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
             * 
             * @return "sushitrack"
             */
            std::string name() const override { return "sushitrack"; }

            /**
             * @brief Processes detections using the SushiTrack algorithm.
             * 
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
                        d.label, d.score);

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
            SushiTrack_Params  params_{};
            SushiTrackHdl      hdl_  = nullptr;
            SushiTrackHdlImpl* impl_ = nullptr;

            /**
             * @brief Re-instantiates the tracker using the current parameter configuration.
             */
            void rebuildTracker()
            {
                if (hdl_) SushiTrack_Destroy(hdl_);
                hdl_ = SushiTrack_Create(&params_);
                if (!hdl_)
                    throw std::runtime_error("SushiTrack_Create failed");
                impl_ = reinterpret_cast<SushiTrackHdlImpl*>(hdl_);
            }

            /**
             * @brief Parses and applies configuration values from a JSON file.
             * 
             * @param path File system path to the JSON configuration.
             * @param p Reference to the configuration parameters structure to be updated.
             */
            void applyJson(const std::string& path, SushiTrack_Params& p)
            {
                JsonDoc doc = parse_json(path);
                auto it = doc.find("sushitrack");
                if (it == doc.end())
                {
                    std::cout << "[ SushiTrackAdapter ] no \"sushitrack\" section in "
                            << path << " -- using defaults\n";
                    return;
                }

                const JsonSection& s = it->second;
                auto gf = [&](const char* k, float d) { return json_get(s, k, d); };
                auto gi = [&](const char* k, int   d) {
                    return static_cast<int>(json_get(s, k, static_cast<float>(d)));
                };

                p.track_thresh  = gf("track_thresh",  p.track_thresh);
                p.high_thresh   = gf("high_thresh",   p.high_thresh);
                p.match_thresh  = gf("match_thresh",  p.match_thresh);

                p.frame_rate   = gi("frame_rate",   p.frame_rate);
                p.track_buffer = gi("track_buffer", p.track_buffer);

                p.time_aware         = gi("time_aware",         p.time_aware);
                p.enable_reid        = gi("enable_reid",        p.enable_reid);
                p.enable_fsm         = gi("enable_fsm",         p.enable_fsm);
                p.enable_tentative   = gi("enable_tentative",   p.enable_tentative);
                p.enable_mahalanobis = gi("enable_mahalanobis", p.enable_mahalanobis);
                p.enable_diou        = gi("enable_diou",        p.enable_diou);
                p.enable_xyah        = gi("enable_xyah",        p.enable_xyah);
                p.kalman_type        = gi("kalman_type",        p.kalman_type);
                p.iou_plus_one       = gi("iou_plus_one",       p.iou_plus_one);

                p.fsm_stationary_vel_thresh = gf("fsm_stationary_vel_thresh", p.fsm_stationary_vel_thresh);
                p.fsm_maneuver_accel_thresh = gf("fsm_maneuver_accel_thresh", p.fsm_maneuver_accel_thresh);

                p.tentative_match_thresh = gf("tentative_match_thresh", p.tentative_match_thresh);
                p.tentative_confirm_hits = gi("tentative_confirm_hits", p.tentative_confirm_hits);
                p.tentative_max_age      = gi("tentative_max_age",      p.tentative_max_age);

                p.tentative_fast_thresh       = gf("tentative_fast_thresh",       p.tentative_fast_thresh);
                p.tentative_fast_hits         = gi("tentative_fast_hits",         p.tentative_fast_hits);
                p.tentative_slow_thresh       = gf("tentative_slow_thresh",       p.tentative_slow_thresh);
                p.tentative_slow_hits_offset  = gi("tentative_slow_hits_offset",  p.tentative_slow_hits_offset);
                p.tentative_vslow_hits_offset = gi("tentative_vslow_hits_offset", p.tentative_vslow_hits_offset);

                p.gating_thresh      = gf("gating_thresh",      p.gating_thresh);
                p.max_area_ratio     = gf("max_area_ratio",     p.max_area_ratio);
                p.motion_cost_weight = gf("motion_cost_weight", p.motion_cost_weight);

                p.iou_match_thresh       = gf("iou_match_thresh",       p.iou_match_thresh);
                p.unconfirmed_iou_thresh = gf("unconfirmed_iou_thresh", p.unconfirmed_iou_thresh);
                p.duplicate_iou_thresh   = gf("duplicate_iou_thresh",   p.duplicate_iou_thresh);

                p.feature_momentum     = gf("feature_momentum",     p.feature_momentum);
                p.reid_direct_assignment_weight = gf("reid_direct_assignment_weight", p.reid_direct_assignment_weight);
                p.reid_high_thresh     = gf("reid_high_thresh",     p.reid_high_thresh);
                p.reid_high_iou_weight = gf("reid_high_iou_weight", p.reid_high_iou_weight);
                p.reid_low_thresh      = gf("reid_low_thresh",      p.reid_low_thresh);
                p.reid_low_iou_weight  = gf("reid_low_iou_weight",  p.reid_low_iou_weight);
                p.reid_recovery_thresh = gf("reid_recovery_thresh", p.reid_recovery_thresh);
                p.max_feature_history  = gi("max_feature_history",  p.max_feature_history);

                p.primary_assoc_mode   = gi("primary_assoc_mode",   p.primary_assoc_mode);
                p.fuse_score           = gi("fuse_score",           p.fuse_score);

                // Note: The following fields are experimental and not exposed in the C API:
                // - enable_ocm, ocm_weight, ocm_delta_t (experimental features)
                // - enable_oos (experimental smoothing)
                // These can be accessed via C++ API (TrackerConfig) if needed.

                std::cout << "[ SushiTrackAdapter ] config loaded from " << path << "\n";
            }
    };

} // namespace TestInfrastructure