/**************************************************************************/
/* default_params.hpp                                                     */
/**************************************************************************/
/*                          This file is part of:                         */
/*                               SushiTrack                               */
/*               https://github.com/SushiSystems/SushiTrack               */
/*                         https://sushisystems.io                        */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License"); you    */
/* may not use this file except in compliance with the License. You may   */
/* obtain a copy of the License at                                        */
/*                                                                        */
/*     https://www.apache.org/licenses/LICENSE-2.0                        */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/**************************************************************************/

/**
 * @file default_params.hpp
 * @brief Declares default_params, the library's default tracker parameters, read from no file.
 * @author Mustafa Garip
 */

#pragma once

#include <SushiTrack/c_api/sushitrack_c.h>

namespace TestSupport
{
    /**
     * @brief Returns the parameters sushitrack_get_default_params fills, which no file or
     *        environment variable changes.
     */
    [[nodiscard]] inline sushitrack_params_t default_params()
    {
        sushitrack_params_t params;
        sushitrack_get_default_params(&params);
        return params;
    }
} // namespace TestSupport
