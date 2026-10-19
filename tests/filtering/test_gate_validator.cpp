/**************************************************************************/
/* test_gate_validator.cpp                                                */
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
 * @file test_gate_validator.cpp
 * @brief Tests the GateValidator contract and the default it gives get_gate_sq_distances.
 * @author Mustafa Garip
 */

#include <vector>

#include <gtest/gtest.h>

#include <SushiTrack/filtering/gate_validator.hpp>
#include <SushiTrack/filtering/kalman_filter.hpp>
#include <SushiTrack/filtering/kalman_types.hpp>

#include <measurement_builders.hpp>

using namespace SushiTrack;

namespace
{
    /** @brief Implements only what GateValidator leaves pure, so its defaults are the ones run. */
    class AcceptingValidator final : public GateValidator
    {
        public:
            /// @brief Accepts every measurement.
            [[nodiscard]] bool is_in_gate(const KalmanTypes::StateMean&,
                                          const KalmanTypes::StateCov&,
                                          const KalmanTypes::DetectBox&, float,
                                          float) const override
            {
                return true;
            }

            /// @brief Accepts every measurement.
            [[nodiscard]] std::vector<bool>
            get_gate_flags(const KalmanTypes::StateMean&, const KalmanTypes::StateCov&,
                           const KalmanTypes::DetectBoxVector& measurements, float,
                           const std::vector<float>&) const override
            {
                return std::vector<bool>(measurements.size(), true);
            }
    };
} // namespace

/**
 * @brief The default get_gate_sq_distances gives one zero per measurement.
 */
TEST(Small_GateValidatorTest, DefaultSquaredDistancesAreZeroPerMeasurement)
{
    const AcceptingValidator validator;
    KalmanTypes::DetectBoxVector measurements{make_box(0.f, 0.f, 1.f, 1.f),
                                              make_box(5.f, 5.f, 1.f, 1.f)};

    const std::vector<float> distances = validator.get_gate_sq_distances(
        KalmanTypes::StateMean::Zero(), KalmanTypes::StateCov::Identity(), measurements);

    EXPECT_EQ(distances, (std::vector<float>{0.f, 0.f}));
}

/**
 * @brief Through GateValidator&, a Kalman filter keeps the measurement it was started on and
 *        rejects one far away.
 */
TEST(Small_GateValidatorTest, KalmanGateKeepsTheNearAndRejectsTheFar)
{
    KalmanFilter filter;
    KalmanTypes::StateMean mean;
    KalmanTypes::StateCov covariance;
    const KalmanTypes::DetectBox start = make_box(100.f, 200.f, 50.f, 80.f);
    filter.initiate(mean, covariance, start);
    const GateValidator& validator = filter;

    EXPECT_TRUE(validator.is_in_gate(mean, covariance, start, 9.4877f));
    EXPECT_FALSE(
        validator.is_in_gate(mean, covariance, make_box(5000.f, 200.f, 50.f, 80.f), 9.4877f));
}
