/**************************************************************************/
/* Helpers.h                                                              */
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
 * @file Helpers.h
 * @brief Provides test fixture helpers and synthetic data generators.
 * @author Mustafa Garip
 */


#ifndef SUSHITRACK_INTEGRATION_HELPERS_H
#define SUSHITRACK_INTEGRATION_HELPERS_H

#include <SushiTrack/sushitrack_c.h>

#include <cmath>
#include <cstddef>
#include <vector>

namespace sushitrack_it
{

/** @brief Track lifecycle states as reported in sushitrack_track_t::state. */
enum TrackStateCode
{
    kNew       = 0,
    kTracked   = 1,
    kLost      = 2,
    kRemoved   = 3,
    kTentative = 4
};

/**
 * @brief Constructs a benign detection without an appearance feature.
 */
inline sushitrack_object_t makeDet(float x,
                                   float y,
                                   float score  = 0.9f,
                                   float width  = 50.0f,
                                   float height = 100.0f,
                                   int   label  = 1)
{
    sushitrack_object_t o{};
    o.x        = x;
    o.y        = y;
    o.width    = width;
    o.height   = height;
    o.score    = score;
    o.label    = label;
    o.feat     = nullptr;
    o.feat_dim = 0;
    return o;
}

/**
 * @brief Constructs a detection backed by an external feature pointer.
 */
inline sushitrack_object_t makeDetWithFeat(float        x,
                                           float        y,
                                           const float* feat,
                                           int          feat_dim,
                                           float        score  = 0.9f,
                                           float        width  = 50.0f,
                                           float        height = 100.0f)
{
    sushitrack_object_t o = makeDet(x, y, score, width, height);
    o.feat             = feat;
    o.feat_dim         = feat_dim;
    return o;
}

/**
 * @brief Locates a track in an output buffer by its persistent ID.
 */
inline const sushitrack_track_t* findById(const std::vector<sushitrack_track_t>& out,
                                          int                                    n,
                                          int64_t                                id)
{
    for (int i = 0; i < n; ++i)
        if (out[static_cast<std::size_t>(i)].track_id == id)
            return &out[static_cast<std::size_t>(i)];
    return nullptr;
}

/**
 * @brief Checks whether geometric fields of track are finite with positive extent.
 */
inline bool geometryIsSane(const sushitrack_track_t& t)
{
    return std::isfinite(t.x) && std::isfinite(t.y) &&
           std::isfinite(t.width) && std::isfinite(t.height) &&
           t.width > 0.0f && t.height > 0.0f;
}

/**
 * @brief Counts tracks in the output that currently report the Tracked state.
 */
inline int countTracked(const std::vector<sushitrack_track_t>& out, int n)
{
    int c = 0;
    for (int i = 0; i < n; ++i)
        if (out[static_cast<std::size_t>(i)].state == kTracked)
            ++c;
    return c;
}

/**
 * @brief Builds an L2-distinct one-hot feature vector along specified axis.
 */
inline std::vector<float> oneHotFeature(int dim, int axis, float magnitude = 1.0f)
{
    std::vector<float> v(static_cast<std::size_t>(dim), 0.0f);
    if (axis >= 0 && axis < dim)
        v[static_cast<std::size_t>(axis)] = magnitude;
    return v;
}

} // namespace sushitrack_it

#endif // SUSHITRACK_INTEGRATION_HELPERS_H
