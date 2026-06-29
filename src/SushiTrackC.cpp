/**
 * @file      SushiTrackC.cpp
 * @brief     C-compatible API implementation for the SushiTrack tracker.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#include "SushiTrackC.h"
#include "SushiTrack/SushiTrackTracker.h"
#include "SushiTrack/KalmanFilter.h"
#include "SushiTrack/NSAKalmanFilter.h"
#include "SushiTrack/AssignmentSolvers.h"
#include "SushiTrack/DistanceStrategies.h"
#include "SushiTrack/Object.h"
#include "SushiTrack/Logger.h"
#include "SushiTrack/KalmanFilterRegistry.h"

/**
 * @brief Internal handle implementation encapsulating tracker state and configuration.
 *
 * Maintains a persistent reference to the core tracker instance and its
 * resolved configuration parameters.
 */
struct SushiTrackHdlImpl
{
    std::unique_ptr<SushiTrack::SushiTrackTracker> tracker;
    SushiTrack::TrackerConfig                     config;
    std::vector<std::shared_ptr<SushiTrack::Tracklet>> last_results;
    std::vector<std::vector<float>>                   last_features;
};

extern "C"
{
    /**
     * @brief Populates a parameters structure with default algorithm values.
     *
     * @param params Pointer to the structure to be initialized.
     */
    void SushiTrack_GetDefaultParams(SushiTrack_Params* params)
    {
        if (!params) return;

        params->enable_reid  = 0;
        params->time_aware   = 0;

        params->enable_fsm                = 0;
        params->fsm_stationary_vel_thresh = 40.0f;
        params->fsm_maneuver_accel_thresh = 250.0f;
        params->fsm_reference_height      = 100.0f;

        params->frame_rate   = 25;
        params->track_buffer = 30;

        params->track_thresh = 0.2f;
        params->high_thresh  = 0.5f;
        params->match_thresh = 0.7f;

        params->enable_tentative = 0;

        params->enable_mahalanobis = 0;
        params->gating_thresh      = 9.4877f;
        params->proximity_thresh   = 0.7f;
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

        params->tentative_match_thresh   = 0.3f;
        params->tentative_confirm_hits   = 3;
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

        params->enable_ocm      = 0;
        params->ocm_vdc_weight  = 0.1f;
        params->ocm_delta_t     = 3;
        params->ocm_min_velocity = 1.0f;
        params->ocm_max_age     = 1;
    }

    /**
     * @brief Instantiates a tracker based on provided configuration.
     *
     * When @p params is NULL the tracker is built from the runtime configuration
     * file instead: compiled defaults are seeded and then overlaid with
     * sushitrack.json resolved through the standard order (see
     * SushiTrack_CreateFromConfig). Pass an explicit structure to bypass file
     * loading entirely and configure the tracker programmatically.
     *
     * @param params Pointer to the configuration structure, or NULL to load
     *               from sushitrack.json.
     * @return Handle to the initialized tracker instance, or NULL on failure.
     */
    SushiTrackHdl SushiTrack_Create(const SushiTrack_Params* params)
    {
        if (!params) return SushiTrack_CreateFromConfig(nullptr);

        SushiTrackHdl     hdl    = nullptr;
        SushiTrack_Status status = SushiTrack_CreateEx(params, &hdl);

        if (status != SUSHITRACK_SUCCESS) return nullptr;

        return hdl;
    }

    /**
     * @brief Extended instantiation providing status code feedback.
     *
     * @param params  Pointer to the configuration structure.
     * @param out_hdl Pointer to receive the initialized tracker handle.
     * @return Status code indicating success or specific failure reason.
     */
    SushiTrack_Status SushiTrack_CreateEx(const SushiTrack_Params* params, SushiTrackHdl* out_hdl)
    {
        if (!params || !out_hdl) return SUSHITRACK_INVALID_ARGUMENT;

        try
        {
            SushiTrack::TrackerConfig config;
            config.enable_reid  = (params->enable_reid  != 0);
            config.time_aware   = (params->time_aware   != 0);

            config.enable_fsm                = (params->enable_fsm != 0);
            config.fsm_stationary_vel_thresh = params->fsm_stationary_vel_thresh;
            config.fsm_maneuver_accel_thresh = params->fsm_maneuver_accel_thresh;
            config.fsm_reference_height      = params->fsm_reference_height;

            config.frame_rate   = params->frame_rate;
            config.track_buffer = params->track_buffer;


            config.track_thresh = params->track_thresh;
            config.high_thresh  = params->high_thresh;
            config.match_thresh = params->match_thresh;

            config.enable_tentative = (params->enable_tentative != 0);

            config.enable_mahalanobis = (params->enable_mahalanobis != 0);
            config.gating_thresh      = params->gating_thresh;
            config.proximity_thresh   = params->proximity_thresh;
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
            config.ocm_vdc_weight   = params->ocm_vdc_weight;
            config.ocm_delta_t      = params->ocm_delta_t;
            config.ocm_min_velocity = params->ocm_min_velocity;
            config.ocm_max_age      = params->ocm_max_age;

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
                    // Fallback if the requested Kalman type is not found
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

            /**
             * @brief Plain-IoU calculator for stages with IoU-tuned thresholds.
             *
             * The duplicate and tentative thresholds are calibrated against the
             * plain IoU distance (one minus IoU). DIoU adds a centroid-distance
             * penalty that is appropriate for primary association but skews these
             * thresholds, so a dedicated plain-IoU instance is provided here and
             * injected into the duplicate and tentative metrics. This preserves
             * the open-closed property: enabling DIoU for primary matching does
             * not silently change the semantics of unrelated stages.
             */
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
                    config.motion_cost_weight,
                    config.proximity_thresh);

                if (config.enable_reid)
                {
                    /**
                     * @brief Fused association path with Mahalanobis gating.
                     *
                     * Combines spatial overlap, cosine appearance distance, and a
                     * statistical gate. Tentative and duplicate metrics are kept
                     * on plain IoU because their thresholds are tuned to that
                     * scale and reusing the fused metric here would change their
                     * semantics outside of the user's intent.
                     */
                    auto cosine_calc     = std::make_shared<SushiTrack::CosineCostCalculator>();
                    high_conf_metric     = std::make_shared<SushiTrack::FusedCostCalculator>(
                        config.reid_high_iou_weight,
                        config.reid_high_thresh, config.gating_thresh,
                        active_gate_validator,
                        gated_iou, cosine_calc, config.reid_direct_assignment_weight,
                        config.proximity_thresh);
                    low_conf_metric      = std::make_shared<SushiTrack::FusedCostCalculator>(
                        config.reid_low_iou_weight,
                        config.reid_low_thresh, config.gating_thresh,
                        active_gate_validator,
                        gated_iou, cosine_calc, config.reid_direct_assignment_weight,
                        config.proximity_thresh);
                    lost_recovery_metric = std::make_shared<SushiTrack::PureReIDCostCalculator>(
                        config.reid_recovery_thresh, config.gating_thresh, active_gate_validator);
                }
                else
                {
                    /**
                     * @brief Mahalanobis-gated IoU path without appearance features.
                     *
                     * The gated IoU metric is used for the primary and secondary
                     * stages. Tentative and duplicate metrics remain on plain IoU
                     * so their thresholds keep their original meaning.
                     */
                    high_conf_metric = gated_iou;
                    low_conf_metric  = gated_iou;
                }
            }
            else if (config.enable_reid)
            {
                /**
                 * @brief Appearance-first path with IoU fallback.
                 *
                 * Uses ReID similarity for the primary and secondary association
                 * with IoU as a fallback when appearance is uninformative.
                 * Tentative and duplicate metrics stay on plain IoU.
                 */
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
                /**
                 * @brief Plain IoU or DIoU path.
                 *
                 * The primary and secondary stages use the configured IoU variant
                 * (DIoU, GIoU, SIoU when iou_type is set, plain IoU otherwise). Tentative
                 * and duplicate metrics remain on plain IoU regardless of the
                 * configuration to preserve the threshold semantics.
                 */
                high_conf_metric = iou_calc;
                low_conf_metric  = iou_calc;
            }

            /**
             * @brief Score fusion is applied to the primary cost matrix only.
             *
             * Wrapping low-confidence, tentative, duplicate, and lost-recovery
             * metrics would over-penalise pairs that are already low-score by
             * construction (low-confidence pass) or operate on a different
             * scale (cosine, Mahalanobis). The reference ByteTrack pipeline
             * uses fuse_score on the first association only, and this matches
             * that behaviour.
             */
            if (config.fuse_score)
            {
                high_conf_metric = std::make_shared<SushiTrack::FuseScoreDecorator>(high_conf_metric);
            }

            /**
             * @brief Observation-centric momentum is the outermost primary-stage
             *        decorator.
             *
             * It must wrap the final high-confidence metric (after score fusion)
             * so it re-ranks the same costs the solver sees. The contribution is
             * a bounded subtractive tie-breaker, applied only to high-confidence
             * primary association; the low-confidence, tentative, duplicate, and
             * lost-recovery stages are intentionally left untouched, mirroring the
             * placement of fuse_score.
             */
            if (config.enable_ocm)
            {
                high_conf_metric = std::make_shared<SushiTrack::ObservationMomentumDecorator>(
                    high_conf_metric,
                    config.ocm_vdc_weight,
                    config.ocm_min_velocity,
                    config.ocm_max_age);
            }

            auto impl     = new SushiTrackHdlImpl();
            impl->config  = config;
            impl->tracker = std::make_unique<SushiTrack::SushiTrackTracker>(
                config, kf_factory, std::move(solver),
                high_conf_metric,
                low_conf_metric,
                tentative_metric,
                duplicate_metric,
                lost_recovery_metric);

            *out_hdl = static_cast<SushiTrackHdl>(impl);

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
            SUSHITRACK_LOG_ERROR(e.what());
            return SUSHITRACK_RUNTIME_ERROR;
        }
    }

    /**
     * @brief Terminates a tracker instance and deallocates associated memory.
     *
     * @param hdl Handle to the tracker instance to be destroyed.
     */
    void SushiTrack_Destroy(SushiTrackHdl hdl)
    {
        if (!hdl) return;
        SUSHITRACK_LOG_INFO("[SushiTrack] Destroying tracker instance.");
        delete static_cast<SushiTrackHdlImpl*>(hdl);
    }

    /**
     * @brief Processes a new set of detections and updates internal track states.
     *
     * @param hdl        Handle to the tracker instance.
     * @param objects    Array of input detection structures.
     * @param n_objects  Number of elements in the objects array.
     * @param out_tracks Array to receive output track state structures.
     * @param max_tracks Maximum capacity of the out_tracks array.
     * @param dt         Time interval elapsed since the previous update.
     * @return Number of active tracks written to the output array.
     */
    int SushiTrack_Update(SushiTrackHdl                    hdl,
                         const SushiTrack_Object* __restrict objects,
                         int                             n_objects,
                         SushiTrack_Track*        __restrict out_tracks,
                         int                             max_tracks,
                         float                           dt)
    {
        if (!hdl || !out_tracks) return 0;
        if (n_objects > 0 && !objects) return 0;
        if (n_objects < 0 || max_tracks < 0) return 0;
        auto* impl = static_cast<SushiTrackHdlImpl*>(hdl);

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
            auto& results = impl->last_results;

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
        catch (const std::exception& e)
        {
            SUSHITRACK_LOG_ERROR("[SushiTrack] Update failed: %s", e.what());
            return 0;
        }
    }

    /**
     * @brief Configures a global callback function for system logging.
     *
     * @param callback Function pointer to the logging implementation.
     */
    void SushiTrack_SetLogCallback(SushiTrack_LogCallback callback)
    {
        SushiTrack::Logger::setCallback(callback);
    }

} // extern "C"
