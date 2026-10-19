/**************************************************************************/
/* test_generic_kalman_factory.cpp                                        */
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
 * @file test_generic_kalman_factory.cpp
 * @brief Tests GenericKalmanFactory, which builds state predictors from the registry.
 * @author Mustafa Garip
 */

#include <memory>

#include <gtest/gtest.h>

#include <SushiTrack/filtering/generic_kalman_factory.hpp>
#include <SushiTrack/filtering/imm_filter.hpp>
#include <SushiTrack/filtering/kalman_filter.hpp>
#include <SushiTrack/filtering/nsa_kalman_filter.hpp>
#include <SushiTrack/filtering/state_predictor.hpp>

#include <captured_log.hpp>

using namespace SushiTrack;

/**
 * @brief Types 0, 1 and 2 build a KalmanFilter, an NSAKalmanFilter and an IMMFilter.
 */
TEST(Small_GenericKalmanFactoryTest, BuildsTheRegisteredTypes)
{
    const std::unique_ptr<StatePredictor> linear = GenericKalmanFactory(0).create();
    const std::unique_ptr<StatePredictor> nsa = GenericKalmanFactory(1).create();
    const std::unique_ptr<StatePredictor> imm = GenericKalmanFactory(2).create();

    EXPECT_NE(dynamic_cast<KalmanFilter*>(linear.get()), nullptr);
    EXPECT_NE(dynamic_cast<NSAKalmanFilter*>(nsa.get()), nullptr);
    EXPECT_NE(dynamic_cast<IMMFilter*>(imm.get()), nullptr);
}

/**
 * @brief An unregistered type builds nothing.
 */
TEST(Small_GenericKalmanFactoryTest, UnregisteredTypeBuildsNothing)
{
    const CapturedLog log(SushiLog::LogLevel::Off);
    EXPECT_EQ(GenericKalmanFactory(987654).create(), nullptr);
}
