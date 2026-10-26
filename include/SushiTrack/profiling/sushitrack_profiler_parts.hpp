/**************************************************************************/
/* sushitrack_profiler_parts.hpp                                          */
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
 * @file sushitrack_profiler_parts.hpp
 * @brief Declares register_profiler_parts, which adds SushiTrack's facts to a registry.
 * @author Mustafa Garip
 * @see docs/design/SUSHITRACK_PROFILING/README.md
 */

#pragma once

#include <memory>

#include <SushiTrack/c_api/sushitrack_export.hpp>

namespace SushiProfiler
{
    class Registry;
} // namespace SushiProfiler

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif
namespace SushiTrack
{
    /**
     * @brief Owns what SushiTrack registers with a SushiProfiler registry.
     *
     * Neither copyable nor movable, because the registry holds the address of what this owns; the
     * caller keeps it alive until every session started with the registry has stopped. Built
     * without SushiProfiler it registers nothing.
     */
    class SUSHITRACK_API SushiTrackProfilerParts final
    {
        public:
            /// @brief Registers the library's environment contributor in @p registry.
            explicit SushiTrackProfilerParts(SushiProfiler::Registry& registry);

            SushiTrackProfilerParts(const SushiTrackProfilerParts&) = delete;
            SushiTrackProfilerParts& operator=(const SushiTrackProfilerParts&) = delete;
            SushiTrackProfilerParts(SushiTrackProfilerParts&&) = delete;
            SushiTrackProfilerParts& operator=(SushiTrackProfilerParts&&) = delete;

            /// @brief Releases what the constructor registered.
            ~SushiTrackProfilerParts();

        private:
            /// @brief Holds the contributor the registry refers to; defined in the library.
            struct State;

            /// @brief The contributor, kept behind a pointer so no SushiProfiler type is named here.
            std::unique_ptr<State> state_;
    };

    /**
     * @brief Adds everything SushiTrack offers to @p registry and returns what owns it.
     *
     * The caller binds the result to a variable that outlives every session started with
     * @p registry.
     */
    [[nodiscard]] SUSHITRACK_API SushiTrackProfilerParts
    register_profiler_parts(SushiProfiler::Registry& registry);
} // namespace SushiTrack
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
