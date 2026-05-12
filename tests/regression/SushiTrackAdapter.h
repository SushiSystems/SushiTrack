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
                nlohmann::json doc = parse_json(path);
                if (!doc.contains("sushitrack"))
                {
                    std::cout << "[ SushiTrackAdapter ] no \"sushitrack\" section in "
                            << path << " -- using defaults\n";
                    return;
                }

                auto ds = doc["sushitrack"];
                auto gf = [&](const nlohmann::json& sec, const char* k, float d) { return json_get(sec, k, d); };
                auto gi = [&](const nlohmann::json& sec, const char* k, int   d) {
                    return static_cast<int>(json_get(sec, k, static_cast<float>(d)));
                };

                if (ds.contains("basic"))
                {
                    auto basic = ds["basic"];
                    p.time_aware         = gi(basic, "time_aware",         p.time_aware);
                    p.frame_rate         = gi(basic, "frame_rate",         p.frame_rate);
                    p.track_buffer       = gi(basic, "track_buffer",       p.track_buffer);
                    p.track_thresh       = gf(basic, "track_thresh",       p.track_thresh);
                    p.high_thresh        = gf(basic, "high_thresh",        p.high_thresh);
                    p.match_thresh       = gf(basic, "match_thresh",       p.match_thresh);
                    p.enable_tentative   = gi(basic, "enable_tentative",   p.enable_tentative);
                    p.enable_fsm         = gi(basic, "enable_fsm",         p.enable_fsm);
                    p.enable_reid        = gi(basic, "enable_reid",        p.enable_reid);
                    p.enable_mahalanobis = gi(basic, "enable_mahalanobis", p.enable_mahalanobis);
                    p.enable_interaction_context = gi(basic, "enable_interaction_context", p.enable_interaction_context);
                }

                if (ds.contains("advanced"))
                {
                    auto adv = ds["advanced"];
                    
                    if (adv.contains("association")) {
                        auto sec = adv["association"];
                        p.primary_assoc_mode     = gi(sec, "primary_assoc_mode",     p.primary_assoc_mode);
                        p.fuse_score             = gi(sec, "fuse_score",             p.fuse_score);
                        p.iou_match_thresh       = gf(sec, "iou_match_thresh",       p.iou_match_thresh);
                        p.unconfirmed_iou_thresh = gf(sec, "unconfirmed_iou_thresh", p.unconfirmed_iou_thresh);
                        p.duplicate_iou_thresh   = gf(sec, "duplicate_iou_thresh",   p.duplicate_iou_thresh);
                    }
                    
                    if (adv.contains("coordinate_system")) {
                        auto sec = adv["coordinate_system"];
                        p.enable_xyah  = gi(sec, "enable_xyah",  p.enable_xyah);
                        p.enable_diou  = gi(sec, "enable_diou",  p.enable_diou);
                        p.iou_plus_one = gi(sec, "iou_plus_one", p.iou_plus_one);
                        p.kalman_type  = gi(sec, "kalman_type",  p.kalman_type);
                    }
                    
                    if (adv.contains("reid_params")) {
                        auto sec = adv["reid_params"];
                        p.feature_momentum     = gf(sec, "feature_momentum",     p.feature_momentum);
                        p.reid_high_thresh     = gf(sec, "reid_high_thresh",     p.reid_high_thresh);
                        p.reid_high_iou_weight = gf(sec, "reid_high_iou_weight", p.reid_high_iou_weight);
                        p.reid_low_thresh      = gf(sec, "reid_low_thresh",      p.reid_low_thresh);
                        p.reid_low_iou_weight  = gf(sec, "reid_low_iou_weight",  p.reid_low_iou_weight);
                        p.reid_recovery_thresh = gf(sec, "reid_recovery_thresh", p.reid_recovery_thresh);
                        p.max_feature_history  = gi(sec, "max_feature_history",  p.max_feature_history);
                        p.reid_direct_assignment_weight = gf(sec, "reid_direct_assignment_weight", p.reid_direct_assignment_weight);
                    }
                    
                    if (adv.contains("fsm_params")) {
                        auto sec = adv["fsm_params"];
                        p.fsm_stationary_vel_thresh = gf(sec, "fsm_stationary_vel_thresh", p.fsm_stationary_vel_thresh);
                        p.fsm_maneuver_accel_thresh = gf(sec, "fsm_maneuver_accel_thresh", p.fsm_maneuver_accel_thresh);
                    }
                    
                    if (adv.contains("tentative_logic")) {
                        auto sec = adv["tentative_logic"];
                        p.tentative_match_thresh    = gf(sec, "tentative_match_thresh",    p.tentative_match_thresh);
                        p.tentative_confirm_hits    = gi(sec, "tentative_confirm_hits",    p.tentative_confirm_hits);
                        p.tentative_max_age_seconds = gf(sec, "tentative_max_age_seconds", p.tentative_max_age_seconds);
                        p.tentative_fast_thresh       = gf(sec, "tentative_fast_thresh",       p.tentative_fast_thresh);
                        p.tentative_fast_hits         = gi(sec, "tentative_fast_hits",         p.tentative_fast_hits);
                        p.tentative_slow_thresh       = gf(sec, "tentative_slow_thresh",       p.tentative_slow_thresh);
                        p.tentative_slow_hits_offset  = gi(sec, "tentative_slow_hits_offset",  p.tentative_slow_hits_offset);
                        p.tentative_vslow_hits_offset = gi(sec, "tentative_vslow_hits_offset", p.tentative_vslow_hits_offset);
                    }
                    
                    if (adv.contains("mahalanobis_gating")) {
                        auto sec = adv["mahalanobis_gating"];
                        p.proximity_thresh   = gf(sec, "proximity_thresh",   p.proximity_thresh);
                        p.gating_thresh      = gf(sec, "gating_thresh",      p.gating_thresh);
                        p.motion_cost_weight = gf(sec, "motion_cost_weight", p.motion_cost_weight);
                        p.max_area_ratio     = gf(sec, "max_area_ratio",     p.max_area_ratio);
                    }

                    if (adv.contains("interaction_context")) {
                        auto sec = adv["interaction_context"];
                        p.interaction_iou_thresh                = gf(sec, "interaction_iou_thresh",                p.interaction_iou_thresh);
                        p.interaction_occluder_penalty          = gf(sec, "interaction_occluder_penalty",          p.interaction_occluder_penalty);
                        p.interaction_occluder_validity_seconds = gf(sec, "interaction_occluder_validity_seconds", p.interaction_occluder_validity_seconds);
                    }
                }

                std::cout << "[ SushiTrackAdapter ] config loaded from " << path << "\n";
            }
    };

} // namespace TestInfrastructure                                                                                                                                   