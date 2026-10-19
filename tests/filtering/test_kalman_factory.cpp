/**************************************************************************/
/* test_kalman_factory.cpp                                                */
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
 * @file test_kalman_factory.cpp
 * @brief Tests the KalmanFactory contract: every create gives a new, independent predictor.
 * @author Mustafa Garip
 */

#include <memory>

#include <gtest/gtest.h>

#include <SushiTrack/filtering/generic_kalman_factory.hpp>
#include <SushiTrack/filtering/kalman_factory.hpp>
#include <SushiTrack/filtering/state_predictor.hpp>

using namespace SushiTrack;

/**
 * @brief Through KalmanFactory&, two calls to create give two distinct predictors.
 */
TEST(Small_KalmanFactoryTest, EveryCreateGivesANewPredictor)
{
    const GenericKalmanFactory generic(0);
    const KalmanFactory& factory = generic;

    const std::unique_ptr<StatePredictor> first = factory.create();
    const std::unique_ptr<StatePredictor> second = factory.create();

    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    EXPECT_NE(first.get(), second.get());
}
