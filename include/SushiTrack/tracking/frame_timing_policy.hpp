/**************************************************************************/
/* frame_timing_policy.hpp                                                */
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
 * @file frame_timing_policy.hpp
 * @brief Declares the timing policy that counts frames.
 * @author Mustafa Garip
 */

#pragma once

#include <SushiTrack/tracking/timing_policy.hpp>

namespace SushiTrack
{
    /**
     * @brief Implements a discrete-step timing policy.
     */
    class FrameTimingPolicy final : public TimingPolicy
    {
        public:
            /**
             * @brief Returns a constant unit step regardless of input.
             * @param raw_dt Input time interval (unused).
             * @return Constant value of 1.0.
             */
            [[nodiscard]] float to_kf_dt(float /*raw_dt*/) const override { return 1.0f; }

            /**
             * @brief Evaluates expiration using frame count comparisons.
             * @param frame_count Number of frames elapsed.
             * @param buffer_frames Configured persistence limit in frames.
             * @return True if frame count exceeds limit.
             */
            [[nodiscard]] bool is_expired(float frame_count, int buffer_frames) const override
            {
                return frame_count > static_cast<float>(buffer_frames);
            }
    };
} // namespace SushiTrack
