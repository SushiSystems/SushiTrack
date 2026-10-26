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
 * @file tracker_bridge.cpp
 * @brief Registers the three adapters and defines the C entry points of tracker_bridge.
 * @author Mustafa Garip
 */

#include <tracker_bridge.h>

#include <exception>
#include <memory>
#include <string>
#include <vector>

#include <SushiTrack/logging/logger.hpp>

#include <byte_track_adapter.hpp>
#include <mot_detection.hpp>
#include <oc_sort_adapter.hpp>
#include <sushitrack_adapter.hpp>
#include <track_result.hpp>
#include <tracker_adapter.hpp>
#include <tracker_bridge_log_category.hpp>
#include <tracker_registry.hpp>

namespace
{
    using TrackerBridge::K_TRACKER_BRIDGE_LOG_CATEGORY;
    using TrackerBridge::TrackerAdapter;
    using TrackerBridge::TrackerRegistry;

    /**
     * @brief Returns the registered names joined by commas, for a refusal that lists them.
     */
    std::string registered_list()
    {
        std::string list;
        for (const std::string& name : TrackerRegistry::get_instance().registered_names())
            list += (list.empty() ? "" : ", ") + name;
        return list;
    }

    /// @brief Registers the adapters before the first create_tracker call.
    const bool K_REGISTERED = []
    {
        TrackerRegistry& registry = TrackerRegistry::get_instance();
        registry.register_tracker("sushitrack",
                                  []
                                  {
                                      return std::make_unique<TrackerBridge::SushiTrackAdapter>();
                                  });
        registry.register_tracker("bytetrack",
                                  []
                                  {
                                      return std::make_unique<TrackerBridge::ByteTrackAdapter>();
                                  });
        registry.register_tracker("ocsort",
                                  []
                                  {
                                      return std::make_unique<TrackerBridge::OCSORTAdapter>();
                                  });
        return true;
    }();
} // namespace

extern "C"
{
    void* create_tracker(const char* name)
    {
        if (name == nullptr || !K_REGISTERED)
        {
            SUSHITRACK_LOG_ERROR(K_TRACKER_BRIDGE_LOG_CATEGORY,
                                 "create_tracker refused: the name is null; no tracker was "
                                 "built.");
            return nullptr;
        }
        try
        {
            std::unique_ptr<TrackerAdapter> tracker = TrackerRegistry::get_instance().create(name);
            if (tracker == nullptr)
            {
                SUSHITRACK_LOG_ERROR(K_TRACKER_BRIDGE_LOG_CATEGORY,
                                     "create_tracker refused: {} is not registered (known: {}); "
                                     "no tracker was built.",
                                     name, registered_list());
            }
            return tracker.release();
        }
        catch (const std::exception& error)
        {
            SUSHITRACK_LOG_ERROR(K_TRACKER_BRIDGE_LOG_CATEGORY,
                                 "create_tracker failed for {}: {}; no tracker was built.", name,
                                 error.what());
            return nullptr;
        }
        catch (...)
        {
            SUSHITRACK_LOG_ERROR(K_TRACKER_BRIDGE_LOG_CATEGORY,
                                 "create_tracker failed for {}: unknown exception; no tracker "
                                 "was built.",
                                 name);
            return nullptr;
        }
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
