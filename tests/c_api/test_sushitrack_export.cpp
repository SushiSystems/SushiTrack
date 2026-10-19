/**************************************************************************/
/* test_sushitrack_export.cpp                                             */
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
 * @file test_sushitrack_export.cpp
 * @brief Tests that SUSHITRACK_API is empty in a static build, which every test program is.
 * @author Mustafa Garip
 */

#include <string>

#include <gtest/gtest.h>

#include <SushiTrack/c_api/sushitrack_export.hpp>

/** @brief Turns the expansion of its arguments into a string literal. */
#define SUSHITRACK_TEST_EXPAND(...) SUSHITRACK_TEST_QUOTE(__VA_ARGS__)
/** @brief Quotes its arguments between brackets, so an empty expansion reads "[]". */
#define SUSHITRACK_TEST_QUOTE(...) "[" #__VA_ARGS__ "]"

/**
 * @brief In the static build the tests use, SUSHITRACK_API imports nothing: it is empty on
 *        Windows and only marks visibility elsewhere.
 */
TEST(Small_SushiTrackExportTest, ApiMacroImportsNothingInAStaticBuild)
{
    const std::string expansion = SUSHITRACK_TEST_EXPAND(SUSHITRACK_API);
    EXPECT_EQ(expansion.find("dllimport"), std::string::npos) << expansion;
#if defined(_WIN32) || defined(__CYGWIN__)
    EXPECT_EQ(expansion, "[]");
#endif
}
