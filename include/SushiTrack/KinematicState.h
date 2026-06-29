/**
 * @file      KinematicState.h
 * @brief     Motion-regime categories reported per track.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      29.06.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#pragma once

namespace SushiTrack
{
    /**
     * @brief Discrete motion regime of a track.
     *
     * Derived from the IMM mode probabilities (see IMMFilter): the
     * near-constant-position mass maps to Stationary, the constant-acceleration
     * and coordinated-turn mass to Maneuvering, the remainder to Cruising, and a
     * track without an observation reports Coasting. Consumed by the
     * association cost shaping, lost-buffer scaling and the C API output as a
     * single regime contract.
     */
    enum class KinematicState
    {
        Stationary,  ///< Near-stationary motion.
        Cruising,    ///< Steady, near-constant-velocity motion.
        Maneuvering, ///< Accelerating or turning motion.
        Coasting     ///< No current observation; estimate is being propagated.
    };

} // namespace SushiTrack
