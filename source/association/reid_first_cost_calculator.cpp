/**************************************************************************/
/* reid_first_cost_calculator.cpp                                         */
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
 * @file reid_first_cost_calculator.cpp
 * @brief Defines the cost calculator that tries appearance before overlap.
 * @author Mustafa Garip
 */

#include <SushiTrack/association/reid_first_cost_calculator.hpp>

#include <algorithm>
#include <association/state_association_modifiers.hpp>
#include <vector>

#include <SushiTrack/tracking/tracklet.hpp>

namespace SushiTrack
{
    /**
     * @brief Implements a prioritized association strategy using visual similarity.
     * @param a Input collection of source tracklets for association.
     * @param b Input collection of candidate tracklets for association.
     * @param out_matrix Reference to the cost matrix for result population.
     */
    void ReIDFirstCostCalculator::calculate(const std::vector<TrackletPtr>& __restrict a,
                                            const std::vector<TrackletPtr>& __restrict b,
                                            CostMatrix& __restrict out_matrix)
    {
        out_matrix.resize(a.size(), b.size(), 1e4f);
        if (a.empty() || b.empty())
            return;

        for (size_t i = 0; i < a.size(); ++i)
        {
            // Per-row state policy: Stationary leans into IoU, Maneuvering/Coasting into appearance, Coasting also rejects geometry-only fallback.
            const StateAssocMods sm = state_assoc_mods(a[i]->get_last_seen_state());
            const float row_iou_weight = std::clamp(iou_weight_ - sm.reid_shift, 0.0f, 1.0f);

            for (size_t j = 0; j < b.size(); ++j)
            {
                float cost = 1e4f;

                if (a[i]->has_feature() && b[j]->has_feature())
                {
                    float reid_sim = a[i]->get_max_feature_similarity(b[j]->get_curr_feat());
                    float reid_dist = 1.0f - reid_sim;

                    if (reid_dist < reid_conf_thresh_)
                    {
                        cost = reid_dist * reid_direct_assignment_weight_;
                        out_matrix(i, j) = cost;
                        continue;
                    }

                    float iou_d = iou_calc_ ? iou_calc_->calculate_single(a[i], b[j]) : 1e4f;
                    if (iou_d >= 1.0f)
                    {
                        out_matrix(i, j) = 1e4f;
                        continue;
                    }
                    cost = (1.0f - row_iou_weight) * reid_dist + row_iou_weight * iou_d;
                    out_matrix(i, j) = cost;
                    continue;
                }

                // Geometry-only fallback (no appearance on one side); Coasting suppresses it, as appearance-free recovery is the main post-occlusion identity-hijack source.
                if (sm.require_reid)
                {
                    out_matrix(i, j) = 1e4f;
                    continue;
                }

                if (iou_calc_)
                {
                    float iou_d = iou_calc_->calculate_single(a[i], b[j]);
                    cost = iou_d;
                    out_matrix(i, j) = cost;
                }
            }
        }
    }

    /**
     * @brief Computes the association cost between a single source and candidate tracklet.
     * @param a Source tracklet for association.
     * @param b Candidate tracklet for association.
     * @return Calculated association cost.
     */
    float ReIDFirstCostCalculator::calculate_single(const TrackletPtr& a,
                                                    const TrackletPtr& b) const
    {
        // Mirror batch calculate() row policy so single-pair path is regime-consistent.
        const StateAssocMods sm = state_assoc_mods(a->get_last_seen_state());
        const float row_iou_weight = std::clamp(iou_weight_ - sm.reid_shift, 0.0f, 1.0f);

        if (a->has_feature() && b->has_feature())
        {
            float reid_sim = a->get_max_feature_similarity(b->get_curr_feat());
            float reid_dist = 1.0f - reid_sim;

            if (reid_dist < reid_conf_thresh_)
            {
                return reid_dist * reid_direct_assignment_weight_;
            }

            float iou_d = iou_calc_ ? iou_calc_->calculate_single(a, b) : 1e4f;
            if (iou_d >= 1.0f)
            {
                return 1e4f;
            }
            return (1.0f - row_iou_weight) * reid_dist + row_iou_weight * iou_d;
        }

        if (sm.require_reid)
        {
            return 1e4f;
        }

        if (iou_calc_)
        {
            return iou_calc_->calculate_single(a, b);
        }

        return 1e4f;
    }
} // namespace SushiTrack
