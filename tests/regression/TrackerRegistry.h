/**************************************************************************/
/* TrackerRegistry.h                                                      */
/**************************************************************************/
/*                          This file is part of:                         */
/*                               SushiTrack                               */
/*              https://github.com/SushiSystems/SushiTrack                */
/*                          https://sushisystems.io                       */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                          */
/**************************************************************************/

/**
 * @file TrackerRegistry.h
 * @brief Declares the registry of available tracker adapters.
 * @author Mustafa Garip
 */

#pragma once

#include "ITrackerAdapter.h"
#include <string>
#include <unordered_map>
#include <functional>
#include <iostream>
#include <algorithm>
#include <cctype>


using namespace TestInfrastructure;

/**
 * @brief Singleton registry for dynamically registering and instantiating tracking algorithms.
 * 
 * Provides a factory pattern implementation to decouple tracker instantiation from 
 * the core pipeline logic. New algorithms can register themselves at startup.
 */
class TrackerRegistry
{
public:
    using CreatorFunc = std::function<ITrackerAdapter*()>;

    /**
     * @brief Retrieves the singleton instance of the registry.
     * 
     * @return Reference to the TrackerRegistry instance.
     */
    static TrackerRegistry& getInstance()
    {
        static TrackerRegistry instance;
        return instance;
    }

    /**
     * @brief Registers a tracking algorithm constructor.
     * 
     * @param name    Identifier string for the tracking algorithm.
     * @param creator Factory function returning an ITrackerAdapter pointer.
     */
    void registerTracker(const std::string& name, CreatorFunc creator)
    {
        std::string normalized = name;
        std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        registry_[normalized] = creator;
    }

    /**
     * @brief Instantiates a registered tracking algorithm by name.
     * 
     * @param name Identifier string for the requested tracking algorithm.
     * @return Pointer to the allocated ITrackerAdapter, or nullptr if not found.
     */
    ITrackerAdapter* create(const std::string& name)
    {
        std::string normalized = name;
        std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        
        auto it = registry_.find(normalized);
        if (it != registry_.end())
        {
            return it->second();
        }
        std::cerr << "Tracker " << name << " not found in registry!" << std::endl;
        return nullptr;
    }

private:
    /**
     * @brief Private constructor to enforce the Singleton pattern.
     */
    TrackerRegistry() = default;

    std::unordered_map<std::string, CreatorFunc> registry_; ///< Mapping of identifiers to factory functions.
};

/**
 * @def REGISTER_TRACKER(NAME, TYPE)
 * @brief Macro to automatically register a tracker implementation at translation unit load time.
 */
#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)

#define REGISTER_TRACKER(NAME, TYPE)                                                    \
    static const bool CONCAT(_registered_, __LINE__) = []()                             \
    {                                                                                   \
        TrackerRegistry::getInstance().registerTracker(NAME, []() -> ITrackerAdapter*   \
        {                                                                               \
            return new TYPE();                                                          \
        });                                                                             \
        return true;                                                                    \
    }();


// C-API for ctypes interoperability
#ifdef _WIN32
#define EXPORT_API __declspec(dllexport)
#else
#define EXPORT_API __attribute__((visibility("default")))
#endif

extern "C" 
{
    /**
     * @brief Minimalist C-structure representing a 2D bounding box detection.
     */
    struct CDetection
    {
        float x;     ///< Horizontal coordinate.
        float y;     ///< Vertical coordinate.
        float w;     ///< Dimension width.
        float h;     ///< Dimension height.
        float score; ///< Detection confidence.
        int label;   ///< Classification label.
        const float* feat; ///< Appearance feature embedding.
        int feat_dim;      ///< Dimension of the feature vector.
    };

    /**
     * @brief Minimalist C-structure representing a tracking output result.
     */
    struct CTrackResult
    {
        int track_id; ///< Track identity identifier.
        float x;      ///< Horizontal coordinate.
        float y;      ///< Vertical coordinate.
        float w;      ///< Dimension width.
        float h;      ///< Dimension height.
        float score;  ///< Track confidence.
    };

    /**
     * @brief Instantiates a tracking algorithm via the C-API.
     * 
     * @param name Null-terminated string identifying the algorithm.
     * @return Opaque pointer to the ITrackerAdapter instance.
     */
    EXPORT_API ITrackerAdapter* create_tracker(const char* name);

    /**
     * @brief Destroys an ITrackerAdapter instance via the C-API.
     * 
     * @param tracker Opaque pointer to the ITrackerAdapter instance.
     */
    EXPORT_API void delete_tracker(ITrackerAdapter* tracker);

    /**
     * @brief Executes a single update iteration of the tracking algorithm via the C-API.
     * 
     * @param tracker        Opaque pointer to the ITrackerAdapter instance.
     * @param detections     Pointer to an array of CDetection inputs.
     * @param num_detections Number of items in the detections array.
     */
    EXPORT_API void tracker_process_c(ITrackerAdapter* tracker, const CDetection* detections, int num_detections, CTrackResult* results, int* num_results, int max_results);
}
