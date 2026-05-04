/**
 * @file      SushiTrackC.h
 * @brief     C-compatible application programming interface for the SushiTrack tracker.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 * 
 * @details   Provides structures and function prototypes to facilitate integration with C 
 *            and languages supporting Foreign Function Interface (FFI) protocols.
 */

#ifndef SUSHITRACK_C_H
#define SUSHITRACK_C_H

#include <stdint.h>
#include "SushiTrack/SushiTrackTypes.h"

#ifdef __cplusplus
extern "C" 
{
#endif

    /** @brief Opaque handle representing a tracker instance. */
    typedef void* SushiTrackHdl;

    /**
    * @brief Representation of input detection data for a single object.
    */
    typedef struct 
    {
        float        x;        ///< Top-left horizontal coordinate.
        float        y;        ///< Top-left vertical coordinate.
        float        width;    ///< Horizontal dimension of the bounding box.
        float        height;   ///< Vertical dimension of the bounding box.
        float        score;    ///< Detection confidence probability in range [0, 1].
        int          label;    ///< Object classification identifier.
        const float* feat;     ///< Pointer to high-dimensional feature vector.
        int          feat_dim; ///< Dimensionality of the feature vector.

    } SushiTrack_Object;

    /**
    * @brief Representation of output tracking state for a single entity.
    */
    typedef struct 
    {
        int64_t track_id;        ///< Unique persistent identifier.
        int64_t age;             ///< Total frames the track has been alive.
        int64_t last_seen_frame; ///< Frame index where the track was last visible.
        /**
         * @brief Pointer to track visual feature vector.
         * @note The data pointed to by this field is only valid until the next
         *       call to SushiTrack_Update or SushiTrack_Destroy.
         */
        const float* feat;
        float x;                 ///< Estimated top-left horizontal coordinate.
        float y;                 ///< Estimated top-left vertical coordinate.
        float width;             ///< Estimated horizontal dimension.
        float height;            ///< Estimated vertical dimension.
        float score;             ///< Association confidence score.
        float lost_time;         ///< Time spent in the lost state.
        int   label;             ///< Object classification identifier.
        int   state;             ///< Track lifecycle state (0=New, 1=Tracked, 2=Lost, 3=Removed, 4=Tentative).
        int   kinematic_state;   ///< Kinematic movement state classification (0=Stationary, 1=Cruising, 2=Maneuvering, 3=Coasting).
        int   feat_dim;          ///< Dimensionality of the feature vector.
        int   is_activated;      ///< Flag indicating if the track has been activated.
        int   is_confirmed;      ///< Flag indicating if the track has been confirmed.
        int   hits;              ///< Number of successful detections matched.

        /**
         * @brief Number of frames elapsed since the last successful observation.
         *
         * Equals zero on the frame in which the track received an observation,
         * grows by one each subsequent frame the track remains in the lost
         * state, and resets to zero on re-association. Useful for consumers
         * deciding whether to draw the track based on a grace policy.
         */
        int   frames_since_last_update;

    } SushiTrack_Track;

    /**
    * @brief Configuration parameters for the SushiTrack tracking algorithm.
    */
    typedef struct 
    {
        int   enable_reid;      ///< Flag to enable visual appearance-based matching.
        int   time_aware;       ///< Flag to enable real-time interval processing.

        int   enable_fsm;                 ///< Flag to enable the kinematic state machine.
        float fsm_stationary_vel_thresh;  ///< Velocity threshold for stationary classification (units/s).
        float fsm_maneuver_accel_thresh;  ///< Acceleration threshold for maneuver classification (units/s^2).

        int   enable_tentative;        ///< Flag to enable intermediate track validation state.
        float tentative_match_thresh;  ///< Maximum cost limit for tentative track association.
        int   tentative_confirm_hits;  ///< Required consecutive observations for track confirmation.
        int   tentative_max_age;       ///< Maximum persistence for tentative tracks without observations.

        
        float tentative_fast_thresh;       ///< Score threshold for instant track confirmation.
        int   tentative_fast_hits;         ///< Hits required when score exceeds fast threshold.
        float tentative_slow_thresh;       ///< Score threshold below which confirmation is delayed.
        int   tentative_slow_hits_offset;  ///< Additional hits required for slow-moving tracks.
        int   tentative_vslow_hits_offset; ///< Additional hits required for very low confidence tracks.

        int   frame_rate;              ///< Target processing frequency.
        int   track_buffer;            ///< Maximum persistence for lost tracks in frame units.

        float track_thresh;     ///< Score threshold for detection classification.
        float high_thresh;      ///< Minimum score for initializing new tracks.
        float match_thresh;     ///< Maximum cost limit for primary association.
        
        int   enable_mahalanobis;   ///< Flag to enable Mahalanobis distance gating.
        float gating_thresh;        ///< Statistical distance threshold for gating.
        float max_area_ratio;       ///< Maximum allowed area ratio for candidate pairs.
        float motion_cost_weight;   ///< Weighting factor for motion similarity in fused metrics (effective when enable_mahalanobis=1).

        float std_weight_position;  ///< Process noise multiplier for position components.
        float std_weight_velocity;  ///< Process noise multiplier for velocity components.
        float kf_pos_weight;        ///< Multiplier for position state uncertainty.
        float kf_size_weight;       ///< Multiplier for dimension state uncertainty.
        float kf_vel_weight;        ///< Multiplier for velocity state uncertainty.
        int   enable_xyah;          ///< Flag for centroid-based coordinate representation.
        int   kalman_type;          ///< Type of Kalman Filter (0: Linear, 1: NSA).
        int   iou_plus_one;         ///< Flag for alternate intersection area calculation.
        int   enable_diou;          ///< Flag to enable Distance-IoU metric.

        float feature_momentum;     ///< Exponential moving average weight for visual feature updates.
        float reid_direct_assignment_weight; ///< Scaling factor for high-confidence direct visual matches.

        float iou_match_thresh;       ///< Maximum cost limit for secondary association.
        float unconfirmed_iou_thresh; ///< Maximum cost limit for unconfirmed track association.
        float duplicate_iou_thresh;   ///< Threshold for identifying redundant tracks.

        float reid_high_thresh;      ///< Maximum allowed cost for direct association in high-confidence ReIDFirst pass (effective when enable_reid=1 and enable_mahalanobis=0).
        float reid_high_iou_weight;  ///< IoU fallback weight in high-confidence ReIDFirst pass.
        float reid_low_thresh;       ///< Maximum allowed cost for direct association in low-confidence ReIDFirst pass (effective when enable_reid=1 and enable_mahalanobis=0).
        float reid_low_iou_weight;   ///< IoU fallback weight in low-confidence ReIDFirst pass.
        float reid_recovery_thresh;  ///< Maximum allowed cost for visual recovery of lost tracks (effective when enable_reid=1).
        int   max_feature_history;   ///< Maximum number of visual appearance vectors stored per tracklet.

    } SushiTrack_Params;

    /**
     * @brief Populates a parameters structure with default algorithm values.
     * 
     * @param params Pointer to the structure to be initialized.
     */
    void SushiTrack_GetDefaultParams(SushiTrack_Params* params);

    /**
     * @brief Instantiates a tracker based on provided configuration.
     * 
     * @param params Pointer to the configuration structure.
     * @return Handle to the initialized tracker instance, or NULL on failure.
     */
    SushiTrackHdl SushiTrack_Create(const SushiTrack_Params* params);

    /**
     * @brief Terminates a tracker instance and deallocates associated memory.
     * 
     * @param hdl Handle to the tracker instance to be destroyed.
     */
    void SushiTrack_Destroy(SushiTrackHdl hdl);

    /**
     * @brief Processes a new set of detections and updates internal track states.
     * 
     * @param hdl Handle to the tracker instance.
     * @param objects Array of input detection structures.
     * @param n_objects Number of elements in the objects array.
     * @param out_tracks Array to receive output track state structures.
     * @param max_tracks Maximum capacity of the out_tracks array.
     * @param dt Time interval elapsed since the previous update.
     * @return Number of active tracks written to the output array.
     */
    int SushiTrack_Update(SushiTrackHdl          hdl,
                        const SushiTrack_Object* __restrict objects,
                        int                   n_objects,
                        SushiTrack_Track*      __restrict out_tracks,
                        int                   max_tracks,
                        float                 dt);

    /**
     * @brief Extended instantiation providing status code feedback.
     * 
     * @param params Pointer to the configuration structure.
     * @param out_hdl Pointer to receive the initialized tracker handle.
     * @return Status code indicating success or specific failure reason.
     */
    SushiTrack_Status SushiTrack_CreateEx(const SushiTrack_Params* params, SushiTrackHdl* out_hdl);

    /**
     * @brief Configures a global callback function for system logging.
     * 
     * @param callback Function pointer to the logging implementation.
     */
    void SushiTrack_SetLogCallback(SushiTrack_LogCallback callback);

#ifdef __cplusplus
}
#endif

#endif