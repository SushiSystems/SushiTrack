/**************************************************************************/
/* siou_cost_calculator.cpp                                               */
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
 * @file siou_cost_calculator.cpp
 * @brief Defines the SCYLLA-IoU cost calculator.
 * @author Mustafa Garip
 */

#include <SushiTrack/association/siou_cost_calculator.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

#include <SushiTrack/tracking/tracklet.hpp>

namespace SushiTrack
{
    void SIoUCostCalculator::calculate(const std::vector<TrackletPtr>& __restrict a,
                                       const std::vector<TrackletPtr>& __restrict b,
                                       CostMatrix& __restrict out_matrix)
    {
        out_matrix.resize(a.size(), b.size(), 1e4f);
        if (a.empty() || b.empty())
            return;

        const float offset = iou_plus_one_ ? 1.0f : 0.0f;

        row_areas_.clear();
        row_areas_.reserve(a.size());
        for (const auto& track : a)
        {
            const auto& r = track->get_rect();
            row_areas_.push_back(r.width() * r.height());
        }

        col_areas_.clear();
        col_areas_.reserve(b.size());
        for (const auto& det : b)
        {
            const auto& r = det->get_rect();
            col_areas_.push_back(r.width() * r.height());
        }

        for (size_t i = 0; i < a.size(); ++i)
        {
            const auto& r1 = a[i]->get_rect();
            float area1 = (r1.width() + offset) * (r1.height() + offset);
            float cx1 = r1.x() + r1.width() * 0.5f;
            float cy1 = r1.y() + r1.height() * 0.5f;

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

                const auto& r2 = b[j]->get_rect();
                float area2 = (r2.width() + offset) * (r2.height() + offset);

                float iw = std::min(r1.x() + r1.width(), r2.x() + r2.width()) -
                           std::max(r1.x(), r2.x()) + offset;
                float iou = 0.0f;
                if (iw > 0)
                {
                    float ih = std::min(r1.y() + r1.height(), r2.y() + r2.height()) -
                               std::max(r1.y(), r2.y()) + offset;
                    if (ih > 0)
                    {
                        float ua = area1 + area2 - iw * ih;
                        iou = (iw * ih) / std::max(1e-6f, ua);
                    }
                }

                float cx2 = r2.x() + r2.width() * 0.5f;
                float cy2 = r2.y() + r2.height() * 0.5f;

                float ch = std::abs(cy1 - cy2);
                float cw = std::abs(cx1 - cx2);
                float sigma = std::sqrt(ch * ch + cw * cw + 1e-6f);
                float sin_alpha = std::min(ch, cw) / sigma;
                float cos_alpha = std::sqrt(std::max(0.0f, 1.0f - sin_alpha * sin_alpha));
                float angle_cost = 2.0f * sin_alpha * cos_alpha;
                if (sigma < 1e-3f)
                    angle_cost = 0.0f;

                float enc_w = std::max(r1.x() + r1.width(), r2.x() + r2.width()) -
                              std::min(r1.x(), r2.x()) + offset;
                float enc_h = std::max(r1.y() + r1.height(), r2.y() + r2.height()) -
                              std::min(r1.y(), r2.y()) + offset;

                float gamma = 2.0f - angle_cost;
                // Explicit product, not std::pow(x, 2): pow's integer-exponent overload promotes to double and narrows back (C4244).
                float dx_norm = (cx1 - cx2) / std::max(1e-6f, enc_w);
                float dy_norm = (cy1 - cy2) / std::max(1e-6f, enc_h);
                float rho_x = dx_norm * dx_norm;
                float rho_y = dy_norm * dy_norm;
                float distance_cost =
                    (1.0f - std::exp(-gamma * rho_x)) + (1.0f - std::exp(-gamma * rho_y));

                float omega_w =
                    std::abs(r1.width() - r2.width()) / std::max({1e-6f, r1.width(), r2.width()});
                float omega_h = std::abs(r1.height() - r2.height()) /
                                std::max({1e-6f, r1.height(), r2.height()});
                float shape_cost = std::pow(1.0f - std::exp(-omega_w), 4.0f) +
                                   std::pow(1.0f - std::exp(-omega_h), 4.0f);

                float siou = iou - (distance_cost + shape_cost) * 0.5f;
                out_matrix(i, j) = 1.0f - siou;
            }
        }
    }

    float SIoUCostCalculator::calculate_single(const TrackletPtr& a, const TrackletPtr& b) const
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
        float iou = 0.0f;
        if (iw > 0)
        {
            float ih = std::min(r1.y() + r1.height(), r2.y() + r2.height()) -
                       std::max(r1.y(), r2.y()) + offset;
            if (ih > 0)
            {
                float ua = area1 + area2 - iw * ih;
                iou = (iw * ih) / std::max(1e-6f, ua);
            }
        }

        float cx1 = r1.x() + r1.width() * 0.5f;
        float cy1 = r1.y() + r1.height() * 0.5f;
        float cx2 = r2.x() + r2.width() * 0.5f;
        float cy2 = r2.y() + r2.height() * 0.5f;

        float ch = std::abs(cy1 - cy2);
        float cw = std::abs(cx1 - cx2);
        float sigma = std::sqrt(ch * ch + cw * cw + 1e-6f);
        float sin_alpha = std::min(ch, cw) / sigma;
        float cos_alpha = std::sqrt(std::max(0.0f, 1.0f - sin_alpha * sin_alpha));
        float angle_cost = 2.0f * sin_alpha * cos_alpha;
        if (sigma < 1e-3f)
            angle_cost = 0.0f;

        float enc_w =
            std::max(r1.x() + r1.width(), r2.x() + r2.width()) - std::min(r1.x(), r2.x()) + offset;
        float enc_h = std::max(r1.y() + r1.height(), r2.y() + r2.height()) -
                      std::min(r1.y(), r2.y()) + offset;

        float gamma = 2.0f - angle_cost;
        // Explicit product, not std::pow(x, 2): pow's integer-exponent overload promotes to double and narrows back (C4244).
        float dx_norm = (cx1 - cx2) / std::max(1e-6f, enc_w);
        float dy_norm = (cy1 - cy2) / std::max(1e-6f, enc_h);
        float rho_x = dx_norm * dx_norm;
        float rho_y = dy_norm * dy_norm;
        float distance_cost = (1.0f - std::exp(-gamma * rho_x)) + (1.0f - std::exp(-gamma * rho_y));

        float omega_w =
            std::abs(r1.width() - r2.width()) / std::max({1e-6f, r1.width(), r2.width()});
        float omega_h =
            std::abs(r1.height() - r2.height()) / std::max({1e-6f, r1.height(), r2.height()});
        float shape_cost =
            std::pow(1.0f - std::exp(-omega_w), 4.0f) + std::pow(1.0f - std::exp(-omega_h), 4.0f);

        float siou = iou - (distance_cost + shape_cost) * 0.5f;
        return 1.0f - siou;
    }
} // namespace SushiTrack
