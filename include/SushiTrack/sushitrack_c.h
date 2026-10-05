/****************************************************************/
/* sushitrack_c.h                                               */
/* SushiTrack - https://github.com/SushiSystems/SushiTrack      */
/* Copyright (c) 2026 Sushi Systems                             */
/* Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.    */
/* Commercial use requires a licence from Sushi Systems.        */
/****************************************************************/

/**
 * @file sushitrack_c.h
 * @brief Declares the C-compatible application programming interface for SushiTrack.
 * @author Mustafa Garip
 */

#ifndef SUSHITRACK_C_H
#define SUSHITRACK_C_H

#include <stddef.h>
#include <stdint.h>

// Holds preprocessor macros only, so this header stays C; see docs/guides/DEPLOY_PACKAGES.md.
#include "SushiTrack/sushitrack_export.hpp"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Defines return status codes for tracker operations.
     */
    typedef enum
    {
        SUSHITRACK_SUCCESS = 0,                 ///< Operation completed successfully.
        SUSHITRACK_INVALID_ARGUMENT = -1,       ///< Parameter validation failure.
        SUSHITRACK_INITIALIZATION_FAILURE = -2, ///< Internal component setup failure.
        SUSHITRACK_OUT_OF_MEMORY = -3,          ///< Dynamic memory allocation failure.
        SUSHITRACK_RUNTIME_ERROR = -4,          ///< Unhandled execution exception.
        SUSHITRACK_BUFFER_OVERFLOW = -5         ///< Target buffer capacity exceeded.
    } sushitrack_status_t;

    /**
     * @brief Defines system logging severity levels.
     */
    typedef enum
    {
        SUSHITRACK_LOG_DEBUG = 0, ///< Detailed diagnostic information.
        SUSHITRACK_LOG_INFO  = 1, ///< Informational system messages.
        SUSHITRACK_LOG_WARN  = 2, ///< Non-terminating operational warnings.
        SUSHITRACK_LOG_ERROR = 3  ///< Terminating operational errors.
    } sushitrack_log_level_t;

    /**
     * @brief Defines function pointer signature for custom log handling.
     */
    typedef void (*sushitrack_log_callback_t)(sushitrack_log_level_t level, const char* message);

    /** @brief Opaque handle representing a tracker instance. */
    typedef void* sushitrack_tracker_t;

    /**
     * @brief Represents input detection data for a single object.
     */
    typedef struct
    {
        float        x;        ///< Top-left horizontal coordinate.
        float        y;        ///< Top-left vertical coordinate.
        float        width;    ///< Horizontal dimension of bounding box.
        float        height;   ///< Vertical dimension of bounding box.
        float        score;    ///< Detection confidence score in range [0, 1].
        int          label;    ///< Object classification identifier.
        const float* feat;     ///< Pointer to visual feature vector.
        int          feat_dim; ///< Dimensionality of the feature vector.
    } sushitrack_object_t;

    /**
     * @brief Represents output tracking state for a single entity.
     */
    typedef struct
    {
        int64_t      track_id;                 ///< Unique persistent identifier.
        int64_t      age;                      ///< Total frames the track has been alive.
        int64_t      last_seen_frame;          ///< Frame index where track was last visible.
        const float* feat;                     ///< Pointer to track visual feature vector.
        float        x;                        ///< Estimated top-left horizontal coordinate.
        float        y;                        ///< Estimated top-left vertical coordinate.
        float        width;                    ///< Estimated horizontal dimension.
        float        height;                   ///< Estimated vertical dimension.
        float        score;                    ///< Association confidence score.
        float        lost_time;                ///< Time spent in the lost state.
        int          label;                    ///< Object classification identifier.
        int          state;                    ///< Track lifecycle state.
        int          kinematic_state;          ///< Kinematic movement state classification.
        int          feat_dim;                 ///< Dimensionality of feature vector.
        int          is_activated;             ///< Flag indicating if track has been activated.
        int          is_confirmed;             ///< Flag indicating if track has been confirmed.
        int          hits;                     ///< Number of successful detections matched.
        int          frames_since_last_update; ///< Frames elapsed since last observation.
        float        maneuver_probability;     ///< Manoeuvre likelihood in range [0, 1].
    } sushitrack_track_t;

    /**
     * @brief Configures parameters for the tracking algorithm.
     */
    typedef struct
    {
        // Basic parameters
        int   time_aware;
        int   frame_rate;
        int   track_buffer;
        float track_thresh;
        float high_thresh;
        float match_thresh;
        int   enable_tentative;
        int   enable_reid;
        int   enable_mahalanobis;

        // Association parameters
        int   primary_assoc_mode;
        int   fuse_score;
        float iou_match_thresh;
        float unconfirmed_iou_thresh;
        float duplicate_iou_thresh;

        // Coordinate system parameters
        int   enable_xyah;
        int   iou_type;
        int   iou_plus_one;
        int   kalman_type;

        // ReID parameters
        float feature_momentum;
        float reid_high_thresh;
        float reid_high_iou_weight;
        float reid_low_thresh;
        float reid_low_iou_weight;
        float reid_recovery_thresh;
        int   max_feature_history;

        // Observation-Centric Momentum parameters
        int   enable_ocm;
        int   enable_oru;
        float ocm_vdc_weight;
        int   ocm_delta_t;
        float ocm_min_velocity;
        int   ocm_max_age;
        float ocm_maneuver_weight_scale;

        // Tentative logic parameters
        float tentative_match_thresh;
        int   tentative_confirm_hits;
        float tentative_max_age_seconds;
        int   tentative_max_miss_frames;
        float tentative_init_suppress_iou;

        // Mahalanobis gating parameters
        float gating_thresh;
        float motion_cost_weight;
        float max_area_ratio;

        // Extended parameters
        float tentative_fast_thresh;
        int   tentative_fast_hits;
        float tentative_slow_thresh;
        int   tentative_slow_hits_offset;
        int   tentative_vslow_hits_offset;
        float reid_direct_assignment_weight;
    } sushitrack_params_t;

    /**
     * @brief Populates a parameters structure with default algorithm values.
     * @param params Pointer to the structure to be initialized.
     */
    SUSHITRACK_API void sushitrack_get_default_params(sushitrack_params_t* params);

    /**
     * @brief Instantiates a tracker based on provided configuration.
     * @param params Pointer to configuration structure, or NULL for defaults.
     * @return Handle to initialized tracker instance, or NULL on failure.
     */
    SUSHITRACK_API sushitrack_tracker_t sushitrack_create(const sushitrack_params_t* params);

    /**
     * @brief Instantiates a tracker directly from a configuration file.
     * @param config_path Path to JSON file, or NULL to use default resolution.
     * @return Handle to initialized tracker instance, or NULL on failure.
     */
    SUSHITRACK_API sushitrack_tracker_t sushitrack_create_tracker(const char* config_path);

    /**
     * @brief Instantiates a tracker directly from a JSON configuration file.
     * @param path Explicit file path, or NULL to use standard resolution order.
     * @return Handle to initialized tracker instance, or NULL on failure.
     */
    SUSHITRACK_API sushitrack_tracker_t sushitrack_create_from_config(const char* path);

    /**
     * @brief Extended instantiation providing status code feedback.
     * @param params Pointer to configuration structure.
     * @param out_tracker Pointer to receive initialized tracker handle.
     * @return Status code indicating success or failure reason.
     */
    SUSHITRACK_API sushitrack_status_t sushitrack_create_ex(
        const sushitrack_params_t* params,
        sushitrack_tracker_t*      out_tracker);

    /**
     * @brief Terminates a tracker instance and deallocates associated memory.
     * @param tracker Handle to tracker instance to destroy.
     */
    SUSHITRACK_API void sushitrack_destroy(sushitrack_tracker_t tracker);

    /**
     * @brief Terminates a tracker instance and deallocates associated memory.
     * @param tracker Handle to tracker instance to destroy.
     */
    SUSHITRACK_API void sushitrack_destroy_tracker(sushitrack_tracker_t tracker);

    /**
     * @brief Updates internal track states with new frame detections.
     * @param tracker Valid tracker instance handle.
     * @param objects Detection array; NULL when n_objects is zero.
     * @param out_tracks Pre-allocated buffer receiving active tracks.
     * @return Count of active tracks written into out_tracks.
     */
    SUSHITRACK_API int sushitrack_update(
        sushitrack_tracker_t       tracker,
        const sushitrack_object_t* objects,
        int                        n_objects,
        sushitrack_track_t*        out_tracks,
        int                        max_tracks,
        float                      dt);

    /**
     * @brief Retrieves currently managed tracklets from the tracker.
     * @param tracker Handle to tracker instance.
     * @param out_tracks Array to receive output track state structures.
     * @param max_tracks Maximum capacity of out_tracks array.
     * @return Number of tracklets written to output array.
     */
    SUSHITRACK_API int sushitrack_get_tracklets(
        sushitrack_tracker_t tracker,
        sushitrack_track_t*  out_tracks,
        int                  max_tracks);

    /**
     * @brief Overlays a JSON configuration file onto a parameters structure.
     * @param path Explicit file path, or NULL to use standard resolution.
     * @param params Parameters structure updated in place.
     * @return Status code indicating success or failure reason.
     */
    SUSHITRACK_API sushitrack_status_t sushitrack_load_params_from_json(
        const char*          path,
        sushitrack_params_t* params);

    /**
     * @brief Configures a global callback function for system logging.
     * @param callback Function pointer to logging implementation.
     */
    SUSHITRACK_API void sushitrack_set_log_callback(sushitrack_log_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif // SUSHITRACK_C_H
