/**************************************************************************/
/* profile_sink.cpp                                                       */
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
 * @file profile_sink.cpp
 * @brief Defines the thread-local slot of the installed ProfileSink.
 * @author Mustafa Garip
 */

#include <SushiTrack/profiling/profile_sink.hpp>

namespace SushiTrack
{
    namespace
    {
        /// @brief The sink installed on this thread.
        thread_local ProfileSink* t_active_sink = nullptr;
    } // namespace

    ProfileSink* active_profile_sink() noexcept
    {
        return t_active_sink;
    }

    ScopedProfileSink::ScopedProfileSink(ProfileSink& sink) noexcept : previous_(t_active_sink)
    {
        t_active_sink = &sink;
    }

    ScopedProfileSink::~ScopedProfileSink()
    {
        t_active_sink = previous_;
    }
} // namespace SushiTrack
