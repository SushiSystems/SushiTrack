/**
 * @file      TrackerRegistry.cpp
 * @brief     Implementation of the TrackerRegistry and C-API bindings.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      12.05.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#include "TrackerRegistry.h"
#include <vector>

#include "ByteTrackAdapter.h"
#include "SushiTrackAdapter.h"
#include "OCSORTAdapter.h"

REGISTER_TRACKER("ByteTrack", TestInfrastructure::ByteTrackAdapter);
REGISTER_TRACKER("SushiTrack", TestInfrastructure::SushiTrackAdapter);
REGISTER_TRACKER("OCSORT", TestInfrastructure::OCSORTAdapter);

extern "C" 
{

    ITrackerAdapter* create_tracker(const char* name)
    {
        if (!name)
        {
            return nullptr;
        }
        return TrackerRegistry::getInstance().create(std::string(name));
    }

    void delete_tracker(ITrackerAdapter* tracker)
    {
        if (tracker)
        {
            delete tracker;
        }
    }

    void tracker_process_c(ITrackerAdapter* tracker, const CDetection* detections, int num_detections, CTrackResult* results, int* num_results, int max_results)
    {
        if (!tracker || !detections || !results || !num_results)
        {
            return;
        }

        std::vector<MOTDetection> dets;
        dets.reserve(num_detections);
        
        for (int i = 0; i < num_detections; ++i)
        {
            MOTDetection d;
            d.x = detections[i].x;
            d.y = detections[i].y;
            d.w = detections[i].w;
            d.h = detections[i].h;
            d.score = detections[i].score;
            d.label = detections[i].label;
            dets.push_back(d);
        }

        std::vector<TrackResult> cpp_results = tracker->update(dets, 1.0f);

        int count = 0;
        for (size_t i = 0; i < cpp_results.size() && count < max_results; ++i)
        {
            results[i].track_id = cpp_results[i].track_id;
            results[i].x = cpp_results[i].x;
            results[i].y = cpp_results[i].y;
            results[i].w = cpp_results[i].w;
            results[i].h = cpp_results[i].h;
            results[i].score = cpp_results[i].score;
            count++;
        }
        
        *num_results = count;
    }

}
