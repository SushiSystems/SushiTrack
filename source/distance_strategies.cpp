/****************************************************************/
/* distance_strategies.cpp                                      */
/* SushiTrack - https://github.com/SushiSystems/SushiTrack      */
/* Copyright (c) 2026 Sushi Systems                             */
/* Licensed under PolyForm Noncommercial 1.0.0. See LICENSE.    */
/* Commercial use requires a licence from Sushi Systems.        */
/****************************************************************/

/**
 * @file distance_strategies.cpp
 * @brief Implements association cost calculation strategies and metrics.
 * @author Mustafa Garip
 */

#include <SushiTrack/distance_strategies.hpp>

#include <algorithm>
#include <cmath>
#include <deque>
#include <vector>

#include <SushiTrack/kinematic_state.hpp>
#include <SushiTrack/logger.hpp>
#include <SushiTrack/tracklet.hpp>

namespace SushiTrack
{
    namespace
    {
        /**
         * @brief Per-state association policy modifiers.
         */
        struct StateAssocMods
        {
            float gate_scale;
            float reid_shift;
            bool  require_reid;
        };

        inline StateAssocMods stateAssocMods(KinematicState s)
        {
            switch (s)
            {
                case KinematicState::Stationary:  return {0.6f, -0.2f, false};
                case KinematicState::Maneuvering: return {1.5f,  0.2f, false};
                case KinematicState::Coasting:    return {2.0f,  0.4f, true };
                case KinematicState::Cruising:
                default:                          return {1.0f,  0.0f, false};
            }
        }

    } // namespace

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
        
        if (a.empty() || b.empty()) return;

        const float offset = iou_plus_one_ ? 1.0f : 0.0f;

        for (size_t i = 0; i < a.size(); ++i)
        {
            const auto& r1 = a[i]->getRect();
            float area1 = (r1.width() + offset) * (r1.height() + offset);

            for (size_t j = 0; j < b.size(); ++j)
            {
                const auto& r2 = b[j]->getRect();
                float area2 = (r2.width() + offset) * (r2.height() + offset);

                float iw = std::min(r1.x() + r1.width(), r2.x() + r2.width())
                        - std::max(r1.x(), r2.x()) + offset;

                if (iw > 0)
                {
                    float ih = std::min(r1.y() + r1.height(), r2.y() + r2.height())
                            - std::max(r1.y(), r2.y()) + offset;
                    if (ih > 0)
                    {
                        float ua = area1 + area2 - iw * ih;
                        out_matrix(i, j) = 1.0f - (iw * ih / std::max(1e-6f, ua));
                    }
                }

                float raw_area1 = r1.width() * r1.height();
                float raw_area2 = r2.width() * r2.height();
                float ratio = (raw_area1 > raw_area2)
                    ? (raw_area1 / std::max(1e-6f, raw_area2))
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
        if (iw > 0)
        {
            float ih = std::min(r1.y() + r1.height(), r2.y() + r2.height())
                    - std::max(r1.y(), r2.y()) + offset;
            if (ih > 0)
            {
                float ua = area1 + area2 - iw * ih;
                return 1.0f - (iw * ih / std::max(1e-6f, ua));
            }
        }
        return 1.0f;
    }

    /**
     * @brief Computes association costs using Distance-IoU (DIoU).
     * @param a Input collection of source tracklets for association.
     * @param b Input collection of candidate tracklets for association.
     * @param out_matrix Reference to the cost matrix for result population.
     */
    void DIoUCostCalculator::calculate(const std::vector<TrackletPtr>& __restrict a,
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

                const auto& r2 = b[j]->getRect();
                float area2 = (r2.width() + offset) * (r2.height() + offset);

                float iw = std::min(r1.x() + r1.width(), r2.x() + r2.width())
                        - std::max(r1.x(), r2.x()) + offset;
                float iou = 0.0f;
                if (iw > 0)
                {
                    float ih = std::min(r1.y() + r1.height(), r2.y() + r2.height())
                            - std::max(r1.y(), r2.y()) + offset;
                    if (ih > 0)
                    {
                        float ua = area1 + area2 - iw * ih;
                        iou = (iw * ih) / std::max(1e-6f, ua);
                    }
                }

                float cx2 = r2.x() + r2.width() * 0.5f;
                float cy2 = r2.y() + r2.height() * 0.5f;
                float d2 = (cx1 - cx2) * (cx1 - cx2) + (cy1 - cy2) * (cy1 - cy2);

                float enc_w = std::max(r1.x() + r1.width(), r2.x() + r2.width())
                            - std::min(r1.x(), r2.x()) + offset;
                float enc_h = std::max(r1.y() + r1.height(), r2.y() + r2.height())
                            - std::min(r1.y(), r2.y()) + offset;
                float c2 = enc_w * enc_w + enc_h * enc_h;

                out_matrix(i, j) = 1.0f - iou + d2 / std::max(1e-6f, c2);
            }
        }
    }

    /**
     * @brief Computes the association cost for a single pair using Distance-IoU (DIoU).
     * @param a Source tracklet for association.
     * @param b Candidate tracklet for association.
     * @return Calculated DIoU-based cost.
     */
    float DIoUCostCalculator::calculate_single(const TrackletPtr& a, const TrackletPtr& b) const
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
        if (iw > 0)
        {
            float ih = std::min(r1.y() + r1.height(), r2.y() + r2.height())
                    - std::max(r1.y(), r2.y()) + offset;
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
        float d2 = (cx1 - cx2) * (cx1 - cx2) + (cy1 - cy2) * (cy1 - cy2);

        float enc_w = std::max(r1.x() + r1.width(), r2.x() + r2.width())
                    - std::min(r1.x(), r2.x()) + offset;
        float enc_h = std::max(r1.y() + r1.height(), r2.y() + r2.height())
                    - std::min(r1.y(), r2.y()) + offset;
        float c2 = enc_w * enc_w + enc_h * enc_h;

        return 1.0f - iou + d2 / std::max(1e-6f, c2);
    }

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

    void SIoUCostCalculator::calculate(const std::vector<TrackletPtr>& __restrict a,
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

                const auto& r2 = b[j]->getRect();
                float area2 = (r2.width() + offset) * (r2.height() + offset);

                float iw = std::min(r1.x() + r1.width(), r2.x() + r2.width())
                        - std::max(r1.x(), r2.x()) + offset;
                float iou = 0.0f;
                if (iw > 0)
                {
                    float ih = std::min(r1.y() + r1.height(), r2.y() + r2.height())
                            - std::max(r1.y(), r2.y()) + offset;
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
                float Lambda = 2.0f * sin_alpha * cos_alpha;
                if (sigma < 1e-3f) Lambda = 0.0f;

                float enc_w = std::max(r1.x() + r1.width(), r2.x() + r2.width()) - std::min(r1.x(), r2.x()) + offset;
                float enc_h = std::max(r1.y() + r1.height(), r2.y() + r2.height()) - std::min(r1.y(), r2.y()) + offset;

                float gamma = 2.0f - Lambda;
                // Explicit product, not std::pow(x, 2): pow's integer-exponent overload promotes to double and narrows back (C4244).
                float dx_norm = (cx1 - cx2) / std::max(1e-6f, enc_w);
                float dy_norm = (cy1 - cy2) / std::max(1e-6f, enc_h);
                float rho_x = dx_norm * dx_norm;
                float rho_y = dy_norm * dy_norm;
                float Delta = (1.0f - std::exp(-gamma * rho_x)) + (1.0f - std::exp(-gamma * rho_y));

                float omega_w = std::abs(r1.width() - r2.width()) / std::max({1e-6f, r1.width(), r2.width()});
                float omega_h = std::abs(r1.height() - r2.height()) / std::max({1e-6f, r1.height(), r2.height()});
                float Omega = std::pow(1.0f - std::exp(-omega_w), 4.0f) + std::pow(1.0f - std::exp(-omega_h), 4.0f);

                float siou = iou - (Delta + Omega) * 0.5f;
                out_matrix(i, j) = 1.0f - siou;
            }
        }
    }

    float SIoUCostCalculator::calculate_single(const TrackletPtr& a, const TrackletPtr& b) const
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
        if (iw > 0)
        {
            float ih = std::min(r1.y() + r1.height(), r2.y() + r2.height())
                    - std::max(r1.y(), r2.y()) + offset;
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
        float Lambda = 2.0f * sin_alpha * cos_alpha;
        if (sigma < 1e-3f) Lambda = 0.0f;

        float enc_w = std::max(r1.x() + r1.width(), r2.x() + r2.width()) - std::min(r1.x(), r2.x()) + offset;
        float enc_h = std::max(r1.y() + r1.height(), r2.y() + r2.height()) - std::min(r1.y(), r2.y()) + offset;

        float gamma = 2.0f - Lambda;
        // Explicit product, not std::pow(x, 2): pow's integer-exponent overload promotes to double and narrows back (C4244).
        float dx_norm = (cx1 - cx2) / std::max(1e-6f, enc_w);
        float dy_norm = (cy1 - cy2) / std::max(1e-6f, enc_h);
        float rho_x = dx_norm * dx_norm;
        float rho_y = dy_norm * dy_norm;
        float Delta = (1.0f - std::exp(-gamma * rho_x)) + (1.0f - std::exp(-gamma * rho_y));

        float omega_w = std::abs(r1.width() - r2.width()) / std::max({1e-6f, r1.width(), r2.width()});
        float omega_h = std::abs(r1.height() - r2.height()) / std::max({1e-6f, r1.height(), r2.height()});
        float Omega = std::pow(1.0f - std::exp(-omega_w), 4.0f) + std::pow(1.0f - std::exp(-omega_h), 4.0f);

        float siou = iou - (Delta + Omega) * 0.5f;
        return 1.0f - siou;
    }

    /**
     * @brief Computes association costs using visual feature cosine distance.
     * @param a Input collection of source tracklets for association.
     * @param b Input collection of candidate tracklets for association.
     * @param out_matrix Reference to the cost matrix for result population.
     */
    void CosineCostCalculator::calculate(const std::vector<TrackletPtr>& __restrict a,
                                        const std::vector<TrackletPtr>& __restrict b,
                                        CostMatrix& __restrict out_matrix)
    {
        out_matrix.resize(a.size(), b.size(), 1.0f);
        for (size_t i = 0; i < a.size(); ++i)
        {
            for (size_t j = 0; j < b.size(); ++j)
            {
                if (!a[i]->hasFeature() || !b[j]->hasFeature())
                {
                    out_matrix(i, j) = 1.0f;
                    continue;
                }
                float max_sim = a[i]->getMaxFeatureSimilarity(b[j]->getCurrFeat());
                out_matrix(i, j) = std::max(0.0f, 1.0f - max_sim);
            }
        }
    }

    /**
     * @brief Computes the association cost for a single pair using cosine distance.
     * @param a Source tracklet for association.
     * @param b Candidate tracklet for association.
     * @return Calculated visual feature distance.
     */
    float CosineCostCalculator::calculate_single(const TrackletPtr& a, const TrackletPtr& b) const
    {
        if (!a->hasFeature() || !b->hasFeature()) return 1.0f;
        return std::max(0.0f, 1.0f - a->getMaxFeatureSimilarity(b->getCurrFeat()));
    }

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
        if (a.empty() || b.empty()) return;

        for (size_t i = 0; i < a.size(); ++i)
        {
            // Per-row state policy: Stationary leans into IoU, Maneuvering/Coasting into appearance, Coasting also rejects geometry-only fallback.
            const StateAssocMods sm = stateAssocMods(a[i]->getLastSeenState());
            const float row_iou_weight = std::clamp(iou_weight_ - sm.reid_shift, 0.0f, 1.0f);

            for (size_t j = 0; j < b.size(); ++j)
            {
                float cost = 1e4f;

                if (a[i]->hasFeature() && b[j]->hasFeature())
                {
                    float reid_sim = a[i]->getMaxFeatureSimilarity(b[j]->getCurrFeat());
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
    float ReIDFirstCostCalculator::calculate_single(const TrackletPtr& a, const TrackletPtr& b) const
    {
        // Mirror batch calculate() row policy so single-pair path is regime-consistent.
        const StateAssocMods sm = stateAssocMods(a->getLastSeenState());
        const float row_iou_weight = std::clamp(iou_weight_ - sm.reid_shift, 0.0f, 1.0f);

        if (a->hasFeature() && b->hasFeature())
        {
            float reid_sim = a->getMaxFeatureSimilarity(b->getCurrFeat());
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

    /**
     * @brief Computes association costs strictly based on visual feature vectors.
     * @param a Input collection of source tracklets for association.
     * @param b Input collection of candidate tracklets for association.
     * @param out_matrix Reference to the cost matrix for result population.
     */
    void PureReIDCostCalculator::calculate(const std::vector<TrackletPtr>& __restrict a,
                                        const std::vector<TrackletPtr>& __restrict b,
                                        CostMatrix& __restrict out_matrix)
    {
        out_matrix.resize(a.size(), b.size(), 1e4f);
        if (a.empty() || b.empty()) return;

        meas_workspace_.clear();
        score_workspace_.clear();
        if (gate_validator_)
        {
            meas_workspace_.reserve(b.size());
            score_workspace_.reserve(b.size());
            for (const auto& det : b)
            {
                meas_workspace_.push_back(det->getMeasurement());
                score_workspace_.push_back(det->getScore());
            }
        }

        size_t gated_pairs = 0;

        for (size_t i = 0; i < a.size(); ++i)
        {
            if (!a[i]->hasFeature()) continue;

            std::vector<bool> in_gate;
            if (gate_validator_)
            {
                in_gate = gate_validator_->getGateFlags(
                    a[i]->getMean(), a[i]->getCovariance(), meas_workspace_,
                    gating_thresh_, score_workspace_);
            }

            for (size_t j = 0; j < b.size(); ++j)
            {
                if (!b[j]->hasFeature()) continue;

                if (gate_validator_ && !in_gate[j])
                {
                    ++gated_pairs;
                    out_matrix(i, j) = 1e4f;
                    continue;
                }

                float reid_sim = a[i]->getMaxFeatureSimilarity(b[j]->getCurrFeat());
                float reid_dist = 1.0f - reid_sim;

                if (reid_dist < reid_thresh_)
                    out_matrix(i, j) = reid_dist;
            }
        }

        if (gated_pairs > 0)
        {
            SUSHITRACK_LOG_DEBUG("[Distance] PureReID: gated out %zu of %zu pairs (Mahalanobis).",
                gated_pairs, a.size() * b.size());
        }
    }

    /**
     * @brief Computes association cost for a single pair using strictly visual features.
     * @param a Source tracklet for association.
     * @param b Candidate tracklet for association.
     * @return Calculated association cost based on visual features.
     */
    float PureReIDCostCalculator::calculate_single(const TrackletPtr& a, const TrackletPtr& b) const
    {
        if (!a->hasFeature() || !b->hasFeature())
        {
            return 1e4f;
        }

        if (gate_validator_)
        {
            KalmanTypes::DetectBoxVector meas_workspace = { b->getMeasurement() };
            std::vector<float>           score_workspace = { b->getScore() };
            std::vector<bool> in_gate = gate_validator_->getGateFlags(
                a->getMean(), a->getCovariance(), meas_workspace,
                gating_thresh_, score_workspace);
            if (!in_gate[0])
            {
                return 1e4f;
            }
        }

        float reid_sim = a->getMaxFeatureSimilarity(b->getCurrFeat());
        float reid_dist = 1.0f - reid_sim;

        if (reid_dist < reid_thresh_)
        {
            return reid_dist;
        }

        return 1e4f;
    }

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

    /**
     * @brief Computes bounded directional bonus weight for one track/detection pair.
     * @param track Source tracklet supplying observed direction.
     * @return Bounded bonus weight.
     */
    float ObservationMomentumDecorator::effectiveWeight(const TrackletPtr& track) const
    {
        if (vdc_weight_ <= 0.0f) return 0.0f;
        if (!track->hasObsVelocity() || !track->hasLastObservation()) return 0.0f;
        if (track->getObsSpeed() < min_velocity_) return 0.0f;
        if (track->getFramesSinceObservation() > max_age_) return 0.0f;

        switch (track->getKinematicState())
        {
            case KinematicState::Cruising:    return vdc_weight_;
            case KinematicState::Maneuvering: return vdc_weight_ * maneuver_weight_scale_;
            default:                          return 0.0f;  // Stationary/Coasting: stored direction is meaningless.
        }
    }

    float ObservationMomentumDecorator::momentumBonus(const TrackletPtr& track, const TrackletPtr& det, float weight) const
    {
        if (weight <= 0.0f) return 0.0f;

        const auto& lo = track->getLastObservation();
        const float ocx = lo.x() + lo.width()  * 0.5f;
        const float ocy = lo.y() + lo.height() * 0.5f;

        const auto& dr = det->getRect();
        const float dcx = dr.x() + dr.width()  * 0.5f;
        const float dcy = dr.y() + dr.height() * 0.5f;

        const float ddx = dcx - ocx;
        const float ddy = dcy - ocy;
        const float dn  = std::sqrt(ddx * ddx + ddy * ddy);
        if (dn < 1e-3f) return 0.0f;

        const float cos_theta = (track->getObsVelocityX() * ddx + track->getObsVelocityY() * ddy) / dn;
        return std::clamp(weight * cos_theta, -weight, weight);
    }

    /**
     * @brief Computes inner costs and applies the momentum re-ranking in place.
     * @param a Collection of source tracklets.
     * @param b Collection of target tracklets (observations).
     * @param out_matrix Output cost matrix populated by inner calculator and re-ranked.
     */
    void ObservationMomentumDecorator::calculate(const std::vector<TrackletPtr>& __restrict a,
                                                 const std::vector<TrackletPtr>& __restrict b,
                                                 CostMatrix& __restrict out_matrix)
    {
        inner_->calculate(a, b, out_matrix);
        if (a.empty() || b.empty() || vdc_weight_ <= 0.0f) return;

        constexpr float kSentinel = 1e3f;

        for (size_t i = 0; i < a.size(); ++i)
        {
            // Per-row gate first so skipped tracks avoid the detection loop.
            const float weight = effectiveWeight(a[i]);
            if (weight <= 0.0f) continue;

            for (size_t j = 0; j < b.size(); ++j)
            {
                float c = out_matrix(i, j);
                if (c >= kSentinel) continue;
                out_matrix(i, j) = c - momentumBonus(a[i], b[j], weight);
            }
        }
    }

    /**
     * @brief Computes the momentum-adjusted association cost for a single pair.
     * @param a Source tracklet for association.
     * @param b Candidate tracklet for association.
     * @return Re-ranked association cost.
     */
    float ObservationMomentumDecorator::calculate_single(const TrackletPtr& a, const TrackletPtr& b) const
    {
        float c = inner_->calculate_single(a, b);
        if (c >= 1e3f) return c;
        return c - momentumBonus(a, b, effectiveWeight(a));
    }

    /**
     * @brief Initializes a GatedIoUCostCalculator instance.
     * @param gating_thresh Chi-squared threshold for Mahalanobis gating.
     * @param gate_validator Instance used for statistical gating validation.
     * @param iou_calc Spatial overlap calculator instance.
     * @param motion_cost_weight Weight assigned to the motion penalty.
     */
    GatedIoUCostCalculator::GatedIoUCostCalculator(float gating_thresh,
                                                std::shared_ptr<IGateValidator> gate_validator,
                                                std::shared_ptr<ICostCalculator> iou_calc,
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

        const bool fuse_motion = (motion_cost_weight_ > 1e-6f);
        size_t gated_pairs = 0;

        for (size_t i = 0; i < a.size(); ++i)
        {
            // Tighten the gate for Stationary, loosen for Maneuvering/Coasting.
            const StateAssocMods sm = stateAssocMods(a[i]->getLastSeenState());
            const float row_gate_thresh = std::max(1e-3f, gating_thresh_ * sm.gate_scale);
            const float gate            = row_gate_thresh;

            // Single projection pass: gate flags are derived from squared distances.
            std::vector<float> sq_dists = gate_validator_->getGateSqDistances(
                a[i]->getMean(), a[i]->getCovariance(), meas_workspace_, score_workspace_);

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
            SUSHITRACK_LOG_DEBUG("[Distance] Gating: gated out %zu of %zu pairs (Mahalanobis).",
                gated_pairs, a.size() * b.size());
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
        const StateAssocMods sm = stateAssocMods(a->getLastSeenState());
        const float row_gate_thresh = std::max(1e-3f, gating_thresh_ * sm.gate_scale);

        KalmanTypes::DetectBoxVector meas_workspace = { b->getMeasurement() };
        std::vector<float>           score_workspace = { b->getScore() };
        std::vector<float> sq_dists = gate_validator_->getGateSqDistances(
            a->getMean(), a->getCovariance(), meas_workspace, score_workspace);

        if (sq_dists[0] > row_gate_thresh)
        {
            return 1e4f;
        }

        float iou_d = iou_calc_->calculate_single(a, b);

        const bool fuse_motion = (motion_cost_weight_ > 1e-6f);
        if (fuse_motion)
        {
            float motion = sq_dists[0] / row_gate_thresh;
            if (motion > 1.0f) motion = 1.0f;

            float motion_penalty = motion_cost_weight_ * motion;
            return 1.0f - (1.0f - iou_d) * (1.0f - motion_penalty);
        }

        return iou_d;
    }

} // namespace SushiTrack