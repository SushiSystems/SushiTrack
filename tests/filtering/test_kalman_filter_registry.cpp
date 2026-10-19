/**************************************************************************/
/* test_kalman_filter_registry.cpp                                        */
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
 * @file test_kalman_filter_registry.cpp
 * @brief Tests KalmanFilterRegistry.
 * @author Mustafa Garip
 */

#include <cmath>
#include <string>

#include <Eigen/Dense>
#include <gtest/gtest.h>

#include <SushiTrack/filtering/gated_state_predictor.hpp>
#include <SushiTrack/filtering/kalman_filter_registry.hpp>
#include <SushiTrack/filtering/kalman_types.hpp>

#include <captured_log.hpp>

using namespace SushiTrack;

using StateMean = KalmanTypes::StateMean;

using StateCov = KalmanTypes::StateCov;

using DetectBox = KalmanTypes::DetectBox;

// KalmanFilterRegistry

/**
 * @brief An unregistered type yields no filter and one ERROR line naming the type.
 */
TEST(Small_KalmanFilterRegistryTest, UnregisteredTypeLogsTheTypeAndReturnsNull)
{
    std::unique_ptr<GatedStatePredictor> filter;
    std::string logged;
    {
        CapturedLog log(SushiLog::LogLevel::Error);
        filter = KalmanFilterRegistry::get_instance().create(987654, false);
        logged = log.last("Unregistered Kalman type requested");
    }

    EXPECT_EQ(filter, nullptr);
    EXPECT_NE(logged.find("Unregistered Kalman type requested: 987654"), std::string::npos)
        << "logged: " << logged;
}
