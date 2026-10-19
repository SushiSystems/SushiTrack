/**************************************************************************/
/* iou_cost_calculator.cpp                                                */
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
 * @file iou_cost_calculator.cpp
 * @brief Defines the IoU cost calculator.
 * @author Mustafa Garip
 */

#include <SushiTrack/association/iou_cost_calculator.hpp>

#include <algorithm>
#include <vector>

#include <SushiTrack/tracking/tracklet.hpp>

namespace SushiTrack
{
    /**
     * @brief Computes association costs using Intersection over Union (IoU).
     * @param a Input collection of source tracklets for association.
     * @param b Input collection of candidate tracklets for association.
     * @param out_matrix Reference to the cost matrix for result population.
     */
    void IoUCostCalculator::calculate(const std::vector<TrackletPtr>& __restrict a,
                                      const std::vector<TrackletPtr>& __restrict b,
                                      CostMatrix& __restrict out_matrix)
    {
        out_matrix.resize(a.size(), b.size(), 1.0f);

        if (a.empty() || b.empty())
            return;

        const float offset = iou_plus_one_ ? 1.0f : 0.0f;

        for (size_t i = 0; i < a.size(); ++i)
        {
            const auto& r1 = a[i]->get_rect();
            float area1 = (r1.width() + offset) * (r1.height() + offset);

            for (size_t j = 0; j < b.size(); ++j)
            {
                const auto& r2 = b[j]->get_rect();
                float area2 = (r2.width() + offset) * (r2.height() + offset);

                float iw = std::min(r1.x() + r1.width(), r2.x() + r2.width()) -
                           std::max(r1.x(), r2.x()) + offset;

                if (iw > 0)
                {
                    float ih = std::min(r1.y() + r1.height(), r2.y() + r2.height()) -
                               std::max(r1.y(), r2.y()) + offset;
                    if (ih > 0)
                    {
                        float ua = area1 + area2 - iw * ih;
                        out_matrix(i, j) = 1.0f - (iw * ih / std::max(1e-6f, ua));
                    }
                }

                float raw_area1 = r1.width() * r1.height();
                float raw_area2 = r2.width() * r2.height();
                float ratio = (raw_area1 > raw_area2) ? (raw_area1 / std::max(1e-6f, raw_area2))
                                                      : (raw_area2 / std::max(1e-6f, raw_area1));

                if (ratio > max_area_ratio_)
                    out_matrix(i, j) = 1e4f;
            }
        }
    }

    /**
     * @brief Computes the association cost for a single pair using Intersection over Union (IoU).
     * @param a Source tracklet for association.
     * @param b Candidate tracklet for association.
     * @return Calculated IoU-based cost.
     */
    float IoUCostCalculator::calculate_single(const TrackletPtr& a, const TrackletPtr& b) const
    {
        const float offset = iou_plus_one_ ? 1.0f : 0.0f;
        const auto& r1 = a->get_rect();
        const auto& r2 = b->get_rect();
        float area1 = (r1.width() + offset) * (r1.height() + offset);
        float area2 = (r2.width() + offset) * (r2.height() + offset);

        float raw_area1 = r1.width() * r1.height();
        float raw_area2 = r2.width() * r2.height();
        float ratio = (raw_area1 > raw_area2) ? (raw_area1 / std::max(1e-6f, raw_area2))
                                              : (raw_area2 / std::max(1e-6f, raw_area1));

        if (ratio > max_area_ratio_)
            return 1e4f;

        float iw =
            std::min(r1.x() + r1.width(), r2.x() + r2.width()) - std::max(r1.x(), r2.x()) + offset;
        if (iw > 0)
        {
            float ih = std::min(r1.y() + r1.height(), r2.y() + r2.height()) -
                       std::max(r1.y(), r2.y()) + offset;
            if (ih > 0)
            {
                float ua = area1 + area2 - iw * ih;
                return 1.0f - (iw * ih / std::max(1e-6f, ua));
            }
        }
        return 1.0f;
    }
} // namespace SushiTrack
