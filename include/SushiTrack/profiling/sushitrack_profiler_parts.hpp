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

#include <SushiTrack/profiling/profiling.hpp>

#if SUSHITRACK_ENABLE_PROFILER

#include <SushiProfiler/environment/environment_contributor.hpp>
#include <SushiProfiler/registry/registry.hpp>
#include <SushiProfiler/version/export.hpp>

#include <SushiTrack/c_api/sushitrack_export.hpp>

SUSHIPROFILER_DLL_WARNINGS_PUSH
namespace SushiTrack
{
    /**
     * @brief Adds the library's name, version, commit, profile level and the run's UTC date to
     *        a record's environment.
     */
    class SUSHITRACK_API SushiTrackEnvironment final : public SushiProfiler::EnvironmentContributor
    {
        public:
            /// @brief Sets the `sushitrack.*` facts of ProfilerKeys in @p facts.
            void contribute(SushiProfiler::RecordFacts& facts) const override;
    };

    /**
     * @brief Owns what SushiTrack registers with a SushiProfiler registry.
     *
     * Neither copyable nor movable, because the registry holds the contributor's address; the
     * caller keeps it alive until every session started with the registry has stopped.
     */
    class SUSHITRACK_API SushiTrackProfilerParts final
    {
        public:
            /// @brief Registers the environment contributor in @p registry.
            explicit SushiTrackProfilerParts(SushiProfiler::Registry& registry);

            SushiTrackProfilerParts(const SushiTrackProfilerParts&) = delete;
            SushiTrackProfilerParts& operator=(const SushiTrackProfilerParts&) = delete;
            SushiTrackProfilerParts(SushiTrackProfilerParts&&) = delete;
            SushiTrackProfilerParts& operator=(SushiTrackProfilerParts&&) = delete;
            ~SushiTrackProfilerParts() = default;

        private:
            /// @brief The contributor the registry refers to.
            SushiTrackEnvironment environment_;
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
SUSHIPROFILER_DLL_WARNINGS_POP

#endif
