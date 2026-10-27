/**************************************************************************/
/* sushitrack_c.cpp                                                       */
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
 * @file sushitrack_c.cpp
 * @brief Implements the C-compatible application programming interface.
 * @author Mustafa Garip
 */

#include <SushiTrack/c_api/sushitrack_c.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <utility>
#include <vector>

#include <SushiTrack/association/cosine_cost_calculator.hpp>
#include <SushiTrack/association/diou_cost_calculator.hpp>
#include <SushiTrack/association/fuse_score_decorator.hpp>
#include <SushiTrack/association/fused_cost_calculator.hpp>
#include <SushiTrack/association/gated_iou_cost_calculator.hpp>
#include <SushiTrack/association/giou_cost_calculator.hpp>
#include <SushiTrack/association/iou_cost_calculator.hpp>
#include <SushiTrack/association/lapjv_solver.hpp>
#include <SushiTrack/association/observation_momentum_decorator.hpp>
#include <SushiTrack/association/pure_reid_cost_calculator.hpp>
#include <SushiTrack/association/reid_first_cost_calculator.hpp>
#include <SushiTrack/association/siou_cost_calculator.hpp>
#include <SushiTrack/c_api/c_api_log_category.hpp>
#include <SushiTrack/detection/object.hpp>
#include <SushiTrack/filtering/gate_validator.hpp>
#include <SushiTrack/filtering/generic_kalman_factory.hpp>
#include <SushiTrack/filtering/kalman_filter.hpp>
#include <SushiTrack/filtering/kalman_filter_registry.hpp>
#include <SushiTrack/filtering/nsa_kalman_filter.hpp>
#include <SushiTrack/filtering/null_gate_validator.hpp>
#include <SushiTrack/logging/log_throttle.hpp>
#include <SushiTrack/logging/logger.hpp>
#include <SushiTrack/profiling/profiler_keys.hpp>
#include <SushiTrack/profiling/profiling.hpp>
#include <SushiTrack/tracking/track_state.hpp>
#include <SushiTrack/tracking/tracker.hpp>

/**
 * @brief Internal handle encapsulating tracker state and configuration.
 */
struct sushitrack_tracker_impl_t
{
        std::unique_ptr<SushiTrack::Tracker> tracker;
        SushiTrack::TrackerConfig config;
        std::vector<std::shared_ptr<SushiTrack::Tracklet>> last_results;
        std::vector<std::vector<float>> last_features;
};

using SushiTrack::K_C_API_LOG_CATEGORY;
namespace Keys = SushiTrack::ProfilerKeys;

namespace
{
    /**
     * @brief Sorts results by state priority and writes the first max_tracks into out_tracks.
     * @return SUCCESS, or BUFFER_OVERFLOW when results did not fit.
     */
    sushitrack_status_t populate_tracks(sushitrack_tracker_impl_t* impl,
                                        std::vector<std::shared_ptr<SushiTrack::Tracklet>>& results,
                                        sushitrack_track_t* out_tracks, int max_tracks,
                                        int* out_count)
    {
        SUSHITRACK_ZONE(Keys::K_ZONE_OUTPUT, Keys::K_GROUP_TRACK);
        std::sort(results.begin(), results.end(),
                  [](const auto& a, const auto& b)
                  {
                      auto priority = [](SushiTrack::TrackState state)
                      {
                          if (state == SushiTrack::TrackState::Tracked)
                              return 0;
                          if (state == SushiTrack::TrackState::Tentative)
                              return 1;
                          if (state == SushiTrack::TrackState::Lost)
                              return 2;
                          if (state == SushiTrack::TrackState::Removed)
                              return 3;
                          return 4;
                      };
                      return priority(a->get_track_state()) < priority(b->get_track_state());
                  });

        const bool overflow = results.size() > static_cast<std::size_t>(max_tracks);
        if (overflow)
        {
            static SushiTrack::LogThrottle overflow_throttle;
            SUSHITRACK_LOG_WARN_LIMITED(K_C_API_LOG_CATEGORY, overflow_throttle,
                                        "{} tracks exceed the output buffer of {}; writing the "
                                        "first {} by state and returning "
                                        "SUSHITRACK_BUFFER_OVERFLOW",
                                        results.size(), max_tracks, max_tracks);
        }

        int count = std::min(static_cast<int>(results.size()), max_tracks);
        impl->last_features.resize(static_cast<std::size_t>(count));

        for (int i = 0; i < count; ++i)
        {
            const auto& res = results[static_cast<std::size_t>(i)];
            const auto& rect = res->get_rect();
            out_tracks[i].x = rect.x();
            out_tracks[i].y = rect.y();
            out_tracks[i].width = rect.width();
            out_tracks[i].height = rect.height();
            out_tracks[i].score = res->get_score();
            out_tracks[i].track_id = static_cast<int64_t>(res->get_track_id());
            out_tracks[i].label = res->get_label();
            out_tracks[i].state = static_cast<int>(res->get_track_state());
            out_tracks[i].kinematic_state = static_cast<int>(res->get_kinematic_state());
            out_tracks[i].maneuver_probability =
                res->imm_active() ? res->get_maneuver_probability() : -1.0f;
            impl->last_features[i] = res->get_smooth_feat();
            out_tracks[i].feat =
                impl->last_features[i].empty() ? nullptr : impl->last_features[i].data();
            out_tracks[i].feat_dim = static_cast<int>(impl->last_features[i].size());
            out_tracks[i].is_activated = res->is_activated() ? 1 : 0;
            out_tracks[i].is_confirmed = res->is_confirmed() ? 1 : 0;
            out_tracks[i].age = static_cast<int64_t>(res->get_tracklet_length());
            out_tracks[i].lost_time = res->get_accumulated_lost_time();
            out_tracks[i].hits = res->get_hits();
            out_tracks[i].last_seen_frame = static_cast<int64_t>(res->get_frame_id());
            out_tracks[i].frames_since_last_update =
                static_cast<int>(impl->tracker->get_global_frame_id() - res->get_frame_id());
        }

        SUSHITRACK_COUNTER(Keys::K_COUNTER_TRACKS_WRITTEN, Keys::K_UNIT_TRACKS, Set, count);
        SUSHITRACK_COUNTER(Keys::K_COUNTER_TRACKS_DROPPED, Keys::K_UNIT_TRACKS, Set,
                           results.size() - static_cast<std::size_t>(count));

        *out_count = count;
        return overflow ? SUSHITRACK_BUFFER_OVERFLOW : SUSHITRACK_SUCCESS;
    }

    /**
     * @brief Returns whether a track buffer and its capacity agree.
     */
    bool is_track_buffer_valid(const sushitrack_track_t* out_tracks, int max_tracks)
    {
        return max_tracks >= 0 && (out_tracks != nullptr || max_tracks == 0);
    }

    /**
     * @brief Logs the ERROR that names the argument @p entry refused, and returns INVALID_ARGUMENT.
     */
    sushitrack_status_t refuse_track_buffer(const char* entry, int max_tracks)
    {
        SUSHITRACK_LOG_ERROR(K_C_API_LOG_CATEGORY,
                             "{} refused: out_tracks is null or max_tracks is {}; no tracks were "
                             "written.",
                             entry, max_tracks);
        return SUSHITRACK_INVALID_ARGUMENT;
    }

    /**
     * @brief Warns about each parameter value the tracker would clamp or ignore.
     */
    void warn_about_adjusted_params(const SushiTrack::TrackerConfig& config)
    {
        if (config.frame_rate < 1)
        {
            SUSHITRACK_LOG_WARN(K_C_API_LOG_CATEGORY,
                                "frame_rate {} is below 1; the tracker clamps it to 1.",
                                config.frame_rate);
        }
        if (config.track_buffer < 1)
        {
            SUSHITRACK_LOG_WARN(K_C_API_LOG_CATEGORY,
                                "track_buffer {} is below 1; the tracker clamps it to 1.",
                                config.track_buffer);
        }
        if (config.iou_type < 0 || config.iou_type > 3)
        {
            SUSHITRACK_LOG_WARN(K_C_API_LOG_CATEGORY,
                                "iou_type {} is unknown; using plain IoU.", config.iou_type);
        }
    }
} // namespace

extern "C"
{
    /**
     * @brief Populates a parameters structure with default algorithm values.
     * @param params Pointer to the structure to be initialized.
     */
    void sushitrack_get_default_params(sushitrack_params_t* params)
    {
        if (!params)
            return;

        params->enable_reid = 0;
        params->time_aware = 0;

        params->frame_rate = 25;
        params->track_buffer = 30;

        params->track_thresh = 0.2f;
        params->high_thresh = 0.5f;
        params->match_thresh = 0.7f;

        params->enable_tentative = 0;

        params->enable_mahalanobis = 0;
        params->gating_thresh = 9.4877f;
        params->max_area_ratio = 50.0f;
        params->motion_cost_weight = 0.15f;

        params->enable_xyah = 0;
        params->kalman_type = 0;
        params->iou_plus_one = 1;
        params->iou_type = 1;
        params->feature_momentum = 0.9f;
        params->reid_direct_assignment_weight = 0.2f;

        params->iou_match_thresh = 0.5f;
        params->unconfirmed_iou_thresh = 0.7f;
        params->duplicate_iou_thresh = 0.15f;

        params->tentative_match_thresh = 0.3f;
        params->tentative_confirm_hits = 3;
        params->tentative_max_age_seconds = 0.08f;
        params->tentative_max_miss_frames = 2;
        params->tentative_init_suppress_iou = 0.7f;

        params->tentative_fast_thresh = 0.85f;
        params->tentative_fast_hits = 1;
        params->tentative_slow_thresh = 0.40f;
        params->tentative_slow_hits_offset = 1;
        params->tentative_vslow_hits_offset = 2;

        params->reid_high_thresh = 0.3f;
        params->reid_high_iou_weight = 0.3f;
        params->reid_low_thresh = 0.5f;
        params->reid_low_iou_weight = 0.2f;
        params->reid_recovery_thresh = 0.4f;
        params->max_feature_history = 100;

        params->primary_assoc_mode = 1;
        params->fuse_score = 1;

        params->enable_ocm = 0;
        params->enable_oru = 0;
        params->ocm_vdc_weight = 0.1f;
        params->ocm_delta_t = 3;
        params->ocm_min_velocity = 1.0f;
        params->ocm_max_age = 1;
        params->ocm_maneuver_weight_scale = 0.15f;
    }

    /**
     * @brief Instantiates a tracker based on provided configuration.
     * @param params Pointer to configuration structure, or NULL for defaults.
     * @return Handle to initialized tracker instance, or NULL on failure.
     */
    sushitrack_tracker_t sushitrack_create(const sushitrack_params_t* params)
    {
        if (!params)
            return sushitrack_create_from_config(nullptr);

        sushitrack_tracker_t hdl = nullptr;
        sushitrack_status_t status = sushitrack_create_ex(params, &hdl);

        if (status != SUSHITRACK_SUCCESS)
            return nullptr;

        return hdl;
    }

    /**
     * @brief Extended instantiation providing status code feedback.
     * @param params Pointer to configuration structure.
     * @param out_tracker Pointer to receive initialized tracker handle.
     * @return Status code indicating success or failure reason.
     */
    sushitrack_status_t sushitrack_create_ex(const sushitrack_params_t* params,
                                             sushitrack_tracker_t* out_tracker)
    {
        if (!out_tracker)
        {
            SUSHITRACK_LOG_ERROR(K_C_API_LOG_CATEGORY,
                                 "sushitrack_create_ex refused: out_tracker is null; no tracker "
                                 "was created.");
            return SUSHITRACK_INVALID_ARGUMENT;
        }
        *out_tracker = nullptr;
        if (!params)
        {
            SUSHITRACK_LOG_ERROR(K_C_API_LOG_CATEGORY,
                                 "sushitrack_create_ex refused: params is null; no tracker was "
                                 "created.");
            return SUSHITRACK_INVALID_ARGUMENT;
        }

        try
        {
            SushiTrack::TrackerConfig config;
            config.enable_reid = (params->enable_reid != 0);
            config.time_aware = (params->time_aware != 0);

            config.frame_rate = params->frame_rate;
            config.track_buffer = params->track_buffer;

            config.track_thresh = params->track_thresh;
            config.high_thresh = params->high_thresh;
            config.match_thresh = params->match_thresh;

            config.enable_tentative = (params->enable_tentative != 0);

            config.enable_mahalanobis = (params->enable_mahalanobis != 0);
            config.gating_thresh = params->gating_thresh;
            config.max_area_ratio = params->max_area_ratio;
            config.motion_cost_weight = params->motion_cost_weight;

            config.iou_match_thresh = params->iou_match_thresh;
            config.unconfirmed_iou_thresh = params->unconfirmed_iou_thresh;
            config.tentative_match_thresh = params->tentative_match_thresh;
            config.tentative_confirm_hits = params->tentative_confirm_hits;
            config.tentative_max_age_seconds = params->tentative_max_age_seconds;
            config.tentative_max_miss_frames = params->tentative_max_miss_frames;
            config.tentative_init_suppress_iou = params->tentative_init_suppress_iou;
            config.tentative_fast_thresh = params->tentative_fast_thresh;
            config.tentative_fast_hits = params->tentative_fast_hits;
            config.tentative_slow_thresh = params->tentative_slow_thresh;
            config.tentative_slow_hits_offset = params->tentative_slow_hits_offset;
            config.tentative_vslow_hits_offset = params->tentative_vslow_hits_offset;
            config.duplicate_iou_thresh = params->duplicate_iou_thresh;
            config.enable_xyah = (params->enable_xyah != 0);
            config.kalman_type = params->kalman_type;
            config.iou_plus_one = (params->iou_plus_one != 0);
            config.iou_type = params->iou_type;
            config.feature_momentum = params->feature_momentum;
            config.reid_direct_assignment_weight = params->reid_direct_assignment_weight;

            config.reid_high_thresh = params->reid_high_thresh;
            config.reid_high_iou_weight = params->reid_high_iou_weight;
            config.reid_low_thresh = params->reid_low_thresh;
            config.reid_low_iou_weight = params->reid_low_iou_weight;
            config.reid_recovery_thresh = params->reid_recovery_thresh;
            config.max_feature_history = params->max_feature_history;

            config.primary_assoc_mode = params->primary_assoc_mode;
            config.fuse_score = (params->fuse_score != 0);

            config.enable_ocm = (params->enable_ocm != 0);
            config.enable_oru = (params->enable_oru != 0);
            config.ocm_vdc_weight = params->ocm_vdc_weight;
            config.ocm_delta_t = params->ocm_delta_t;
            config.ocm_min_velocity = params->ocm_min_velocity;
            config.ocm_max_age = params->ocm_max_age;
            config.ocm_maneuver_weight_scale = params->ocm_maneuver_weight_scale;

            warn_about_adjusted_params(config);

            // The registry logs the ERROR that names the unregistered type.
            auto gate_kf = SushiTrack::KalmanFilterRegistry::get_instance().create(
                config.kalman_type, config.enable_xyah);
            if (!gate_kf)
                return SUSHITRACK_INVALID_ARGUMENT;

            auto kf_factory = std::make_shared<SushiTrack::GenericKalmanFactory>(
                config.kalman_type, config.enable_xyah);

            std::shared_ptr<SushiTrack::GateValidator> active_gate_validator;

            if (config.enable_mahalanobis)
                active_gate_validator = std::move(gate_kf);
            else
                active_gate_validator = SushiTrack::NullGateValidator::get_instance();

            auto solver = std::make_unique<SushiTrack::LAPJVSolver>();

            std::shared_ptr<SushiTrack::CostCalculator> iou_calc;
            if (config.iou_type == 1)
            {
                iou_calc = std::make_shared<SushiTrack::DIoUCostCalculator>(config.max_area_ratio,
                                                                            config.iou_plus_one);
            }
            else if (config.iou_type == 2)
            {
                iou_calc = std::make_shared<SushiTrack::GIoUCostCalculator>(config.max_area_ratio,
                                                                            config.iou_plus_one);
            }
            else if (config.iou_type == 3)
            {
                iou_calc = std::make_shared<SushiTrack::SIoUCostCalculator>(config.max_area_ratio,
                                                                            config.iou_plus_one);
            }
            else
            {
                iou_calc = std::make_shared<SushiTrack::IoUCostCalculator>(config.max_area_ratio,
                                                                           config.iou_plus_one);
            }

            // Plain-IoU calculator for stages with IoU-tuned thresholds.
            auto iou_plain = std::make_shared<SushiTrack::IoUCostCalculator>(config.max_area_ratio,
                                                                             config.iou_plus_one);

            std::shared_ptr<SushiTrack::CostCalculator> high_conf_metric;
            std::shared_ptr<SushiTrack::CostCalculator> low_conf_metric;
            std::shared_ptr<SushiTrack::CostCalculator> tentative_metric = iou_plain;
            std::shared_ptr<SushiTrack::CostCalculator> duplicate_metric = iou_plain;
            std::shared_ptr<SushiTrack::CostCalculator> lost_recovery_metric = nullptr;

            if (config.enable_mahalanobis)
            {
                auto gated_iou = std::make_shared<SushiTrack::GatedIoUCostCalculator>(
                    config.gating_thresh, active_gate_validator, iou_calc,
                    config.motion_cost_weight);

                if (config.enable_reid)
                {
                    // Fused association path with Mahalanobis gating.
                    auto cosine_calc = std::make_shared<SushiTrack::CosineCostCalculator>();
                    high_conf_metric = std::make_shared<SushiTrack::FusedCostCalculator>(
                        config.reid_high_iou_weight, config.reid_high_thresh, config.gating_thresh,
                        active_gate_validator, gated_iou, cosine_calc,
                        config.reid_direct_assignment_weight);
                    low_conf_metric = std::make_shared<SushiTrack::FusedCostCalculator>(
                        config.reid_low_iou_weight, config.reid_low_thresh, config.gating_thresh,
                        active_gate_validator, gated_iou, cosine_calc,
                        config.reid_direct_assignment_weight);
                    lost_recovery_metric = std::make_shared<SushiTrack::PureReIDCostCalculator>(
                        config.reid_recovery_thresh, config.gating_thresh, active_gate_validator);
                }
                else
                {
                    // Mahalanobis-gated IoU path without appearance features.
                    high_conf_metric = gated_iou;
                    low_conf_metric = gated_iou;
                }
            }
            else if (config.enable_reid)
            {
                // Appearance-first path with IoU fallback.
                auto cosine_calc = std::make_shared<SushiTrack::CosineCostCalculator>();
                high_conf_metric = std::make_shared<SushiTrack::ReIDFirstCostCalculator>(
                    config.reid_high_thresh, config.reid_high_iou_weight,
                    config.reid_direct_assignment_weight, iou_calc, cosine_calc);
                low_conf_metric = std::make_shared<SushiTrack::ReIDFirstCostCalculator>(
                    config.reid_low_thresh, config.reid_low_iou_weight,
                    config.reid_direct_assignment_weight, iou_calc, cosine_calc);
                lost_recovery_metric = std::make_shared<SushiTrack::PureReIDCostCalculator>(
                    config.reid_recovery_thresh);
            }
            else
            {
                // Plain IoU or DIoU path.
                high_conf_metric = iou_calc;
                low_conf_metric = iou_calc;
            }

            if (config.fuse_score)
            {
                high_conf_metric =
                    std::make_shared<SushiTrack::FuseScoreDecorator>(high_conf_metric);
            }

            if (config.enable_ocm)
            {
                high_conf_metric = std::make_shared<SushiTrack::ObservationMomentumDecorator>(
                    high_conf_metric, config.ocm_vdc_weight, config.ocm_min_velocity,
                    config.ocm_max_age, config.ocm_maneuver_weight_scale);
            }

            auto impl = std::make_unique<sushitrack_tracker_impl_t>();
            impl->config = config;
            impl->tracker = std::make_unique<SushiTrack::Tracker>(
                config, kf_factory, std::move(solver), high_conf_metric, low_conf_metric,
                tentative_metric, duplicate_metric, lost_recovery_metric);

            SUSHITRACK_LOG_INFO(K_C_API_LOG_CATEGORY,
                                "Tracker instance created successfully.");

            *out_tracker = static_cast<sushitrack_tracker_t>(impl.release());
            return SUSHITRACK_SUCCESS;
        }
        catch (const std::bad_alloc&)
        {
            SUSHITRACK_LOG_ERROR(K_C_API_LOG_CATEGORY,
                                 "Tracker creation failed: out of memory.");
            return SUSHITRACK_OUT_OF_MEMORY;
        }
        catch (const std::exception& e)
        {
            SUSHITRACK_LOG_ERROR(K_C_API_LOG_CATEGORY,
                                 "Tracker creation failed: {}; returning "
                                 "SUSHITRACK_INITIALIZATION_FAILURE",
                                 e.what());
            return SUSHITRACK_INITIALIZATION_FAILURE;
        }
        catch (...)
        {
            SUSHITRACK_LOG_ERROR(K_C_API_LOG_CATEGORY,
                                 "Tracker creation failed: unknown exception; returning "
                                 "SUSHITRACK_INITIALIZATION_FAILURE");
            return SUSHITRACK_INITIALIZATION_FAILURE;
        }
    }

    /**
     * @brief Terminates a tracker instance and deallocates associated memory.
     * @param tracker Handle to tracker instance to destroy.
     */
    void sushitrack_destroy(sushitrack_tracker_t tracker)
    {
        if (!tracker)
            return;
        std::unique_ptr<sushitrack_tracker_impl_t> owner(
            static_cast<sushitrack_tracker_impl_t*>(tracker));

        try
        {
            SUSHITRACK_LOG_INFO(K_C_API_LOG_CATEGORY, "Destroying tracker instance.");
            owner.reset();
        }
        catch (...)
        {
            SUSHITRACK_LOG_ERROR(K_C_API_LOG_CATEGORY,
                                 "Destroy threw an unknown exception; the tracker is released.");
        }
    }

    /**
     * @brief Terminates a tracker instance and deallocates associated memory.
     * @param tracker Handle to tracker instance to destroy.
     */
    void sushitrack_destroy_tracker(sushitrack_tracker_t tracker)
    {
        sushitrack_destroy(tracker);
    }

    /**
     * @brief Applies one frame of detections and writes the tracks it leaves.
     * @return SUCCESS, BUFFER_OVERFLOW, INVALID_ARGUMENT, OUT_OF_MEMORY or RUNTIME_ERROR.
     */
    sushitrack_status_t sushitrack_update(sushitrack_tracker_t tracker,
                                          const sushitrack_object_t* objects, int n_objects,
                                          sushitrack_track_t* out_tracks, int max_tracks, float dt,
                                          int* out_count)
    {
        if (!out_count)
        {
            SUSHITRACK_LOG_ERROR(K_C_API_LOG_CATEGORY,
                                 "sushitrack_update refused: out_count is null; the frame was "
                                 "not applied.");
            return SUSHITRACK_INVALID_ARGUMENT;
        }
        *out_count = 0;
        if (!tracker)
        {
            SUSHITRACK_LOG_ERROR(K_C_API_LOG_CATEGORY,
                                 "sushitrack_update refused: tracker is null; the frame was not "
                                 "applied.");
            return SUSHITRACK_INVALID_ARGUMENT;
        }
        if (n_objects < 0 || (n_objects > 0 && !objects))
        {
            SUSHITRACK_LOG_ERROR(K_C_API_LOG_CATEGORY,
                                 "sushitrack_update refused: n_objects is {} and objects is {}; "
                                 "the frame was not applied.",
                                 n_objects, objects ? "set" : "null");
            return SUSHITRACK_INVALID_ARGUMENT;
        }
        if (!is_track_buffer_valid(out_tracks, max_tracks))
            return refuse_track_buffer("sushitrack_update", max_tracks);
        auto* impl = static_cast<sushitrack_tracker_impl_t*>(tracker);

        try
        {
            SushiTrack::ObjectVector detections;
            detections.reserve(static_cast<std::size_t>(n_objects));

            int without_feature = 0;
            for (int i = 0; i < n_objects; ++i)
            {
                const auto& obj = objects[i];

                std::vector<float> feat_vec;
                if (impl->config.enable_reid)
                {
                    if (obj.feat && obj.feat_dim > 0)
                        feat_vec.assign(obj.feat, obj.feat + obj.feat_dim);
                    else
                        ++without_feature;
                }

                SushiTrack::Rect<float> rect(obj.x, obj.y, obj.width, obj.height);
                detections.emplace_back(rect, obj.label, obj.score, std::move(feat_vec));
            }

            if (without_feature > 0)
            {
                static SushiTrack::LogThrottle feature_throttle;
                SUSHITRACK_LOG_WARN_LIMITED(K_C_API_LOG_CATEGORY, feature_throttle,
                                            "enable_reid is on but {} of {} detections carry no "
                                            "feature; matching uses their overlap alone.",
                                            without_feature, n_objects);
            }

            impl->last_results = impl->tracker->update(detections, dt);
            return populate_tracks(impl, impl->last_results, out_tracks, max_tracks, out_count);
        }
        catch (const std::bad_alloc&)
        {
            *out_count = 0;
            SUSHITRACK_LOG_ERROR(K_C_API_LOG_CATEGORY,
                                 "Update of {} detections failed: out of memory; returning "
                                 "SUSHITRACK_OUT_OF_MEMORY",
                                 n_objects);
            return SUSHITRACK_OUT_OF_MEMORY;
        }
        catch (const std::exception& e)
        {
            *out_count = 0;
            SUSHITRACK_LOG_ERROR(K_C_API_LOG_CATEGORY,
                                 "Update of {} detections failed: {}; returning "
                                 "SUSHITRACK_RUNTIME_ERROR",
                                 n_objects, e.what());
            return SUSHITRACK_RUNTIME_ERROR;
        }
        catch (...)
        {
            *out_count = 0;
            SUSHITRACK_LOG_ERROR(K_C_API_LOG_CATEGORY,
                                 "Update of {} detections failed: unknown exception; returning "
                                 "SUSHITRACK_RUNTIME_ERROR",
                                 n_objects);
            return SUSHITRACK_RUNTIME_ERROR;
        }
    }

    /**
     * @brief Writes the tracks the tracker holds, in the order sushitrack_update writes them.
     * @return SUCCESS, BUFFER_OVERFLOW, INVALID_ARGUMENT, OUT_OF_MEMORY or RUNTIME_ERROR.
     */
    sushitrack_status_t sushitrack_get_tracklets(sushitrack_tracker_t tracker,
                                                 sushitrack_track_t* out_tracks, int max_tracks,
                                                 int* out_count)
    {
        if (!out_count)
        {
            SUSHITRACK_LOG_ERROR(K_C_API_LOG_CATEGORY,
                                 "sushitrack_get_tracklets refused: out_count is null; no tracks "
                                 "were written.");
            return SUSHITRACK_INVALID_ARGUMENT;
        }
        *out_count = 0;
        if (!tracker)
        {
            SUSHITRACK_LOG_ERROR(K_C_API_LOG_CATEGORY,
                                 "sushitrack_get_tracklets refused: tracker is null; no tracks "
                                 "were written.");
            return SUSHITRACK_INVALID_ARGUMENT;
        }
        if (!is_track_buffer_valid(out_tracks, max_tracks))
            return refuse_track_buffer("sushitrack_get_tracklets", max_tracks);
        auto* impl = static_cast<sushitrack_tracker_impl_t*>(tracker);

        try
        {
            auto tracks = impl->tracker->get_tracks();
            return populate_tracks(impl, tracks, out_tracks, max_tracks, out_count);
        }
        catch (const std::bad_alloc&)
        {
            *out_count = 0;
            SUSHITRACK_LOG_ERROR(K_C_API_LOG_CATEGORY,
                                 "Reading the tracklets failed: out of memory; returning "
                                 "SUSHITRACK_OUT_OF_MEMORY");
            return SUSHITRACK_OUT_OF_MEMORY;
        }
        catch (const std::exception& e)
        {
            *out_count = 0;
            SUSHITRACK_LOG_ERROR(K_C_API_LOG_CATEGORY,
                                 "Reading the tracklets failed: {}; returning "
                                 "SUSHITRACK_RUNTIME_ERROR",
                                 e.what());
            return SUSHITRACK_RUNTIME_ERROR;
        }
        catch (...)
        {
            *out_count = 0;
            SUSHITRACK_LOG_ERROR(K_C_API_LOG_CATEGORY,
                                 "Reading the tracklets failed: unknown exception; returning "
                                 "SUSHITRACK_RUNTIME_ERROR");
            return SUSHITRACK_RUNTIME_ERROR;
        }
    }

} // extern "C"
