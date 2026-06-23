/**
 * @file      Helpers.h
 * @brief     Shared fixtures and helpers for the end-to-end integration suite.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      15.06.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 *
 * These helpers drive the tracker exclusively through the public C API
 * (SushiTrackC.h), exactly as a downstream FFI consumer would. They are kept
 * header-only and inline so each translation unit in the integration target can
 * include them without an extra link dependency.
 *
 * Assertion philosophy
 * --------------------
 * Integration assertions check *observable contract* and *structural
 * invariants* — identity persistence, lifecycle state, output capacity,
 * finiteness — not the exact filtered geometry. The Kalman filter deliberately
 * blends prediction with measurement, so the reported box lags a raw detection
 * by (1 - K) * innovation; asserting equality with the raw detection coordinate
 * tests "did the filter ignore its own prediction", which is never the property
 * under test. Where position matters, prefer "is the track on the correct side"
 * (closer to A than B) over "equals A".
 */

#ifndef SUSHITRACK_INTEGRATION_HELPERS_H
#define SUSHITRACK_INTEGRATION_HELPERS_H

#include "SushiTrackC.h"

#include <cmath>
#include <cstddef>
#include <vector>

namespace sushitrack_it
{

/** @brief Track lifecycle states as reported in SushiTrack_Track::state. */
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
 *
 * Defaults model a pedestrian-sized box (50x100) at high confidence.
 */
inline SushiTrack_Object makeDet(float x,
                                float y,
                                float score  = 0.9f,
                                float width  = 50.0f,
                                float height = 100.0f,
                                int   label  = 1)
{
    SushiTrack_Object o{};
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
 *        The caller owns the feature memory; it must outlive the Update call.
 */
inline SushiTrack_Object makeDetWithFeat(float        x,
                                        float        y,
                                        const float* feat,
                                        int          feat_dim,
                                        float        score  = 0.9f,
                                        float        width  = 50.0f,
                                        float        height = 100.0f)
{
    SushiTrack_Object o = makeDet(x, y, score, width, height);
    o.feat             = feat;
    o.feat_dim         = feat_dim;
    return o;
}

/**
 * @brief Locates a track in an output buffer by its persistent ID.
 * @return Pointer into @p out for the matching track, or nullptr.
 */
inline const SushiTrack_Track* findById(const std::vector<SushiTrack_Track>& out,
                                       int                                 n,
                                       int64_t                             id)
{
    for (int i = 0; i < n; ++i)
        if (out[static_cast<std::size_t>(i)].track_id == id)
            return &out[static_cast<std::size_t>(i)];
    return nullptr;
}

/**
 * @brief Returns true when every geometric field of @p t is finite and the
 *        box has strictly positive extent. The single invariant every track
 *        must satisfy on every frame regardless of configuration.
 */
inline bool geometryIsSane(const SushiTrack_Track& t)
{
    return std::isfinite(t.x) && std::isfinite(t.y) &&
           std::isfinite(t.width) && std::isfinite(t.height) &&
           t.width > 0.0f && t.height > 0.0f;
}

/**
 * @brief Counts tracks in the output that currently report the Tracked state.
 */
inline int countTracked(const std::vector<SushiTrack_Track>& out, int n)
{
    int c = 0;
    for (int i = 0; i < n; ++i)
        if (out[static_cast<std::size_t>(i)].state == kTracked)
            ++c;
    return c;
}

/**
 * @brief Builds an L2-distinct one-hot feature vector along @p axis.
 *
 * Two vectors produced with different axes are exactly orthogonal, so their
 * cosine similarity is zero — the cleanest possible appearance separation for
 * a ReID test.
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
