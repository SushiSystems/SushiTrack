/**************************************************************************/
/* profile_session.hpp                                                    */
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
 * @file profile_session.hpp
 * @brief Declares ProfiledFrames, which runs frames of work under a SushiProfiler session for a
 *        test and reads the zones and counters back from the record.
 * @author Mustafa Garip
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include <SushiProfiler/config/profiler_config.hpp>
#include <SushiProfiler/periods/period_timeline.hpp>
#include <SushiProfiler/record/run_record.hpp>
#include <SushiProfiler/registry/registry.hpp>
#include <SushiProfiler/session/session.hpp>

#include <SushiTrack/profiling/profiler_keys.hpp>
#include <SushiTrack/profiling/sushitrack_profiler_parts.hpp>

namespace SushiTrack
{
    /**
     * @brief Starts a session that records @p frames periods of `sushitrack.frame`, marks one
     *        per frame() call and returns the record from finish().
     */
    class ProfiledFrames final
    {
        public:
            /// @brief Starts the session at runtime level @p level for @p frames frames.
            explicit ProfiledFrames(std::size_t frames, std::uint8_t level = 2U)
                : parts_(register_profiler_parts(registry_))
            {
                SushiProfiler::ProfilerConfig config;
                config.timeline = ProfilerKeys::K_TIMELINE_FRAME;
                config.level = level;
                config.window.warmup = 0U;
                config.window.active = static_cast<std::uint32_t>(frames);
                config.buffers.linger_ms = 0U;
                SushiProfiler::SessionStart started =
                    SushiProfiler::Session::start(config, registry_);
                error_ = started.error;
                session_ = std::move(started.session);
                if (session_)
                    timeline_ = &session_->timeline(ProfilerKeys::K_TIMELINE_FRAME);
            }

            ProfiledFrames(const ProfiledFrames&) = delete;
            ProfiledFrames& operator=(const ProfiledFrames&) = delete;
            ProfiledFrames(ProfiledFrames&&) = delete;
            ProfiledFrames& operator=(ProfiledFrames&&) = delete;
            ~ProfiledFrames() = default;

            /// @brief Returns whether the session started.
            [[nodiscard]] bool ok() const noexcept
            {
                return session_ != nullptr;
            }

            /// @brief Returns why the session did not start, or an empty string.
            [[nodiscard]] const std::string& error() const noexcept
            {
                return error_;
            }

            /// @brief Opens the next period and runs @p work inside it.
            void frame(const std::function<void()>& work)
            {
                timeline_->mark();
                work();
            }

            /// @brief Closes the last period, stops the session and returns the record.
            [[nodiscard]] SushiProfiler::RunRecord finish()
            {
                timeline_->mark();
                return session_->stop().record;
            }

        private:
            /// @brief Where the parts register.
            SushiProfiler::Registry registry_;
            /// @brief SushiTrack's registered parts.
            SushiTrackProfilerParts parts_;
            /// @brief The running session.
            std::unique_ptr<SushiProfiler::Session> session_;
            /// @brief The frame timeline.
            SushiProfiler::PeriodTimeline* timeline_ = nullptr;
            /// @brief Why the session did not start.
            std::string error_;
    };

    /// @brief Returns the names of the zones @p period holds.
    inline std::set<std::string> zone_names(const SushiProfiler::RunRecord& record,
                                            const SushiProfiler::PeriodRecord& period)
    {
        std::set<std::string> names;
        for (const SushiProfiler::ZoneTree& tree : period.zones)
        {
            for (const SushiProfiler::ZoneNode& node : tree.nodes)
                names.insert(record.sites[node.site].name);
        }
        return names;
    }

    /**
     * @brief Returns what counter @p name folded to in @p period, or -1 when it recorded nothing.
     *
     * Add sums the samples; Set takes the last one.
     */
    inline double counter_value(const SushiProfiler::RunRecord& record,
                                const SushiProfiler::PeriodRecord& period,
                                const std::string& name)
    {
        double folded = -1.0;
        bool seen = false;
        for (const SushiProfiler::SampleSeries& series : period.samples)
        {
            const SushiProfiler::SiteRecord& site = record.sites[series.site];
            if (site.name != name || series.values.empty())
                continue;
            if (site.op == SushiProfiler::CounterOp::Add)
            {
                folded = seen ? folded : 0.0;
                for (const double value : series.values)
                    folded += value;
            }
            else
            {
                folded = series.values.back();
            }
            seen = true;
        }
        return folded;
    }

    /// @brief Returns how many instants named @p name @p period holds.
    inline std::size_t instant_count(const SushiProfiler::RunRecord& record,
                                     const SushiProfiler::PeriodRecord& period,
                                     const std::string& name)
    {
        std::size_t count = 0U;
        for (const SushiProfiler::SampleSeries& series : period.samples)
        {
            if (record.sites[series.site].name == name)
                count += series.time_ns.size();
        }
        return count;
    }
} // namespace SushiTrack
