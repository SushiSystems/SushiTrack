/**************************************************************************/
/* tracker_bridge.cpp                                                     */
/**************************************************************************/
/*                          This file is part of:                         */
/*                               SushiTrack                               */
/*               https://github.com/SushiSystems/SushiTrack               */
/*                         https://sushisystems.io                        */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License"); you    */
/* may not use this file except in compliance with the License. You may   */
/* obtain a copy of the License at                                        */
/*                                                                        */
/*     https://www.apache.org/licenses/LICENSE-2.0                        */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/**************************************************************************/

/**
 * @file tracker_bridge.cpp
 * @brief Registers the three adapters and defines the C entry points of tracker_bridge.
 * @author Mustafa Garip
 */

#include <tracker_bridge.h>

#include <exception>
#include <vector>

#include <SushiTrack/logging/logger.hpp>

#include <default_trackers.hpp>
#include <mot_detection.hpp>
#include <track_result.hpp>
#include <tracker_adapter.hpp>
#include <tracker_bridge_log_category.hpp>
#include <tracker_builder.hpp>
#include <tracker_registry.hpp>

namespace
{
    using TrackerBridge::K_TRACKER_BRIDGE_LOG_CATEGORY;
    using TrackerBridge::TrackerAdapter;
    using TrackerBridge::TrackerRegistry;

    /// @brief Registers the adapters before the first create_tracker call.
    const bool K_REGISTERED = []
    {
        TrackerBridge::register_default_trackers(TrackerRegistry::get_instance());
        return true;
    }();
} // namespace

extern "C"
{
    void* create_tracker(const char* name)
    {
        if (!K_REGISTERED)
            return nullptr;
        return TrackerBridge::build_tracker(TrackerRegistry::get_instance(), name).release();
    }

    void delete_tracker(void* tracker)
    {
        delete static_cast<TrackerAdapter*>(tracker);
    }

    tracker_bridge_status_t tracker_process_c(void* tracker, const CDetection* detections,
                                              int num_detections, CTrackResult* results,
                                              int* num_results, int max_results)
    {
        if (tracker == nullptr || results == nullptr || num_results == nullptr)
        {
            SUSHITRACK_LOG_ERROR(K_TRACKER_BRIDGE_LOG_CATEGORY,
                                 "tracker_process_c refused: tracker, results or num_results is "
                                 "null; the frame was not run.");
            return TRACKER_BRIDGE_INVALID_ARGUMENT;
        }
        *num_results = 0;
        if ((detections == nullptr && num_detections > 0) || num_detections < 0 ||
            max_results < 0)
        {
            SUSHITRACK_LOG_ERROR(K_TRACKER_BRIDGE_LOG_CATEGORY,
                                 "tracker_process_c refused: num_detections is {} with detections "
                                 "{} and max_results is {}; the frame was not run.",
                                 num_detections, detections ? "set" : "null", max_results);
            return TRACKER_BRIDGE_INVALID_ARGUMENT;
        }

        try
        {
            std::vector<TrackerBridge::MOTDetection> dets;
            dets.reserve(static_cast<size_t>(num_detections));
            for (int i = 0; i < num_detections; ++i)
            {
                const CDetection& d = detections[i];
                TrackerBridge::MOTDetection det{d.x, d.y, d.w, d.h, d.score, d.label, {}};
                if (d.feat != nullptr && d.feat_dim > 0)
                    det.feature.assign(d.feat, d.feat + d.feat_dim);
                dets.push_back(std::move(det));
            }

            const std::vector<TrackerBridge::TrackResult> tracks =
                static_cast<TrackerAdapter*>(tracker)->update(dets, 1.0f);
            int count = 0;
            for (const TrackerBridge::TrackResult& t : tracks)
            {
                if (count >= max_results)
                    break;
                results[count++] = {t.track_id, t.x, t.y, t.w, t.h, t.score};
            }
            *num_results = count;
            if (static_cast<size_t>(count) < tracks.size())
            {
                SUSHITRACK_LOG_WARN(K_TRACKER_BRIDGE_LOG_CATEGORY,
                                    "tracker_process_c wrote {} of {} tracks; max_results is {}.",
                                    count, tracks.size(), max_results);
                return TRACKER_BRIDGE_TRUNCATED;
            }
            return TRACKER_BRIDGE_OK;
        }
        catch (const std::exception& error)
        {
            SUSHITRACK_LOG_ERROR(K_TRACKER_BRIDGE_LOG_CATEGORY,
                                 "tracker_process_c failed on {} detections: {}; no track was "
                                 "written.",
                                 num_detections, error.what());
        }
        catch (...)
        {
            SUSHITRACK_LOG_ERROR(K_TRACKER_BRIDGE_LOG_CATEGORY,
                                 "tracker_process_c failed on {} detections: unknown exception; "
                                 "no track was written.",
                                 num_detections);
        }
        *num_results = 0;
        return TRACKER_BRIDGE_ERROR;
    }
}
