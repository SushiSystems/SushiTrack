/****************************************************************/
/* UnitTest.cpp                                                 */
/* SushiTrack - https://github.com/SushiSystems/SushiTrack      */
/* Copyright (c) 2026 Sushi Systems                             */
/* Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.    */
/* Commercial use requires a licence from Sushi Systems.        */
/****************************************************************/

/**
 * @file UnitTest.cpp
 * @brief Provides the entry point for the unit test executable.
 * @author Mustafa Garip
 */

#include <gtest/gtest.h>

/**
 * @brief Program entry point for the unit test binary.
 * @param argc Argument count forwarded to GoogleTest.
 * @param argv Argument vector forwarded to GoogleTest.
 * @return 0 on success, non-zero if any test fails.
 */
int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
