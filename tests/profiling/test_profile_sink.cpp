/**************************************************************************/
/* test_profile_sink.cpp                                                  */
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
 * @file test_profile_sink.cpp
 * @brief Tests ScopedProfileSink on the thread that installs it.
 * @author Mustafa Garip
 */

#include <SushiTrack/profiling/memory_profile_sink.hpp>
#include <SushiTrack/profiling/profile_sink.hpp>

#include <gtest/gtest.h>

using namespace SushiTrack;

/**
 * @brief With no scope open, the thread has no sink.
 */
TEST(Small_ProfileSinkTest, NoSinkIsActiveOutsideAScope)
{
    EXPECT_EQ(active_profile_sink(), nullptr);
}

/**
 * @brief Inside the scope the installed sink is the active one, and the scope's end removes it.
 */
TEST(Small_ProfileSinkTest, TheInstalledSinkIsActiveForTheScope)
{
    MemoryProfileSink sink;
    {
        const ScopedProfileSink scope(sink);

        EXPECT_EQ(active_profile_sink(), &sink);
    }

    EXPECT_EQ(active_profile_sink(), nullptr);
}

/**
 * @brief A scope inside another makes its sink active, and the end of the inner scope puts the
 *        outer sink back.
 */
TEST(Small_ProfileSinkTest, ANestedScopePutsTheOuterSinkBack)
{
    MemoryProfileSink outer;
    MemoryProfileSink inner;
    const ScopedProfileSink outer_scope(outer);
    {
        const ScopedProfileSink inner_scope(inner);

        EXPECT_EQ(active_profile_sink(), &inner);
    }

    EXPECT_EQ(active_profile_sink(), &outer);
}
