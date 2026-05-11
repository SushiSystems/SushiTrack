/**
 * @file      InteractionState.h
 * @brief     Scene-level interaction state classification for tracked objects.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      11.05.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#pragma once

namespace SushiTrack
{
    /**
     * @brief Scene-level interaction categories for a tracklet.
     *
     * Captures the relationship between a tracklet and other tracklets in the
     * same frame. This axis is independent of the kinematic state machine: a
     * tracklet has both a kinematic regime (its own motion profile) and an
     * interaction regime (its relationship to neighbouring tracklets), and
     * downstream policies are expected to consume both axes when needed.
     *
     * The enum is deliberately minimal in this version. The Occluding regime
     * (a tracklet that is currently in front of another) is not modelled
     * because the conservative recovery policy only requires knowledge of the
     * occludee side. Additional regimes can be introduced without breaking
     * downstream code provided that Free remains the zero-cost default.
     */
    enum class InteractionState
    {
        Free,      ///< No registered interaction with another tracklet.
        Occluded   ///< Predicted bounding box overlaps with a closer tracklet.
    };

} // namespace SushiTrack
