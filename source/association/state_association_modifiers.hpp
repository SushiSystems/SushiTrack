/**************************************************************************/
/* state_association_modifiers.hpp                                        */
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
 * @file state_association_modifiers.hpp
 * @brief Declares the association modifiers each kinematic state applies.
 * @author Mustafa Garip
 */

#pragma once

#include <SushiTrack/tracking/kinematic_state.hpp>

namespace SushiTrack
{
    /**
     * @brief Per-state association policy modifiers.
     */
    struct StateAssocMods
    {
            float gate_scale;
            float reid_shift;
            bool require_reid;
    };

    inline StateAssocMods state_assoc_mods(KinematicState s)
    {
        switch (s)
        {
            case KinematicState::Stationary: return {0.6f, -0.2f, false};
            case KinematicState::Maneuvering: return {1.5f, 0.2f, false};
            case KinematicState::Coasting: return {2.0f, 0.4f, true};
            case KinematicState::Cruising:
            default: return {1.0f, 0.0f, false};
        }
    }
} // namespace SushiTrack
