/**************************************************************************/
/* params_overlay.hpp                                                     */
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
 * @file params_overlay.hpp
 * @brief Declares overlay_params, which reads a configuration document over a parameters struct.
 * @author Mustafa Garip
 */

#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include <SushiTrack/c_api/sushitrack_c.h>

namespace SushiTrack::Detail
{
    /**
     * @brief Applies @p document to @p params, refusing an unknown key, a wrong kind or a fraction.
     *
     * The document is the `sushitrack` object or an object holding it. @p params is untouched
     * when anything is refused.
     *
     * @return The first refusal as `<dotted path>: <reason>`, or an empty string when it applied.
     */
    [[nodiscard]] std::string overlay_params(const nlohmann::json& document,
                                             sushitrack_params_t& params);
} // namespace SushiTrack::Detail
