/****************************************************************/
/* sushitrack_export.hpp                                        */
/* SushiTrack - https://github.com/SushiSystems/SushiTrack      */
/* Copyright (c) 2026 Sushi Systems                             */
/* Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.    */
/* Commercial use requires a licence from Sushi Systems.        */
/****************************************************************/

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
