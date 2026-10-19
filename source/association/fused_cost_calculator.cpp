/**************************************************************************/
/* fused_cost_calculator.cpp                                              */
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
 * @file fused_cost_calculator.cpp
 * @brief Defines the cost calculator that fuses overlap, appearance and motion gating.
 * @author Mustafa Garip
 */

#include <SushiTrack/association/fused_cost_calculator.hpp>

#include <algorithm>
#include <memory>
#include <vector>

#include <SushiTrack/tracking/tracklet.hpp>
#include <SushiTrack/logging/logger.hpp>
#include <association/state_association_modifiers.hpp>

namespace SushiTrack
{
    /**
     * @brief Initializes a FusedCostCalculator instance.
     */
    FusedCostCalculator::FusedCostCalculator(float iou_weight,
                                            float reid_thresh,
                                            float gating_thresh,
                                            std::shared_ptr<IGateValidator> gate_validator,
                                            std::shared_ptr<ICostCalculator> iou_calc,
                                            std::shared_ptr<ICostCalculator> cosine_calc,
                                            float reid_direct_assignment_weight)
        : iou_weight_(iou_weight),
        reid_thresh_(reid_thresh),
        gating_thresh_(gating_thresh),
        gate_validator_(std::move(gate_validator)),
        iou_calc_(std::move(iou_calc)),
        cosine_calc_(std::move(cosine_calc)),
        reid_direct_assignment_weight_(reid_direct_assignment_weight)
    {
    }

    /**
     * @brief Aggregates spatial overlap and visual similarity costs.
     * @param a Input collection of source tracklets for association.
     * @param b Input collection of candidate tracklets for association.
     * @param out_matrix Reference to the cost matrix for result population.
     */
    void FusedCostCalculator::calculate(const std::vector<TrackletPtr>& __restrict a,
                                        const std::vector<TrackletPtr>& __restrict b,
                                        CostMatrix& __restrict out_matrix)
    {
        out_matrix.resize(a.size(), b.size(), 1e4f);
        if (a.empty() || b.empty()) return;

        meas_workspace_.clear();
        score_workspace_.clear();
        meas_workspace_.reserve(b.size());
        score_workspace_.reserve(b.size());
        for (const auto& det : b)
        {
            meas_workspace_.push_back(det->getMeasurement());
            score_workspace_.push_back(det->getScore());
        }

        size_t gated_pairs = 0;

        for (size_t i = 0; i < a.size(); ++i)
        {
            // Per-track state-aware policy; without an IMM regime the state is Cruising and the modifiers collapse to identity.
            const StateAssocMods sm = stateAssocMods(a[i]->getLastSeenState());
            const float row_gate_thresh = std::max(1e-3f, gating_thresh_ * sm.gate_scale);
            const float row_motion_w    = std::clamp(iou_weight_ - sm.reid_shift, 0.0f, 1.0f);
            const float row_appear_w    = 1.0f - row_motion_w;
            const float gate            = row_gate_thresh;

            // Single projection pass: gate flags are derived from squared distances.
            std::vector<float> sq_dists = gate_validator_->getGateSqDistances(
                a[i]->getMean(), a[i]->getCovariance(), meas_workspace_, score_workspace_);

            for (size_t j = 0; j < b.size(); ++j)
            {
                // Validation gate: Mahalanobis distance past the chi-squared threshold gets a sentinel cost penalty.
                if (sq_dists[j] > row_gate_thresh)
                {
                    ++gated_pairs;
                    out_matrix(i, j) = 1e4f;
                    continue;
                }

                float motion_d = sq_dists[j] / gate;

                if (a[i]->hasFeature() && b[j]->hasFeature())
                {
                    float max_sim = a[i]->getMaxFeatureSimilarity(b[j]->getCurrFeat());
                    float cos_d = std::max(0.0f, 1.0f - max_sim);

                    if (reid_direct_assignment_weight_ > 0.0f && cos_d < reid_thresh_)
                    {
                        out_matrix(i, j) = cos_d * reid_direct_assignment_weight_;
                    }
                    else
                    {
                        if (cos_d > reid_thresh_)
                            cos_d = 1.0f;
                        out_matrix(i, j) = row_motion_w * motion_d + row_appear_w * cos_d;
                    }
                }
                else if (sm.require_reid)
                {
                    // Coasting recoveries demand appearance corroboration; geometry alone after an occlusion gap is the dominant identity-hijack source in crowds.
                    out_matrix(i, j) = 1e4f;
                }
                else
                {
                    out_matrix(i, j) = motion_d;
                }
            }
        }

        if (gated_pairs > 0)
        {
            SUSHITRACK_LOG_DEBUG("[Distance] Gating: gated out %zu of %zu pairs (Mahalanobis).",
                gated_pairs, a.size() * b.size());
        }
    }

    /**
     * @brief Computes aggregated spatial and visual association cost for a single pair.
     * @param a Source tracklet for association.
     * @param b Candidate tracklet for association.
     * @return Aggregated association cost.
     */
    float FusedCostCalculator::calculate_single(const TrackletPtr& a, const TrackletPtr& b) const
    {
        // Mirror batch calculate() row policy for regime-consistent single pairs.
        const StateAssocMods sm = stateAssocMods(a->getLastSeenState());
        const float row_gate_thresh = std::max(1e-3f, gating_thresh_ * sm.gate_scale);
        const float row_motion_w    = std::clamp(iou_weight_ - sm.reid_shift, 0.0f, 1.0f);
        const float row_appear_w    = 1.0f - row_motion_w;

        KalmanTypes::DetectBoxVector meas_workspace = { b->getMeasurement() };
        std::vector<float>           score_workspace = { b->getScore() };
        std::vector<float> sq_dists = gate_validator_->getGateSqDistances(
            a->getMean(), a->getCovariance(), meas_workspace, score_workspace);

        if (sq_dists[0] > row_gate_thresh)
        {
            return 1e4f;
        }

        float motion_d = sq_dists[0] / row_gate_thresh;

        if (a->hasFeature() && b->hasFeature())
        {
            float max_sim = a->getMaxFeatureSimilarity(b->getCurrFeat());
            float cos_d = std::max(0.0f, 1.0f - max_sim);

            if (reid_direct_assignment_weight_ > 0.0f && cos_d < reid_thresh_)
            {
                return cos_d * reid_direct_assignment_weight_;
            }

            if (cos_d > reid_thresh_)
            {
                cos_d = 1.0f;
            }
            return row_motion_w * motion_d + row_appear_w * cos_d;
        }

        if (sm.require_reid)
        {
            return 1e4f;
        }

        return motion_d;
    }
} // namespace SushiTrack
