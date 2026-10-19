/**************************************************************************/
/* giou_cost_calculator.cpp                                               */
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
 * @file giou_cost_calculator.cpp
 * @brief Defines the generalised-IoU cost calculator.
 * @author Mustafa Garip
 */

#include <SushiTrack/association/giou_cost_calculator.hpp>

#include <algorithm>
#include <vector>

#include <SushiTrack/tracking/tracklet.hpp>

namespace SushiTrack
{
    void GIoUCostCalculator::calculate(const std::vector<TrackletPtr>& __restrict a,
                                    const std::vector<TrackletPtr>& __restrict b,
                                    CostMatrix& __restrict out_matrix)
    {
        out_matrix.resize(a.size(), b.size(), 1e4f);
        if (a.empty() || b.empty()) return;

        const float offset = iou_plus_one_ ? 1.0f : 0.0f;

        row_areas_.clear();
        row_areas_.reserve(a.size());
        for (const auto& track : a)
        {
            const auto& r = track->getRect();
            row_areas_.push_back(r.width() * r.height());
        }

        col_areas_.clear();
        col_areas_.reserve(b.size());
        for (const auto& det : b)
        {
            const auto& r = det->getRect();
            col_areas_.push_back(r.width() * r.height());
        }

        for (size_t i = 0; i < a.size(); ++i)
        {
            const auto& r1 = a[i]->getRect();
            float area1 = (r1.width() + offset) * (r1.height() + offset);

            for (size_t j = 0; j < b.size(); ++j)
            {
                float ratio = row_areas_[i] > col_areas_[j]
                    ? row_areas_[i] / std::max(1e-6f, col_areas_[j])
                    : col_areas_[j] / std::max(1e-6f, row_areas_[i]);

                if (ratio > max_area_ratio_)
                {
                    out_matrix(i, j) = 1e4f;
                    continue;
                }

                const auto& r2 = b[j]->getRect();
                float area2 = (r2.width() + offset) * (r2.height() + offset);

                float iw = std::min(r1.x() + r1.width(), r2.x() + r2.width())
                        - std::max(r1.x(), r2.x()) + offset;
                float iou = 0.0f;
                float intersection = 0.0f;
                float ua = area1 + area2;
                if (iw > 0)
                {
                    float ih = std::min(r1.y() + r1.height(), r2.y() + r2.height())
                            - std::max(r1.y(), r2.y()) + offset;
                    if (ih > 0)
                    {
                        intersection = iw * ih;
                        ua = area1 + area2 - intersection;
                        iou = intersection / std::max(1e-6f, ua);
                    }
                }

                float enc_w = std::max(r1.x() + r1.width(), r2.x() + r2.width())
                            - std::min(r1.x(), r2.x()) + offset;
                float enc_h = std::max(r1.y() + r1.height(), r2.y() + r2.height())
                            - std::min(r1.y(), r2.y()) + offset;
                float ac = enc_w * enc_h;

                float giou = iou - (ac - ua) / std::max(1e-6f, ac);
                out_matrix(i, j) = 1.0f - giou;
            }
        }
    }

    float GIoUCostCalculator::calculate_single(const TrackletPtr& a, const TrackletPtr& b) const
    {
        const float offset = iou_plus_one_ ? 1.0f : 0.0f;
        const auto& r1 = a->getRect();
        const auto& r2 = b->getRect();
        float area1 = (r1.width() + offset) * (r1.height() + offset);
        float area2 = (r2.width() + offset) * (r2.height() + offset);

        float raw_area1 = r1.width() * r1.height();
        float raw_area2 = r2.width() * r2.height();
        float ratio = (raw_area1 > raw_area2)
            ? (raw_area1 / std::max(1e-6f, raw_area2))
            : (raw_area2 / std::max(1e-6f, raw_area1));

        if (ratio > max_area_ratio_) return 1e4f;

        float iw = std::min(r1.x() + r1.width(), r2.x() + r2.width())
                - std::max(r1.x(), r2.x()) + offset;
        float iou = 0.0f;
        float intersection = 0.0f;
        float ua = area1 + area2;
        if (iw > 0)
        {
            float ih = std::min(r1.y() + r1.height(), r2.y() + r2.height())
                    - std::max(r1.y(), r2.y()) + offset;
            if (ih > 0)
            {
                intersection = iw * ih;
                ua = area1 + area2 - intersection;
                iou = intersection / std::max(1e-6f, ua);
            }
        }

        float enc_w = std::max(r1.x() + r1.width(), r2.x() + r2.width())
                    - std::min(r1.x(), r2.x()) + offset;
        float enc_h = std::max(r1.y() + r1.height(), r2.y() + r2.height())
                    - std::min(r1.y(), r2.y()) + offset;
        float ac = enc_w * enc_h;

        float giou = iou - (ac - ua) / std::max(1e-6f, ac);
        return 1.0f - giou;
    }
} // namespace SushiTrack
