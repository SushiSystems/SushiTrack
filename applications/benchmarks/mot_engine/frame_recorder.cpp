/**************************************************************************/
/* frame_recorder.cpp                                                     */
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
 * @file frame_recorder.cpp
 * @brief Defines FrameRecorder over a SushiProfiler session, or as a no-op without the profiler.
 * @author Mustafa Garip
 */

#include <frame_recorder.hpp>

#include <SushiTrack/profiling/profile_config.hpp>

#if SUSHITRACK_ENABLE_PROFILER

#include <algorithm>
#include <fstream>
#include <limits>
#include <utility>

#include <SushiProfiler/config/profiler_config.hpp>
#include <SushiProfiler/environment/environment_contributor.hpp>
#include <SushiProfiler/periods/period_timeline.hpp>
#include <SushiProfiler/record_json/record_json.hpp>
#include <SushiProfiler/registry/registry.hpp>
#include <SushiProfiler/session/session.hpp>

#include <SushiTrack/logging/logger.hpp>
#include <SushiTrack/profiling/profiler_keys.hpp>
#include <SushiTrack/profiling/sushitrack_profiler_parts.hpp>

#include <mot_engine_log_category.hpp>

namespace MOTEngine
{
    namespace
    {
        /// @brief The environment fact naming the tracker the record was made with.
        constexpr const char* K_FACT_TRACKER = "mot.tracker";
        /// @brief The environment fact naming the sequence the record was made over.
        constexpr const char* K_FACT_SEQUENCE = "mot.sequence";
        /// @brief Events each thread ring holds: 4 times the default, so a benchmark loop that
        ///        outruns the consumer's wait does not drop events.
        constexpr std::uint32_t K_RING_EVENTS = 262'144U;

        /**
         * @brief Adds the tracker and the sequence of the run to a record's environment.
         */
        class RunFacts final : public SushiProfiler::EnvironmentContributor
        {
            public:
                /// @brief Holds the names to state.
                RunFacts(std::string tracker, std::string sequence)
                    : tracker_(std::move(tracker)), sequence_(std::move(sequence))
                {

                }

                /// @brief Sets `mot.tracker` and `mot.sequence`.
                void contribute(SushiProfiler::RecordFacts& facts) const override
                {
                    SushiProfiler::set_fact(facts.environment, K_FACT_TRACKER, tracker_);
                    SushiProfiler::set_fact(facts.environment, K_FACT_SEQUENCE, sequence_);
                }

            private:
                /// @brief The tracker's name.
                std::string tracker_;
                /// @brief The sequence's name.
                std::string sequence_;
        };
    } // namespace

    /// @brief Owns the registry, the contributors and the running session.
    struct FrameRecorder::Impl
    {
            /// @brief Registers the contributors in the registry.
            Impl(std::string tracker, std::string sequence)
                : facts(std::move(tracker), std::move(sequence)),
                  parts(SushiTrack::register_profiler_parts(registry))
            {
                registry.add_contributor(facts);
            }

            SushiProfiler::Registry registry;                 /**< Where the parts register. */
            RunFacts facts;                                   /**< The run's own facts. */
            SushiTrack::SushiTrackProfilerParts parts;        /**< SushiTrack's facts. */
            std::unique_ptr<SushiProfiler::Session> session;  /**< The running session. */
            SushiProfiler::PeriodTimeline* frame = nullptr;   /**< The frame timeline. */
            std::filesystem::path path;                       /**< Where the record is written. */
    };

    FrameRecorder::FrameRecorder(const std::filesystem::path& folder, const std::string& tracker,
                                 const std::string& sequence, std::size_t frames)
    {
        if (folder.empty())
            return;

        auto impl = std::make_unique<Impl>(tracker, sequence);
        SushiProfiler::ProfilerConfig config;
        config.timeline = SushiTrack::ProfilerKeys::K_TIMELINE_FRAME;
        config.window.skip = 0U;
        config.window.wait = 0U;
        config.window.warmup = 0U;
        config.window.active = static_cast<std::uint32_t>(
            std::clamp<std::size_t>(frames, 1U, std::numeric_limits<std::uint32_t>::max()));
        config.window.repeat = 1U;
        config.buffers.ring_events = K_RING_EVENTS;
        config.buffers.linger_ms = 0U;

        SushiProfiler::SessionStart started =
            SushiProfiler::Session::start(config, impl->registry);
        if (!started.ok())
        {
            SUSHITRACK_LOG_ERROR(K_MOT_ENGINE_LOG_CATEGORY,
                                 "No profile for {} on {}: {}; the run goes on unrecorded.",
                                 tracker, sequence, started.error);
            return;
        }

        std::error_code error;
        std::filesystem::create_directories(folder, error);
        impl->path = folder / (tracker + "_" + sequence + ".sprec.json");
        impl->session = std::move(started.session);
        impl->frame = &impl->session->timeline(SushiTrack::ProfilerKeys::K_TIMELINE_FRAME);
        impl_ = std::move(impl);
    }

    FrameRecorder::~FrameRecorder() = default;

    void FrameRecorder::begin_frame()
    {
        if (impl_)
            impl_->frame->mark();
    }

    FrameRecording FrameRecorder::finish()
    {
        FrameRecording recording;
        if (!impl_)
            return recording;

        impl_->frame->mark();
        SushiProfiler::SessionStop stopped = impl_->session->stop();
        for (const std::string& problem : stopped.problems)
        {
            SUSHITRACK_LOG_WARN(K_MOT_ENGINE_LOG_CATEGORY, "Profile session problem: {}.",
                                problem);
        }

        recording.dropped = stopped.record.drops.outside_periods +
                            stopped.record.drops.open_at_stop;
        for (const SushiProfiler::ThreadRecord& thread : stopped.record.threads)
            recording.dropped += thread.dropped + thread.late;
        if (recording.dropped != 0U)
        {
            SUSHITRACK_LOG_WARN(K_MOT_ENGINE_LOG_CATEGORY,
                                "The profile of {} lost {} events; its timings are incomplete.",
                                impl_->path.string(), recording.dropped);
        }

        std::ofstream file(impl_->path, std::ios::binary);
        file << SushiProfiler::write_record(stopped.record);
        if (!file)
        {
            recording.error = "cannot write " + impl_->path.string();
            SUSHITRACK_LOG_ERROR(K_MOT_ENGINE_LOG_CATEGORY, "No profile written: {}.",
                                 recording.error);
        }
        else
        {
            recording.path = impl_->path;
        }
        impl_.reset();
        return recording;
    }
} // namespace MOTEngine

#else

namespace MOTEngine
{
    /// @brief Holds nothing: the profiler is compiled out.
    struct FrameRecorder::Impl
    {

    };

    FrameRecorder::FrameRecorder(const std::filesystem::path&, const std::string&,
                                 const std::string&, std::size_t)
    {

    }

    FrameRecorder::~FrameRecorder() = default;

    void FrameRecorder::begin_frame()
    {

    }

    FrameRecording FrameRecorder::finish()
    {
        return {};
    }
} // namespace MOTEngine

#endif
