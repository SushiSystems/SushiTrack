/**************************************************************************/
/* gating_log_fixture.hpp                                                 */
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
 * @file gating_log_fixture.hpp
 * @brief Declares GatingLogFixture, which counts the gating log lines of one cost matrix call.
 * @author Mustafa Garip
 */

#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include <SushiTrack/association/cost_matrix.hpp>
#include <SushiTrack/filtering/kalman_filter.hpp>
#include <SushiTrack/tracking/tracklet.hpp>

#include <captured_log.hpp>

namespace SushiTrack
{
    /**
     * @brief Counts the gating log lines a calculator writes for one track and three far
     *        detections.
     *
     * The line is TRACE, which a release build compiles out, so a call logs at most one line.
     */
    class GatingLogFixture : public ::testing::Test
    {
        protected:
            /// @brief Collects the records of each test.
            std::unique_ptr<CapturedLog> log_ = std::make_unique<CapturedLog>();

            /** @brief Activates and predicts a track so that its Kalman gate is live. */
            static std::shared_ptr<Tracklet> live(std::shared_ptr<Tracklet> track)
            {
                track->activate(std::make_unique<KalmanFilter>(), 1, 1);
                track->predict();
                return track;
            }

            /** @brief Expects three sentinel cells and at most one line, counting all three pairs. */
            void expect_one_line_for_three_gated_pairs(const CostMatrix& m) const
            {
                for (std::size_t j = 0; j < 3; ++j)
                    EXPECT_GE(m(0, j), 1e3f);

                const int lines = log_->count("gated out");
                EXPECT_LE(lines, 1);
                if (lines == 1)
                {
                    const std::string line = log_->last("gated out");
                    EXPECT_NE(line.find("3 of 3"), std::string::npos) << "logged: " << line;
                }
            }
    };
} // namespace SushiTrack
