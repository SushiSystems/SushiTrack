/**************************************************************************/
/* test_mot_runner.cpp                                                    */
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
 * @file test_mot_runner.cpp
 * @brief Tests run_mot_sequence on a three-frame sequence written to a temporary folder.
 * @author Mustafa Garip
 */

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

#include <mot_runner.hpp>
#include <sushitrack_adapter.hpp>

using namespace MOTEvaluation;

namespace
{
    /** @brief Lays out `<root>/seq/det/det.txt` and `seqinfo.ini` and removes them afterwards. */
    class Medium_MOTRunnerTest : public ::testing::Test
    {
        protected:
            /// @brief Writes a three-frame sequence at 25 fps.
            void SetUp() override
            {
                std::filesystem::create_directories(root_ / "seq" / "det");
                std::ofstream(root_ / "seq" / "det" / "det.txt")
                    << "1,-1,100,100,50,80,0.9\n2,-1,102,100,50,80,0.9\n3,-1,104,100,50,80,0.9\n";
                std::ofstream(root_ / "seq" / "seqinfo.ini") << "frameRate=25\n";
            }

            /// @brief Removes the folder.
            void TearDown() override { std::filesystem::remove_all(root_); }

            /// @brief The temporary data root.
            const std::filesystem::path root_ =
                std::filesystem::temp_directory_path() / "sushitrack_mot_runner_test";
    };
} // namespace

/**
 * @brief Every frame is processed and the predictions land in `pred/<tracker>/<sequence>.txt`.
 */
TEST_F(Medium_MOTRunnerTest, ProcessesEveryFrameAndWritesPredictions)
{
    TrackerBridge::SushiTrackAdapter adapter;
    const RunResult result = run_mot_sequence(root_, "seq", adapter);

    EXPECT_EQ(result.frames_processed, 3);
    EXPECT_GE(result.mean_us(), 0.0);
    EXPECT_TRUE(std::filesystem::exists(root_ / "seq" / "pred" / "sushitrack" / "seq.txt"));
}

/**
 * @brief A sequence with no detections file is refused.
 */
TEST_F(Medium_MOTRunnerTest, MissingDetectionsAreRefused)
{
    TrackerBridge::SushiTrackAdapter adapter;
    EXPECT_THROW(static_cast<void>(run_mot_sequence(root_, "absent", adapter)), std::runtime_error);
}
