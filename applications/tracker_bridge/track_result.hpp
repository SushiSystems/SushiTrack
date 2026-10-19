/**************************************************************************/
/* track_result.hpp                                                       */
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
 * @file track_result.hpp
 * @brief Declares TrackResult, the track every tracker adapter hands back.
 * @author Mustafa Garip
 */

#pragma once

namespace TrackerBridge
{
    /**
     * @brief Holds one confirmed track of a frame: its identity, box and score.
     */
    struct TrackResult
    {
            int track_id = 0;  /**< Track identity. */
            float x = 0.f;     /**< Left edge. */
            float y = 0.f;     /**< Top edge. */
            float w = 0.f;     /**< Width. */
            float h = 0.f;     /**< Height. */
            float score = 0.f; /**< Track confidence. */
    };
} // namespace TrackerBridge
