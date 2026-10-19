/**************************************************************************/
/* tracker_bridge.h                                                       */
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
 * @file tracker_bridge.h
 * @brief Declares the C entry points of tracker_bridge, which the Python inference pipeline loads.
 * @author Mustafa Garip
 */

#pragma once

#if defined(_WIN32)
#define TRACKER_BRIDGE_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#define TRACKER_BRIDGE_API __attribute__((visibility("default")))
#else
#define TRACKER_BRIDGE_API
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    /** @brief Holds one detection handed in from C. */
    typedef struct
    {
            float x;           /**< Left edge. */
            float y;           /**< Top edge. */
            float w;           /**< Width. */
            float h;           /**< Height. */
            float score;       /**< Detection confidence. */
            int label;         /**< Class label. */
            const float* feat; /**< Appearance embedding, or NULL. */
            int feat_dim;      /**< Length of feat. */
    } CDetection;

    /** @brief Holds one track handed back to C. */
    typedef struct
    {
            int track_id; /**< Track identity. */
            float x;      /**< Left edge. */
            float y;      /**< Top edge. */
            float w;      /**< Width. */
            float h;      /**< Height. */
            float score;  /**< Track confidence. */
    } CTrackResult;

    /**
     * @brief Returns a new tracker of the registered @p name, in any case, or NULL.
     */
    TRACKER_BRIDGE_API void* create_tracker(const char* name);

    /**
     * @brief Releases a tracker create_tracker returned; NULL is ignored.
     */
    TRACKER_BRIDGE_API void delete_tracker(void* tracker);

    /**
     * @brief Runs one frame and writes at most @p max_results tracks.
     * @param num_results Receives the number of tracks written.
     */
    TRACKER_BRIDGE_API void tracker_process_c(void* tracker, const CDetection* detections,
                                              int num_detections, CTrackResult* results,
                                              int* num_results, int max_results);

#ifdef __cplusplus
}
#endif
