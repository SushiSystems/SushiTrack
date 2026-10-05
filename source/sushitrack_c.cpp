/****************************************************************/
/* sushitrack_c.cpp                                             */
/* SushiTrack - https://github.com/SushiSystems/SushiTrack      */
/* Copyright (c) 2026 Sushi Systems                             */
/* Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.    */
/* Commercial use requires a licence from Sushi Systems.        */
/****************************************************************/

/**
 * @file sushitrack_c.cpp
 * @brief Implements the C-compatible application programming interface.
 * @author Mustafa Garip
 */

#include <SushiTrack/sushitrack_c.h>
#include <SushiTrack/tracker.hpp>
#include <SushiTrack/kalman_filter.hpp>
#include <SushiTrack/nsa_kalman_filter.hpp>
#include <SushiTrack/assignment_solvers.hpp>
#include <SushiTrack/distance_strategies.hpp>
#include <SushiTrack/object.hpp>
#include <SushiTrack/logger.hpp>
#include <SushiTrack/kalman_filter_registry.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

/**
 * @brief Internal handle encapsulating tracker state and configuration.
 */
struct sushitrack_tracker_impl_t
{
    std::unique_ptr<SushiTrack::Tracker>              tracker;
    SushiTrack::TrackerConfig                         config;
    std::vector<std::shared_ptr<SushiTrack::Tracklet>> last_results;
    std::vector<std::vector<float>>                   last_features;
};

namespace
{
    int populate_tracks(
        sushitrack_tracker_impl_t* impl,
        std::vector<std::shared_ptr<SushiTrack::Tracklet>>& results,
        sushitrack_track_t* out_tracks,
        int max_tracks)
    {
        std::sort(results.begin(), results.end(), [](const auto& a, const auto& b) {
            auto priority = [](SushiTrack::TrackState state) {
                if (state == SushiTrack::TrackState::Tracked) return 0;
                if (state == SushiTrack::TrackState::Tentative) return 1;
                if (state == SushiTrack::TrackState::Lost) return 2;
                if (state == SushiTrack::TrackState::Removed) return 3;
                return 4;
            };
            return priority(a->getTrackState()) < priority(b->getTrackState());
        });

        if (results.size() > static_cast<std::size_t>(max_tracks))
        {
            SUSHITRACK_LOG_WARN("[SushiTrack] Output buffer capacity exceeded. Truncating lower priority tracks.");
        }

        int count = std::min(static_cast<int>(results.size()), max_tracks);
        impl->last_features.resize(static_cast<std::size_t>(count));

        for (int i = 0; i < count; ++i)
        {
            const auto& res      = results[static_cast<std::size_t>(i)];
            const auto& rect     = res->getRect();
            out_tracks[i].x          = rect.x();
            out_tracks[i].y          = rect.y();
            out_tracks[i].width      = rect.width();
            out_tracks[i].height     = rect.height();
            out_tracks[i].score      = res->getScore();
            out_tracks[i].track_id   = static_cast<int64_t>(res->getTrackId());
            out_tracks[i].label      = res->getLabel();
            out_tracks[i].state      = static_cast<int>(res->getTrackState());
            out_tracks[i].kinematic_state = static_cast<int>(res->getKinematicState());
            out_tracks[i].maneuver_probability =
                res->immActive() ? res->getManeuverProbability() : -1.0f;
            impl->last_features[i] = res->getSmoothFeat();
            out_tracks[i].feat     = impl->last_features[i].empty() ? nullptr : impl->last_features[i].data();
            out_tracks[i].feat_dim = static_cast<int>(impl->last_features[i].size());
            out_tracks[i].is_activated = res->isActivated() ? 1 : 0;
            out_tracks[i].is_confirmed = res->isConfirmed() ? 1 : 0;
            out_tracks[i].age          = static_cast<int64_t>(res->getTrackletLength());
            out_tracks[i].lost_time    = res->getAccumulatedLostTime();
            out_tracks[i].hits         = res->getHits();
            out_tracks[i].last_seen_frame = static_cast<int64_t>(res->getFrameId());
            out_tracks[i].frames_since_last_update = static_cast<int>(impl->tracker->getGlobalFrameId() - res->getFrameId());
        }

        return count;
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
        if (!params) return;

        params->enable_reid  = 0;
        params->time_aware   = 0;

        params->frame_rate   = 25;
        params->track_buffer = 30;

        params->track_thresh = 0.2f;
        params->high_thresh  = 0.5f;
        params->match_thresh = 0.7f;

        params->enable_tentative = 0;

        params->enable_mahalanobis = 0;
        params->gating_thresh      = 9.4877f;
        params->max_area_ratio     = 50.0f;
        params->motion_cost_weight = 0.15f;

        params->enable_xyah         = 0;
        params->kalman_type         = 0;
        params->iou_plus_one        = 1;
        params->iou_type            = 1;
        params->feature_momentum    = 0.9f;
        params->reid_direct_assignment_weight = 0.2f;

        params->iou_match_thresh       = 0.5f;
        params->unconfirmed_iou_thresh = 0.7f;
        params->duplicate_iou_thresh   = 0.15f;

        params->tentative_match_thresh    = 0.3f;
        params->tentative_confirm_hits    = 3;
        params->tentative_max_age_seconds = 0.08f;
        params->tentative_max_miss_frames = 2;
        params->tentative_init_suppress_iou = 0.7f;

        params->tentative_fast_thresh       = 0.85f;
        params->tentative_fast_hits         = 1;
        params->tentative_slow_thresh       = 0.40f;
        params->tentative_slow_hits_offset  = 1;
        params->tentative_vslow_hits_offset = 2;

        params->reid_high_thresh     = 0.3f;
        params->reid_high_iou_weight = 0.3f;
        params->reid_low_thresh      = 0.5f;
        params->reid_low_iou_weight  = 0.2f;
        params->reid_recovery_thresh = 0.4f;
        params->max_feature_history  = 100;

        params->primary_assoc_mode   = 1;
        params->fuse_score           = 1;

        params->enable_ocm       = 0;
        params->enable_oru       = 0;
        params->ocm_vdc_weight   = 0.1f;
        params->ocm_delta_t      = 3;
        params->ocm_min_velocity = 1.0f;
        params->ocm_max_age      = 1;
        params->ocm_maneuver_weight_scale = 0.15f;
    }

    /**
     * @brief Instantiates a tracker based on provided configuration.
     * @param params Pointer to configuration structure, or NULL for defaults.
     * @return Handle to initialized tracker instance, or NULL on failure.
     */
    sushitrack_tracker_t sushitrack_create(const sushitrack_params_t* params)
    {
        if (!params) return sushitrack_create_from_config(nullptr);

        sushitrack_tracker_t hdl    = nullptr;
        sushitrack_status_t  status = sushitrack_create_ex(params, &hdl);

        if (status != SUSHITRACK_SUCCESS) return nullptr;

        return hdl;
    }

    /**
     * @brief Extended instantiation providing status code feedback.
     * @param params Pointer to configuration structure.
     * @param out_tracker Pointer to receive initialized tracker handle.
     * @return Status code indicating success or failure reason.
     */
    sushitrack_status_t sushitrack_create_ex(
        const sushitrack_params_t* params,
        sushitrack_tracker_t*      out_tracker)
    {
        if (!params || !out_tracker) return SUSHITRACK_INVALID_ARGUMENT;

        try
        {
            SushiTrack::TrackerConfig config;
            config.enable_reid  = (params->enable_reid  != 0);
            config.time_aware   = (params->time_aware   != 0);

            config.frame_rate   = params->frame_rate;
            config.track_buffer = params->track_buffer;

            config.track_thresh = params->track_thresh;
            config.high_thresh  = params->high_thresh;
            config.match_thresh = params->match_thresh;

            config.enable_tentative = (params->enable_tentative != 0);

            config.enable_mahalanobis = (params->enable_mahalanobis != 0);
            config.gating_thresh      = params->gating_thresh;
            config.max_area_ratio     = params->max_area_ratio;
            config.motion_cost_weight = params->motion_cost_weight;

            config.iou_match_thresh       = params->iou_match_thresh;
            config.unconfirmed_iou_thresh = params->unconfirmed_iou_thresh;
            config.tentative_match_thresh    = params->tentative_match_thresh;
            config.tentative_confirm_hits    = params->tentative_confirm_hits;
            config.tentative_max_age_seconds = params->tentative_max_age_seconds;
            config.tentative_max_miss_frames = params->tentative_max_miss_frames;
            config.tentative_init_suppress_iou = params->tentative_init_suppress_iou;
            config.tentative_fast_thresh       = params->tentative_fast_thresh;
            config.tentative_fast_hits         = params->tentative_fast_hits;
            config.tentative_slow_thresh       = params->tentative_slow_thresh;
            config.tentative_slow_hits_offset  = params->tentative_slow_hits_offset;
            config.tentative_vslow_hits_offset = params->tentative_vslow_hits_offset;
            config.duplicate_iou_thresh   = params->duplicate_iou_thresh;
            config.enable_xyah            = (params->enable_xyah  != 0);
            config.kalman_type            = params->kalman_type;
            config.iou_plus_one           = (params->iou_plus_one != 0);
            config.iou_type               = params->iou_type;
            config.feature_momentum       = params->feature_momentum;
            config.reid_direct_assignment_weight = params->reid_direct_assignment_weight;

            config.reid_high_thresh     = params->reid_high_thresh;
            config.reid_high_iou_weight = params->reid_high_iou_weight;
            config.reid_low_thresh      = params->reid_low_thresh;
            config.reid_low_iou_weight  = params->reid_low_iou_weight;
            config.reid_recovery_thresh = params->reid_recovery_thresh;
            config.max_feature_history  = params->max_feature_history;

            config.primary_assoc_mode   = params->primary_assoc_mode;
            config.fuse_score           = (params->fuse_score != 0);

            config.enable_ocm       = (params->enable_ocm != 0);
            config.enable_oru       = (params->enable_oru != 0);
            config.ocm_vdc_weight   = params->ocm_vdc_weight;
            config.ocm_delta_t      = params->ocm_delta_t;
            config.ocm_min_velocity = params->ocm_min_velocity;
            config.ocm_max_age      = params->ocm_max_age;
            config.ocm_maneuver_weight_scale = params->ocm_maneuver_weight_scale;

            auto kf_factory = std::make_shared<SushiTrack::GenericKalmanFactory>(
                config.kalman_type, config.enable_xyah);

            std::shared_ptr<SushiTrack::IGateValidator> active_gate_validator;

            if (config.enable_mahalanobis)
            {
                auto gate_kf = SushiTrack::KalmanFilterRegistry::getInstance().create(
                    config.kalman_type, config.enable_xyah);
                
                if (gate_kf)
                {
                    active_gate_validator = std::move(gate_kf);
                }
                else
                {
                    active_gate_validator = SushiTrack::NullGateValidator::getInstance();
                    SUSHITRACK_LOG_WARN("[SushiTrack] Gating disabled due to invalid kalman_type.");
                }
            }
            else
            {
                active_gate_validator = SushiTrack::NullGateValidator::getInstance();
            }

            auto solver = std::make_unique<SushiTrack::LAPJVSolver>();

            std::shared_ptr<SushiTrack::ICostCalculator> iou_calc;
            if (config.iou_type == 1)
            {
                iou_calc = std::make_shared<SushiTrack::DIoUCostCalculator>(
                    config.max_area_ratio, config.iou_plus_one);
            }
            else if (config.iou_type == 2)
            {
                iou_calc = std::make_shared<SushiTrack::GIoUCostCalculator>(
                    config.max_area_ratio, config.iou_plus_one);
            }
            else if (config.iou_type == 3)
            {
                iou_calc = std::make_shared<SushiTrack::SIoUCostCalculator>(
                    config.max_area_ratio, config.iou_plus_one);
            }
            else
            {
                iou_calc = std::make_shared<SushiTrack::IoUCostCalculator>(
                    config.max_area_ratio, config.iou_plus_one);
            }

            // Plain-IoU calculator for stages with IoU-tuned thresholds.
            auto iou_plain = std::make_shared<SushiTrack::IoUCostCalculator>(
                config.max_area_ratio, config.iou_plus_one);

            std::shared_ptr<SushiTrack::ICostCalculator> high_conf_metric;
            std::shared_ptr<SushiTrack::ICostCalculator> low_conf_metric;
            std::shared_ptr<SushiTrack::ICostCalculator> tentative_metric     = iou_plain;
            std::shared_ptr<SushiTrack::ICostCalculator> duplicate_metric     = iou_plain;
            std::shared_ptr<SushiTrack::ICostCalculator> lost_recovery_metric = nullptr;

            if (config.enable_mahalanobis)
            {
                auto gated_iou = std::make_shared<SushiTrack::GatedIoUCostCalculator>(
                    config.gating_thresh,
                    active_gate_validator,
                    iou_calc,
                    config.motion_cost_weight);

                if (config.enable_reid)
                {
                    // Fused association path with Mahalanobis gating.
                    auto cosine_calc     = std::make_shared<SushiTrack::CosineCostCalculator>();
                    high_conf_metric     = std::make_shared<SushiTrack::FusedCostCalculator>(
                        config.reid_high_iou_weight,
                        config.reid_high_thresh, config.gating_thresh,
                        active_gate_validator,
                        gated_iou, cosine_calc, config.reid_direct_assignment_weight);
                    low_conf_metric      = std::make_shared<SushiTrack::FusedCostCalculator>(
                        config.reid_low_iou_weight,
                        config.reid_low_thresh, config.gating_thresh,
                        active_gate_validator,
                        gated_iou, cosine_calc, config.reid_direct_assignment_weight);
                    lost_recovery_metric = std::make_shared<SushiTrack::PureReIDCostCalculator>(
                        config.reid_recovery_thresh, config.gating_thresh, active_gate_validator);
                }
                else
                {
                    // Mahalanobis-gated IoU path without appearance features.
                    high_conf_metric = gated_iou;
                    low_conf_metric  = gated_iou;
                }
            }
            else if (config.enable_reid)
            {
                // Appearance-first path with IoU fallback.
                auto cosine_calc     = std::make_shared<SushiTrack::CosineCostCalculator>();
                high_conf_metric     = std::make_shared<SushiTrack::ReIDFirstCostCalculator>(
                    config.reid_high_thresh,
                    config.reid_high_iou_weight,
                    config.reid_direct_assignment_weight,
                    iou_calc, cosine_calc);
                low_conf_metric      = std::make_shared<SushiTrack::ReIDFirstCostCalculator>(
                    config.reid_low_thresh,
                    config.reid_low_iou_weight,
                    config.reid_direct_assignment_weight,
                    iou_calc, cosine_calc);
                lost_recovery_metric = std::make_shared<SushiTrack::PureReIDCostCalculator>(
                    config.reid_recovery_thresh);
            }
            else
            {
                // Plain IoU or DIoU path.
                high_conf_metric = iou_calc;
                low_conf_metric  = iou_calc;
            }

            if (config.fuse_score)
            {
                high_conf_metric = std::make_shared<SushiTrack::FuseScoreDecorator>(high_conf_metric);
            }

            if (config.enable_ocm)
            {
                high_conf_metric = std::make_shared<SushiTrack::ObservationMomentumDecorator>(
                    high_conf_metric,
                    config.ocm_vdc_weight,
                    config.ocm_min_velocity,
                    config.ocm_max_age,
                    config.ocm_maneuver_weight_scale);
            }

            auto impl     = new sushitrack_tracker_impl_t();
            impl->config  = config;
            impl->tracker = std::make_unique<SushiTrack::Tracker>(
                config, kf_factory, std::move(solver),
                high_conf_metric,
                low_conf_metric,
                tentative_metric,
                duplicate_metric,
                lost_recovery_metric);

            *out_tracker = static_cast<sushitrack_tracker_t>(impl);

            SUSHITRACK_LOG_INFO("[SushiTrack] Tracker instance created successfully.");

            return SUSHITRACK_SUCCESS;
        }
        catch (const std::bad_alloc&)
        {
            SUSHITRACK_LOG_ERROR("[SushiTrack] Tracker creation failed: out of memory.");
            return SUSHITRACK_OUT_OF_MEMORY;
        }
        catch (const std::exception& e)
        {
            SUSHITRACK_LOG_ERROR("[SushiTrack] Tracker creation failed: %s", e.what());
            return SUSHITRACK_RUNTIME_ERROR;
        }
        catch (...)
        {
            SUSHITRACK_LOG_ERROR("[SushiTrack] Tracker creation failed: unknown exception.");
            return SUSHITRACK_RUNTIME_ERROR;
        }
    }

    /**
     * @brief Terminates a tracker instance and deallocates associated memory.
     * @param tracker Handle to tracker instance to destroy.
     */
    void sushitrack_destroy(sushitrack_tracker_t tracker)
    {
        if (!tracker) return;
        std::unique_ptr<sushitrack_tracker_impl_t> owner(
            static_cast<sushitrack_tracker_impl_t*>(tracker));

        try
        {
            SUSHITRACK_LOG_INFO("[SushiTrack] Destroying tracker instance.");
            owner.reset();
        }
        catch (...)
        {
            SUSHITRACK_LOG_ERROR("[SushiTrack] Destroy threw an unknown exception; the tracker is released.");
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
     * @brief Updates internal track states with new frame detections.
     * @param tracker Valid tracker instance handle.
     * @param objects Detection array; NULL when n_objects is zero.
     * @param out_tracks Pre-allocated buffer receiving active tracks.
     * @return Count of active tracks written into out_tracks.
     */
    int sushitrack_update(
        sushitrack_tracker_t       tracker,
        const sushitrack_object_t* objects,
        int                        n_objects,
        sushitrack_track_t*        out_tracks,
        int                        max_tracks,
        float                      dt)
    {
        if (!tracker || !out_tracks) return 0;
        if (n_objects > 0 && !objects) return 0;
        if (n_objects < 0 || max_tracks < 0) return 0;
        auto* impl = static_cast<sushitrack_tracker_impl_t*>(tracker);

        try
        {
            SushiTrack::ObjectVector detections;
            detections.reserve(static_cast<std::size_t>(n_objects));

            for (int i = 0; i < n_objects; ++i)
            {
                const auto& obj = objects[i];

                std::vector<float> feat_vec;
                if (impl->config.enable_reid && obj.feat && obj.feat_dim > 0)
                    feat_vec.assign(obj.feat, obj.feat + obj.feat_dim);

                SushiTrack::Rect<float> rect(obj.x, obj.y, obj.width, obj.height);
                detections.emplace_back(rect, obj.label, obj.score, std::move(feat_vec));
            }

            impl->last_results = impl->tracker->update(detections, dt);
            return populate_tracks(impl, impl->last_results, out_tracks, max_tracks);
        }
        catch (const std::exception& e)
        {
            SUSHITRACK_LOG_ERROR("[SushiTrack] Update failed: %s", e.what());
            return 0;
        }
        catch (...)
        {
            SUSHITRACK_LOG_ERROR("[SushiTrack] Update failed: unknown exception; returning zero tracks.");
            return 0;
        }
    }

    /**
     * @brief Retrieves currently managed tracklets from the tracker.
     * @param tracker Handle to tracker instance.
     * @param out_tracks Array to receive output track state structures.
     * @param max_tracks Maximum capacity of out_tracks array.
     * @return Number of tracklets written to output array.
     */
    int sushitrack_get_tracklets(
        sushitrack_tracker_t tracker,
        sushitrack_track_t*  out_tracks,
        int                  max_tracks)
    {
        if (!tracker || !out_tracks || max_tracks <= 0) return 0;
        auto* impl = static_cast<sushitrack_tracker_impl_t*>(tracker);

        try
        {
            auto tracks = impl->tracker->getTracks();
            return populate_tracks(impl, tracks, out_tracks, max_tracks);
        }
        catch (const std::exception& e)
        {
            SUSHITRACK_LOG_ERROR("[SushiTrack] Get tracklets failed: %s", e.what());
            return 0;
        }
        catch (...)
        {
            SUSHITRACK_LOG_ERROR("[SushiTrack] Get tracklets failed: unknown exception; returning zero tracks.");
            return 0;
        }
    }

    /**
     * @brief Configures a global callback function for system logging.
     * @param callback Function pointer to logging implementation.
     */
    void sushitrack_set_log_callback(sushitrack_log_callback_t callback)
    {
        SushiTrack::Logger::setCallback(callback);
    }

} // extern "C"
