/**************************************************************************/
/* test_memory_profile_sink.cpp                                           */
/**************************************************************************/
/*                          This file is part of:                         */
/*                               SushiTrack                               */
/*               https://github.com/SushiSystems/SushiTrack               */
/*                         https://sushisystems.io                        */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License"); you    */
/* may not use this file except in compliance with the License. You may   */
/* obtain a copy of the License at                                        */
/*                                                                        */
/*     https://www.apache.org/licenses/LICENSE-2.0                        */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/**************************************************************************/

/**
 * @file test_memory_profile_sink.cpp
 * @brief Tests MemoryProfileSink: the frames it groups events by and the way it folds counters.
 * @author Mustafa Garip
 */

#include <SushiTrack/profiling/memory_profile_sink.hpp>

#include <optional>

#include <gtest/gtest.h>

using namespace SushiTrack;

namespace
{
    /// @brief One counter and how the samples 2, 7 and 3 fold under its operation.
    struct FoldRow
    {
            ProfileCounterOp op;
            double folded;
    };
} // namespace

/**
 * @brief Fixture of the folding rows.
 */
class Small_MemoryProfileSinkFoldTest : public ::testing::TestWithParam<FoldRow>
{
};

/**
 * @brief The samples 2, 7 and 3 of one frame fold as the operation says: Add 12, Set 3 (the
 *        last), Max 7, Min 2.
 */
TEST_P(Small_MemoryProfileSinkFoldTest, FoldsTheSamplesOfAFrameByItsOperation)
{
    MemoryProfileSink sink;
    sink.begin_frame();
    sink.counter("c", GetParam().op, 2.0);
    sink.counter("c", GetParam().op, 7.0);
    sink.counter("c", GetParam().op, 3.0);

    EXPECT_EQ(sink.counter_value(0U, "c"), std::optional<double>(GetParam().folded));
}

INSTANTIATE_TEST_SUITE_P(Operations, Small_MemoryProfileSinkFoldTest,
                         ::testing::Values(FoldRow{ProfileCounterOp::Add, 12.0},
                                           FoldRow{ProfileCounterOp::Set, 3.0},
                                           FoldRow{ProfileCounterOp::Max, 7.0},
                                           FoldRow{ProfileCounterOp::Min, 2.0}));

/**
 * @brief A counter that took no sample in a frame has no value there.
 */
TEST(Small_MemoryProfileSinkTest, ACounterWithNoSampleInAFrameHasNoValue)
{
    MemoryProfileSink sink;
    sink.begin_frame();
    sink.counter("c", ProfileCounterOp::Set, 5.0);
    sink.begin_frame();

    EXPECT_EQ(sink.counter_value(1U, "c"), std::nullopt);
}

/**
 * @brief Zones count once per frame that entered them, however often a frame did.
 */
TEST(Small_MemoryProfileSinkTest, CountsTheFramesThatEnteredAZone)
{
    MemoryProfileSink sink;
    sink.begin_frame();
    sink.zone("z", "g");
    sink.zone("z", "g");
    sink.begin_frame();
    sink.begin_frame();
    sink.zone("z", "g");

    EXPECT_EQ(sink.frame_count(), 3U);
    EXPECT_EQ(sink.frames_holding_zone("z"), 2U);
    EXPECT_EQ(sink.frames_holding_zone("other"), 0U);
}

/**
 * @brief Instants are kept per frame.
 */
TEST(Small_MemoryProfileSinkTest, KeepsInstantsPerFrame)
{
    MemoryProfileSink sink;
    sink.begin_frame();
    sink.instant("i");
    sink.instant("i");
    sink.begin_frame();
    sink.instant("i");

    EXPECT_EQ(sink.instant_count(0U, "i"), 2U);
    EXPECT_EQ(sink.instant_count(1U, "i"), 1U);
    EXPECT_EQ(sink.instant_count(1U, "none"), 0U);
}

/**
 * @brief Events that arrive before any begin_frame form the first frame.
 */
TEST(Small_MemoryProfileSinkTest, EventsBeforeTheFirstFrameFormTheFirstFrame)
{
    MemoryProfileSink sink;
    sink.zone("z", "g");

    EXPECT_EQ(sink.frame_count(), 1U);
    EXPECT_EQ(sink.frames_holding_zone("z"), 1U);
}
