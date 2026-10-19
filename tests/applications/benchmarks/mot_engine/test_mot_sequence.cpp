/**************************************************************************/
/* test_mot_sequence.cpp                                                  */
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
 * @file test_mot_sequence.cpp
 * @brief Tests read_mot_detections and read_frame_rate on files written to a temporary folder.
 * @author Mustafa Garip
 */

#include <filesystem>
#include <fstream>
#include <string>

#include <gtest/gtest.h>

#include <mot_sequence.hpp>

using namespace MOTEngine;

namespace
{
    /** @brief Writes @p text to a fresh file in the temporary folder and returns its path. */
    std::filesystem::path write_temp(const std::string& name, const std::string& text)
    {
        const std::filesystem::path path = std::filesystem::temp_directory_path() / name;
        std::ofstream(path) << text;
        return path;
    }
} // namespace

/**
 * @brief Rows group by frame, the box and score come from columns 3 to 7, and a comment, a blank
 *        line and a short row are skipped.
 */
TEST(Medium_MOTSequenceTest, ReadsDetectionsByFrame)
{
    const std::filesystem::path path = write_temp(
        "sushitrack_mot_sequence_det.txt",
        "# comment\n1,-1,10,20,30,40,0.9\n1,-1,50,60,70,80,0.8,2\n\n2,-1,1,2\n3,-1,5,6,7,8,0.7\n");

    const MOTFrames frames = read_mot_detections(path.string());
    std::filesystem::remove(path);

    ASSERT_EQ(frames.size(), 2u);
    ASSERT_EQ(frames.at(1).size(), 2u);
    EXPECT_FLOAT_EQ(frames.at(1)[0].x, 10.f);
    EXPECT_FLOAT_EQ(frames.at(1)[0].score, 0.9f);
    EXPECT_EQ(frames.at(1)[0].label, 1);
    EXPECT_EQ(frames.at(1)[1].label, 2);
    EXPECT_FLOAT_EQ(frames.at(3)[0].h, 8.f);
}

/**
 * @brief The frame rate comes from `frameRate=`; a missing file gives the fallback.
 */
TEST(Medium_MOTSequenceTest, ReadsTheFrameRateOrFallsBack)
{
    const std::filesystem::path path =
        write_temp("sushitrack_mot_sequence_seqinfo.ini", "[Sequence]\nframeRate=25\n");

    EXPECT_EQ(read_frame_rate(path.string(), 30), 25);
    std::filesystem::remove(path);
    EXPECT_EQ(read_frame_rate(path.string(), 30), 30);
}
