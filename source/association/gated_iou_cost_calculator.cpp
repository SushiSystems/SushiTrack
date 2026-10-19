/**************************************************************************/
/* gated_iou_cost_calculator.cpp                                          */
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
 * @file gated_iou_cost_calculator.cpp
 * @brief Defines the IoU cost calculator gated by motion.
 * @author Mustafa Garip
 */

#include <SushiTrack/association/gated_iou_cost_calculator.hpp>

#include <algorithm>
#include <association/state_association_modifiers.hpp>
#include <memory>
#include <vector>

#include <SushiTrack/logging/logger.hpp>
#include <SushiTrack/tracking/tracklet.hpp>

namespace SushiTrack
{
    /**
     * @brief Initializes a GatedIoUCostCalculator instance.
     * @param gating_thresh Chi-squared threshold for Mahalanobis gating.
     * @param gate_validator Instance used for statistical gating validation.
     * @param iou_calc Spatial overlap calculator instance.
     * @param motion_cost_weight Weight assigned to the motion penalty.
     */
    GatedIoUCostCalculator::GatedIoUCostCalculator(float gating_thresh,
                                                   std::shared_ptr<GateValidator> gate_validator,
                                                   std::shared_ptr<CostCalculator> iou_calc,
                                                   float motion_cost_weight)
        : gating_thresh_(gating_thresh),
          motion_cost_weight_(std::max(0.0f, std::min(0.999f, motion_cost_weight))),
          gate_validator_(std::move(gate_validator)),
          iou_calc_(std::move(iou_calc))
    {
    }

    /**
     * @brief Computes spatial overlap costs with motion-based gating.
     * @param a Input collection of source tracklets for association.
     * @param b Input collection of candidate tracklets for association.
     * @param out_matrix Reference to the cost matrix for result population.
     */
    void GatedIoUCostCalculator::calculate(const std::vector<TrackletPtr>& __restrict a,
                                           const std::vector<TrackletPtr>& __restrict b,
                                           CostMatrix& __restrict out_matrix)
    {
        out_matrix.resize(a.size(), b.size(), 1e4f);
        if (a.empty() || b.empty())
            return;

        meas_workspace_.clear();
        score_workspace_.clear();
        meas_workspace_.reserve(b.size());
        score_workspace_.reserve(b.size());
        for (const auto& det : b)
        {
            meas_workspace_.push_back(det->get_measurement());
            score_workspace_.push_back(det->get_score());
        }

        const bool fuse_motion = (motion_cost_weight_ > 1e-6f);
        size_t gated_pairs = 0;

        for (size_t i = 0; i < a.size(); ++i)
        {
            // Tighten the gate for Stationary, loosen for Maneuvering/Coasting.
            const StateAssocMods sm = state_assoc_mods(a[i]->get_last_seen_state());
            const float row_gate_thresh = std::max(1e-3f, gating_thresh_ * sm.gate_scale);
            const float gate = row_gate_thresh;

            // Single projection pass: gate flags are derived from squared distances.
            std::vector<float> sq_dists = gate_validator_->get_gate_sq_distances(
                a[i]->get_mean(), a[i]->get_covariance(), meas_workspace_, score_workspace_);

            for (size_t j = 0; j < b.size(); ++j)
            {
                // Validation gate: Mahalanobis distance past threshold gets a sentinel cost.
                if (sq_dists[j] > row_gate_thresh)
                {
                    ++gated_pairs;
                    out_matrix(i, j) = 1e4f;
                    continue;
                }

                float iou_d = iou_calc_->calculate_single(a[i], b[j]);

                if (fuse_motion)
                {
                    float motion = sq_dists[j] / gate;
                    float motion_penalty = motion_cost_weight_ * motion;
                    out_matrix(i, j) = 1.0f - (1.0f - iou_d) * (1.0f - motion_penalty);
                }
                else
                {
                    out_matrix(i, j) = iou_d;
                }
            }
        }

        if (gated_pairs > 0)
        {
            SUSHITRACK_LOG_TRACE(K_SUSHITRACK_LOG_CATEGORY,
                                 "Gating: gated out {} of {} pairs (Mahalanobis).", gated_pairs,
                                 a.size() * b.size());
        }
    }

    /**
     * @brief Computes spatial overlap cost with motion-based gating for a single pair.
     * @param a Source tracklet for association.
     * @param b Candidate tracklet for association.
     * @return Calculated spatial overlap cost with gating penalties.
     */
    float GatedIoUCostCalculator::calculate_single(const TrackletPtr& a, const TrackletPtr& b) const
    {
        // Mirror batch calculate() row policy with single projection pass.
        const StateAssocMods sm = state_assoc_mods(a->get_last_seen_state());
        const float row_gate_thresh = std::max(1e-3f, gating_thresh_ * sm.gate_scale);

        KalmanTypes::DetectBoxVector meas_workspace = {b->get_measurement()};
        std::vector<float> score_workspace = {b->get_score()};
        std::vector<float> sq_dists = gate_validator_->get_gate_sq_distances(
            a->get_mean(), a->get_covariance(), meas_workspace, score_workspace);

        if (sq_dists[0] > row_gate_thresh)
        {
            return 1e4f;
        }

        float iou_d = iou_calc_->calculate_single(a, b);

        const bool fuse_motion = (motion_cost_weight_ > 1e-6f);
        if (fuse_motion)
        {
            float motion = sq_dists[0] / row_gate_thresh;
            if (motion > 1.0f)
                motion = 1.0f;

            float motion_penalty = motion_cost_weight_ * motion;
            return 1.0f - (1.0f - iou_d) * (1.0f - motion_penalty);
        }

        return iou_d;
    }
} // namespace SushiTrack
