/**************************************************************************/
/* sushitrack_export.hpp                                                  */
/**************************************************************************/
/*                          This file is part of:                         */
/*                               SushiTrack                               */
/*              https://github.com/SushiSystems/SushiTrack                */
/*                          https://sushisystems.io                       */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                          */
/**************************************************************************/

/**
 * @file sushitrack_export.hpp
 * @brief Defines symbol export and visibility macros for shared library builds.
 * @author Mustafa Garip
 */

#pragma once

#ifndef SUSHITRACK_API
#if defined(_WIN32) || defined(__CYGWIN__)
    #if defined(SUSHITRACK_EXPORTS) || defined(SUSHITRACK_BUILDING_LIBRARY)
        #define SUSHITRACK_API __declspec(dllexport)
    #elif defined(SUSHITRACK_STATIC)
        #define SUSHITRACK_API
    #else
        #define SUSHITRACK_API __declspec(dllimport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define SUSHITRACK_API __attribute__((visibility("default")))
    #else
        #define SUSHITRACK_API
    #endif
#endif
#endif
