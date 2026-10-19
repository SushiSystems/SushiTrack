/**************************************************************************/
/* fuse_score_decorator.cpp                                               */
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
 * @file fuse_score_decorator.cpp
 * @brief Defines the decorator that scales costs by detection score.
 * @author Mustafa Garip
 */

#include <SushiTrack/association/fuse_score_decorator.hpp>

#include <algorithm>
#include <vector>

#include <SushiTrack/tracking/tracklet.hpp>

namespace SushiTrack
{
    /**
     * @brief Applies score-weighted rescaling to an inner cost matrix.
     * @param a Collection of source tracklets.
     * @param b Collection of target tracklets (observations).
     * @param out_matrix Output cost matrix populated by inner calculator and rescaled.
     */
    void FuseScoreDecorator::calculate(const std::vector<TrackletPtr>& __restrict a,
                                       const std::vector<TrackletPtr>& __restrict b,
                                       CostMatrix& __restrict out_matrix)
    {
        inner_->calculate(a, b, out_matrix);
        if (a.empty() || b.empty()) return;

        constexpr float kSentinel = 1e3f;

        for (size_t j = 0; j < b.size(); ++j)
        {
            const float score = std::clamp(b[j]->getScore(), 0.0f, 1.0f);
            for (size_t i = 0; i < a.size(); ++i)
            {
                float c = out_matrix(i, j);
                if (c >= kSentinel) continue;
                out_matrix(i, j) = 1.0f - (1.0f - c) * score;
            }
        }
    }

    /**
     * @brief Single-pair score-weighted cost.
     * @param a Source tracklet.
     * @param b Candidate tracklet.
     * @return Score-fused association cost.
     */
    float FuseScoreDecorator::calculate_single(const TrackletPtr& a, const TrackletPtr& b) const
    {
        float c = inner_->calculate_single(a, b);
        if (c >= 1e3f) return c;
        const float score = std::clamp(b->getScore(), 0.0f, 1.0f);
        return 1.0f - (1.0f - c) * score;
    }
} // namespace SushiTrack
