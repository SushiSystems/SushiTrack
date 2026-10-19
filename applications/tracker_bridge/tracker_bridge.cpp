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

#include <memory>
#include <string>
#include <vector>

#include <byte_track_adapter.hpp>
#include <mot_detection.hpp>
#include <oc_sort_adapter.hpp>
#include <sushitrack_adapter.hpp>
#include <track_result.hpp>
#include <tracker_adapter.hpp>
#include <tracker_registry.hpp>

namespace
{
    using TrackerBridge::TrackerAdapter;
    using TrackerBridge::TrackerRegistry;

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
            return nullptr;
        return TrackerRegistry::get_instance().create(name).release();
    }

    void delete_tracker(void* tracker)
    {
        delete static_cast<TrackerAdapter*>(tracker);
    }

    void tracker_process_c(void* tracker, const CDetection* detections, int num_detections,
                           CTrackResult* results, int* num_results, int max_results)
    {
        if (tracker == nullptr || results == nullptr || num_results == nullptr)
            return;
        *num_results = 0;
        if (detections == nullptr && num_detections > 0)
            return;

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
    }
}
