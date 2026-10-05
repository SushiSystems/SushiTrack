/****************************************************************/
/* kinematic_state.hpp                                          */
/* SushiTrack - https://github.com/SushiSystems/SushiTrack      */
/* Copyright (c) 2026 Sushi Systems                             */
/* Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.    */
/* Commercial use requires a licence from Sushi Systems.        */
/****************************************************************/

/**
 * @file kinematic_state.hpp
 * @brief Defines motion-regime categories reported per tracklet.
 * @author Mustafa Garip
 */

#pragma once

#include <cstdint>

namespace SushiTrack
{
    /**
     * @brief Discrete motion regime of a tracklet.
     *
     * Classifies motion into stationary, cruising, maneuvering, or coasting
     * regimes derived from IMM mode probabilities or velocity profiles.
     */
    enum class KinematicState : std::uint8_t
    {
        Stationary,  ///< Near-stationary motion.
        Cruising,    ///< Steady, near-constant-velocity motion.
        Maneuvering, ///< Accelerating or turning motion.
        Coasting     ///< No current observation; estimate is being propagated.
    };

} // namespace SushiTrack
