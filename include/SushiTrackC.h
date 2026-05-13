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
        // ====================================================================
        // BASIC PARAMETERS
        // ====================================================================

        /** @brief Flag to enable real-time interval processing rather than frame-based intervals. */
        int   time_aware;

        /** @brief Target processing frequency (frames per second). */
        int   frame_rate;

        /** @brief Maximum number of frames to persist a track in the lost state before deletion. */
        int   track_buffer;

        /** @brief Score threshold for classifying detections into high and low confidence pools. */
        float track_thresh;

        /** @brief Minimum confidence score required to initialize a new tracklet. */
        float high_thresh;

        /** @brief Maximum cost limit allowed for the primary tracking association step. */
        float match_thresh;

        /** @brief Flag to enable the tentative, unconfirmed track lifecycle state. */
        int   enable_tentative;

        /** @brief Flag to enable the Kinematic Finite State Machine for dynamic noise adjustment. */
        int   enable_fsm;

        /** @brief Flag to enable ReID visual feature extraction and association. */
        int   enable_reid;

        /** @brief Flag to enable Mahalanobis distance gating to filter impossible associations. */
        int   enable_mahalanobis;

        // ====================================================================
        // ADVANCED PARAMETERS
        // ====================================================================

        // --- Association ---

        /** @brief Primary association mode (0: Age-cascade legacy, 1: Global LAPJV). */
        int   primary_assoc_mode;

        /** @brief Flag to enable confidence-based score fusion in the primary association matrix. */
        int   fuse_score;

        /** @brief Maximum cost limit allowed for the secondary (IoU-based) association step. */
        float iou_match_thresh;

        /** @brief Maximum cost limit allowed for associating unconfirmed tracks. */
        float unconfirmed_iou_thresh;

        /** @brief IoU threshold to identify and remove redundant, duplicate tracks. */
        float duplicate_iou_thresh;

        // --- Coordinate System ---

        /** @brief Flag to enable centroid-based (XYAH) coordinate representation instead of XYWH. */
        int   enable_xyah;

        /** @brief Defines the type of IoU metric used for spatial penalization (0: IoU, 1: DIoU, 2: GIoU, 3: SIoU). */
        int   iou_type;

        /** @brief Flag to add a +1 offset in bounding box dimension calculations (inclusive). */
        int   iou_plus_one;

        /** @brief Defines the type of Kalman Filter utilized (0: Linear, 1: Noise Scale Adaptive). */
        int   kalman_type;

        // --- ReID Params ---

        /** @brief Exponential moving average weight for smoothing visual feature updates. */
        float feature_momentum;

        /** @brief Distance threshold for direct visual association in the high-confidence ReID pass. */
        float reid_high_thresh;

        /** @brief Fallback weight applied to the IoU cost during the high-confidence ReID pass. */
        float reid_high_iou_weight;

        /** @brief Distance threshold for direct visual association in the low-confidence ReID pass. */
        float reid_low_thresh;

        /** @brief Fallback weight applied to the IoU cost during the low-confidence ReID pass. */
        float reid_low_iou_weight;

        /** @brief Distance threshold allowed for recovering lost tracks strictly via visual features. */
        float reid_recovery_thresh;

        /** @brief Maximum capacity of visual appearance vectors stored in the gallery per tracklet. */
        int   max_feature_history;

        // --- FSM Params ---

        /** @brief Velocity threshold to classify an object's kinematic state as stationary. */
        float fsm_stationary_vel_thresh;

        /** @brief Acceleration threshold to classify an object's kinematic state as maneuvering. */
        float fsm_maneuver_accel_thresh;

        // --- Tentative Logic ---

        /** @brief Maximum cost limit allowed for tentative track associations. */
        float tentative_match_thresh;

        /** @brief Number of consecutive successful matches required to confirm a tentative track. */
        int   tentative_confirm_hits;

        /**
         * @brief Maximum time a tentative track may go without a match, in seconds.
         *
         * Works identically in both timing modes. In frame-based mode the value is
         * compared against accumulated_lost_time_, which predict() increments by
         * 1/frame_rate per frame. Default 0.08 s equals 2 frames at 25 fps.
         */
        float tentative_max_age_seconds;

        // --- Mahalanobis Gating ---

        /** @brief Proximity threshold for initial broad spatial filtering before statistical gating. */
        float proximity_thresh;

        /** @brief Chi-squared statistical distance threshold utilized for Mahalanobis gating. */
        float gating_thresh;

        /** @brief Weighting factor applied to motion similarity within fused association costs. */
        float motion_cost_weight;

        /** @brief Maximum allowed area ratio disparity to filter impossible candidate pairs. */
        float max_area_ratio;

        // ====================================================================
        // EXTENDED PARAMETERS
        // ====================================================================

        /** @brief Score threshold for instant tentative track confirmation. */
        float tentative_fast_thresh;
        /** @brief Hits required when score exceeds fast threshold. */
        int   tentative_fast_hits;
        /** @brief Score threshold below which confirmation is delayed. */
        float tentative_slow_thresh;
        /** @brief Additional hits required for slow-moving tentative tracks. */
        int   tentative_slow_hits_offset;
        /** @brief Additional hits required for very low confidence tentative tracks. */
        int   tentative_vslow_hits_offset;
        /** @brief Scaling factor for high-confidence direct visual matches. */
        float reid_direct_assignment_weight;

        // ====================================================================
        // INTERACTION CONTEXT (GLOBAL OCCLUSION MODELLING)
        // ====================================================================

        /**
         * @brief Master switch for the scene-level interaction context module.
         *
         * When non-zero, the tracker builds a per-frame interaction graph over
         * the predicted bounding boxes of all confirmed and lost tracklets,
         * captures an occluder identifier at the moment a track transitions
         * to the lost state, and biases lost-recovery association against
         * detections that geometrically coincide with that occluder while the
         * snapshot remains valid. Has no effect unless enable_fsm is also
         * non-zero.
         */
        int   enable_interaction_context;

        /**
         * @brief Minimum predicted-box intersection-over-union required to
         *        register a pairwise interaction in the graph.
         */
        float interaction_iou_thresh;

        /**
         * @brief Additive cost penalty applied to recovery matches that
         *        coincide with a still-valid occluder snapshot.
         */
        float interaction_occluder_penalty;

        /**
         * @brief Base lifetime of an occluder snapshot, in seconds.
         *
         * Interpreted the same way in both timing modes. In frame-based mode
         * the value is multiplied by frame_rate to obtain the equivalent frame
         * count. The effective window is further scaled by the occluder's
         * kinematic state at evaluation time. Default 2.0 s equals 50 frames
         * at 25 fps.
         */
        float interaction_occluder_validity_seconds;

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
