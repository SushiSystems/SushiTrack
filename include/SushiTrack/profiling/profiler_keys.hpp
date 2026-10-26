/**************************************************************************/
/* profiler_keys.hpp                                                      */
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
 * @file profiler_keys.hpp
 * @brief Declares the zone, counter, group, timeline and fact names SushiTrack records under.
 * @author Mustafa Garip
 * @see docs/reference/PROFILER_KEYS.md
 */

#pragma once

namespace SushiTrack::ProfilerKeys
{
    /// @brief The timeline the host marks once per frame, so each period is one `update` call.
    inline constexpr const char* K_TIMELINE_FRAME = "sushitrack.frame";

    /// @brief The group of the zones that run the frame and its track lifecycle.
    inline constexpr const char* K_GROUP_TRACK = "Track";
    /// @brief The group of the zones that match tracks with detections.
    inline constexpr const char* K_GROUP_ASSOCIATE = "Associate";
    /// @brief The group of the zone that moves the tracks to the frame's time.
    inline constexpr const char* K_GROUP_FILTER = "Filter";
    /// @brief The group of the zone that sorts the frame's detections.
    inline constexpr const char* K_GROUP_DETECT = "Detect";

    /// @brief One call of Tracker::update, level 1.
    inline constexpr const char* K_ZONE_UPDATE = "update";
    /// @brief Splitting the detections into the high and low pools, level 1.
    inline constexpr const char* K_ZONE_SPLIT = "split";
    /// @brief Predicting every pooled track, level 1.
    inline constexpr const char* K_ZONE_PREDICT = "predict";
    /// @brief Matching tracks with the high-confidence detections, level 1.
    inline constexpr const char* K_ZONE_MATCH_HIGH = "match_high";
    /// @brief Matching the leftover tracks with the low-confidence detections, level 1.
    inline constexpr const char* K_ZONE_MATCH_LOW = "match_low";
    /// @brief Recovering lost tracks by appearance, level 1; present with ReID only.
    inline constexpr const char* K_ZONE_LOST_RECOVERY = "lost_recovery";
    /// @brief The tentative stage, level 1; present with `enable_tentative`.
    inline constexpr const char* K_ZONE_TENTATIVE = "tentative";
    /// @brief The unconfirmed stage, level 1; present without `enable_tentative`.
    inline constexpr const char* K_ZONE_UNCONFIRMED = "unconfirmed";
    /// @brief Removing lost tracks past their buffer, level 1.
    inline constexpr const char* K_ZONE_CLEANUP_LOST = "cleanup_lost";
    /// @brief Rebuilding the track pools and dropping duplicates, level 1.
    inline constexpr const char* K_ZONE_FINALIZE = "finalize";
    /// @brief Sorting the tracks and copying them into the caller's buffer, level 1.
    inline constexpr const char* K_ZONE_OUTPUT = "output";
    /// @brief One assignment solve, level 2.
    inline constexpr const char* K_ZONE_SOLVE = "solve";
    /// @brief One cost matrix fill, level 2.
    inline constexpr const char* K_ZONE_COST_MATRIX = "cost_matrix";

    /// @brief The unit of a counter that counts detections.
    inline constexpr const char* K_UNIT_DETECTIONS = "detections";
    /// @brief The unit of a counter that counts tracks.
    inline constexpr const char* K_UNIT_TRACKS = "tracks";
    /// @brief The unit of a counter that counts cost matrix cells.
    inline constexpr const char* K_UNIT_CELLS = "cells";

    /// @brief Detections handed to the frame, level 1.
    inline constexpr const char* K_COUNTER_DETECTIONS_IN = "detections_in";
    /// @brief Detections at or above `track_thresh`, level 2.
    inline constexpr const char* K_COUNTER_DETECTIONS_HIGH = "detections_high";
    /// @brief Detections below `track_thresh`, level 2.
    inline constexpr const char* K_COUNTER_DETECTIONS_LOW = "detections_low";
    /// @brief Tracks in the pool the high stage matches, level 2.
    inline constexpr const char* K_COUNTER_TRACKS_POOL = "tracks_pool";
    /// @brief Tentative tracks entering the frame, level 2.
    inline constexpr const char* K_COUNTER_TRACKS_TENTATIVE_IN = "tracks_tentative_in";
    /// @brief Tracks awaiting confirmation without the tentative stage, level 2.
    inline constexpr const char* K_COUNTER_TRACKS_NON_ACTIVE = "tracks_non_active";
    /// @brief Tracks matched, updated or recovered by the high stage, level 1.
    inline constexpr const char* K_COUNTER_MATCHES_HIGH = "matches_high";
    /// @brief Tracks matched by the low stage, level 2.
    inline constexpr const char* K_COUNTER_MATCHES_LOW = "matches_low";
    /// @brief Tracks the frame marked lost, level 1.
    inline constexpr const char* K_COUNTER_TRACKS_NEWLY_LOST = "tracks_newly_lost";
    /// @brief Low-pool detections no track took, level 2.
    inline constexpr const char* K_COUNTER_DETECTIONS_LOW_REMAINING = "detections_low_remaining";
    /// @brief Lost tracks matched again, level 1.
    inline constexpr const char* K_COUNTER_TRACKS_RECOVERED = "tracks_recovered";
    /// @brief Tentative or unconfirmed tracks confirmed by the frame, level 1.
    inline constexpr const char* K_COUNTER_TENTATIVE_CONFIRMED = "tentative_confirmed";
    /// @brief Tentative or unconfirmed tracks removed by the frame, level 2.
    inline constexpr const char* K_COUNTER_TENTATIVE_REMOVED = "tentative_removed";
    /// @brief Tracks started by the frame, level 2.
    inline constexpr const char* K_COUNTER_TRACKS_STARTED = "tracks_started";
    /// @brief Tracks alive in the Tracked state when the frame ends, level 1.
    inline constexpr const char* K_COUNTER_ALIVE_TRACKED = "alive_tracked";
    /// @brief Tracks alive in the Lost state when the frame ends, level 1.
    inline constexpr const char* K_COUNTER_ALIVE_LOST = "alive_lost";
    /// @brief Tracks alive in the Tentative state when the frame ends, level 2.
    inline constexpr const char* K_COUNTER_ALIVE_TENTATIVE = "alive_tentative";
    /// @brief Tracks removed by the frame, level 2.
    inline constexpr const char* K_COUNTER_REMOVED = "removed_this_frame";
    /// @brief Tracks written to the caller's buffer, level 1.
    inline constexpr const char* K_COUNTER_TRACKS_WRITTEN = "tracks_written";
    /// @brief Tracks that did not fit the caller's buffer, level 1.
    inline constexpr const char* K_COUNTER_TRACKS_DROPPED = "tracks_dropped";
    /// @brief Cost matrix cells filled by the frame's matching stages, level 2.
    inline constexpr const char* K_COUNTER_COST_CELLS = "cost_cells";

    /// @brief A track the low stage marked lost, level 2.
    inline constexpr const char* K_INSTANT_TRACK_LOST = "track_lost";
    /// @brief A tentative or unconfirmed track that became confirmed, level 2.
    inline constexpr const char* K_INSTANT_TRACK_CONFIRMED = "track_confirmed";
    /// @brief A track that was removed, level 2.
    inline constexpr const char* K_INSTANT_TRACK_REMOVED = "track_removed";
    /// @brief A lost track that matched a detection again, level 2.
    inline constexpr const char* K_INSTANT_TRACK_RECOVERED = "track_recovered";

    /// @brief The environment fact naming the library.
    inline constexpr const char* K_FACT_LIBRARY = "sushitrack.library";
    /// @brief The environment fact holding the library's version.
    inline constexpr const char* K_FACT_VERSION = "sushitrack.version";
    /// @brief The environment fact holding the commit the library was built from.
    inline constexpr const char* K_FACT_COMMIT = "sushitrack.commit";
    /// @brief The environment fact holding the UTC date and time the session started.
    inline constexpr const char* K_FACT_DATE = "sushitrack.date";
    /// @brief The environment fact holding the profile level compiled into the library.
    inline constexpr const char* K_FACT_PROFILE_LEVEL = "sushitrack.profile_level";
} // namespace SushiTrack::ProfilerKeys
