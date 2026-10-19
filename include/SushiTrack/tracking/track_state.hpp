/**************************************************************************/
/* track_state.hpp                                                        */
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
/*                                                                        */
/* Holds code ported from ByteTrack-cpp (MIT), see NOTICE.md              */
/* Copyright (c) 2022 Vertical Beach (lp6m, medalotte)                    */
/* Copyright (c) 2021 Yifu Zhang                                          */
/**************************************************************************/

/**
 * @file track_state.hpp
 * @brief Declares the lifecycle states of a track.
 * @author Mustafa Garip
 */

#pragma once

#include <cstdint>

namespace SushiTrack
{
    /**
     * @brief Internal states for track lifecycle management.
     */
    enum class TrackState : std::uint8_t
    {
        New = 0,      ///< Initial state for detected entities.
        Tracked = 1,  ///< Active state for verified tracks.
        Lost = 2,     ///< State for tracks missing observations within persistence limits.
        Removed = 3,  ///< Terminal state for track termination.
        Tentative = 4 ///< Intermediate state for tracks awaiting confirmation.
    };
} // namespace SushiTrack
