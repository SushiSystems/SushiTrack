/**************************************************************************/
/* profile_sink.hpp                                                       */
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
 * @file profile_sink.hpp
 * @brief Declares ProfileSink, the port the profiling macros report to, and ScopedProfileSink.
 * @author Mustafa Garip
 * @see docs/design/SUSHITRACK_PROFILING/README.md
 */

#pragma once

#include <SushiTrack/c_api/sushitrack_export.hpp>

namespace SushiTrack
{
    /// @brief How a counter folds the samples of one period.
    enum class ProfileCounterOp
    {
        Add, /**< The samples are summed. */
        Set, /**< The last sample is kept. */
        Max, /**< The largest sample is kept. */
        Min  /**< The smallest sample is kept. */
    };

    /**
     * @brief Receives each zone entered, counter sample and instant the library records, on the
     *        thread that installed it.
     *
     * The macros of profiling.hpp report to the installed sink beside SushiProfiler, so a test
     * reads what a code path recorded without a session, a ring or a consumer thread. The
     * in-memory adapter is MemoryProfileSink.
     */
    class ProfileSink
    {
        public:
            virtual ~ProfileSink() = default;

            /// @brief A zone named @p name of group @p group was entered.
            virtual void zone(const char* name, const char* group) = 0;

            /// @brief A counter named @p name took @p value, to be folded by @p op.
            virtual void counter(const char* name, ProfileCounterOp op, double value) = 0;

            /// @brief An instant named @p name occurred.
            virtual void instant(const char* name) = 0;
    };

    /// @brief Returns the sink installed on the calling thread, or null when none is.
    SUSHITRACK_API ProfileSink* active_profile_sink() noexcept;

    /**
     * @brief Installs a sink on the calling thread for its lifetime and puts the previous one back.
     *
     * The sink is thread-local, so code that runs on another thread sees no sink. With none
     * installed the macros report to nothing and behave as without this port.
     */
    class SUSHITRACK_API ScopedProfileSink final
    {
        public:
            /// @brief Installs @p sink on the calling thread.
            explicit ScopedProfileSink(ProfileSink& sink) noexcept;

            ScopedProfileSink(const ScopedProfileSink&) = delete;
            ScopedProfileSink& operator=(const ScopedProfileSink&) = delete;
            ScopedProfileSink(ScopedProfileSink&&) = delete;
            ScopedProfileSink& operator=(ScopedProfileSink&&) = delete;

            /// @brief Puts back the sink that was installed before.
            ~ScopedProfileSink();

        private:
            /// @brief The sink installed when this one took over.
            ProfileSink* previous_;
    };
} // namespace SushiTrack
