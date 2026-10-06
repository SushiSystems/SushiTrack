/**************************************************************************/
/* sushitrack_export.hpp                                                  */
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
