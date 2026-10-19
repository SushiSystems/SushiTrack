/**************************************************************************/
/* tracklet_builders.hpp                                                  */
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
 * @file tracklet_builders.hpp
 * @brief Declares make_t and make_feat_t, which build the tracklets the cost calculator tests use.
 * @author Mustafa Garip
 */

#pragma once

#include <memory>
#include <utility>
#include <vector>

#include <SushiTrack/detection/rect.hpp>
#include <SushiTrack/filtering/kalman_filter.hpp>
#include <SushiTrack/tracking/tracklet.hpp>

namespace SushiTrack
{
    /** @brief Returns a Tracklet with TLWH (x, y, w, h) at score @p score. */
    inline std::shared_ptr<Tracklet> make_t(float x, float y, float w, float h, float score = 0.9f,
                                            std::vector<float> feat = {})
    {
        return std::make_shared<Tracklet>(std::make_unique<KalmanFilter>(), Rect<float>(x, y, w, h),
                                          1, score, std::move(feat));
    }

    /** @brief Returns a 50 x 80 Tracklet whose feature is the unit vector along @p axis. */
    inline std::shared_ptr<Tracklet> make_feat_t(float x, float y, int feat_dim, int axis,
                                                 float score = 0.9f)
    {
        std::vector<float> feat(feat_dim, 0.f);
        feat[axis] = 1.f;
        return make_t(x, y, 50.f, 80.f, score, std::move(feat));
    }
} // namespace SushiTrack
