/**
 * @file      DistanceStrategies.cpp
 * @brief     Implementation of association cost calculation strategies.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#include "SushiTrack/DistanceStrategies.h"
#include <algorithm>
#include "SushiTrack/Logger.h"
#include <cmath>
#include <deque>

namespace SushiTrack
{
    /**
     * @brief Computes association costs using Intersection over Union (IoU).
     * 
     * Populates the cost matrix with values derived from the overlap ratio of 
     * bounding box pairs. Applies a maximum area ratio constraint to filter 
     * candidate pairs.
     * 
     * @param a Input collection of source tracklets for association.
     * @param b Input collection of candidate tracklets for association.
     * @param out_matrix Reference to the cost matrix for result population.
     */
    void IoUCostCalculator::calculate(const std::vector<TrackletPtr>& __restrict a,
                                    const std::vector<TrackletPtr>& __restrict b,
                                    CostMatrix& __restrict out_matrix)
    {
        out_matrix.resize(a.size(), b.size(), 1e4f);
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
     * 
     * Incorporates normalized centroid distance into the IoU-based cost 
     * calculation to account for spatial alignment of bounding boxes.
     * 
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

    /**
     * @brief Computes association costs using visual feature cosine distance.
     * 
     * Calculates angular distance between high-dimensional feature vectors 
     * extracted from detection observations.
     * 
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

    float CosineCostCalculator::calculate_single(const TrackletPtr& a, const TrackletPtr& b) const
    {
        if (!a->hasFeature() || !b->hasFeature()) return 1.0f;
        return std::max(0.0f, 1.0f - a->getMaxFeatureSimilarity(b->getCurrFeat()));
    }

    /**
     * @brief Implements a prioritized association strategy using visual similarity.
     * 
     * Evaluates visual distance and applies spatial overlap fallback only when 
     * visual confidence is below the configured threshold.
     * 
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
                    cost = (1.0f - iou_weight_) * reid_dist + iou_weight_ * iou_d;
                    out_matrix(i, j) = cost;
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
     * 
     * Applies the ReID-first logic to a single pair: evaluates visual distance, applies ReID 
     * thresholding, and conditionally incorporates spatial overlap.
     * 
     * @param a Source tracklet for association.
     * @param b Candidate tracklet for association.
     * @return float Calculated association cost.
     */
    float ReIDFirstCostCalculator::calculate_single(const TrackletPtr& a, const TrackletPtr& b) const
    {
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
            return (1.0f - iou_weight_) * reid_dist + iou_weight_ * iou_d;
        }

        if (iou_calc_)
        {
            return iou_calc_->calculate_single(a, b);
        }

        return 1e4f;
    }

    /**
     * @brief Computes association costs strictly based on visual feature vectors.
     * 
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
        if (gate_validator_)
        {
            for (const auto& det : b)
                meas_workspace_.push_back(det->getMeasurement());
        }

        for (size_t i = 0; i < a.size(); ++i)
        {
            if (!a[i]->hasFeature()) continue;

            std::vector<bool> in_gate;
            if (gate_validator_)
            {
                in_gate = gate_validator_->getGateFlags(
                    a[i]->getMean(), a[i]->getCovariance(), meas_workspace_, gating_thresh_);
            }

            for (size_t j = 0; j < b.size(); ++j)
            {
                if (!b[j]->hasFeature()) continue;

                if (gate_validator_ && !in_gate[j])
                {
                    SUSHITRACK_LOG_DEBUG("[Distance] PureReID: Track %llu gated out detection %zu (Mahalanobis).", (unsigned long long)a[i]->getTrackId(), j);
                    out_matrix(i, j) = 1e4f;
                    continue;
                }

                float reid_sim = a[i]->getMaxFeatureSimilarity(b[j]->getCurrFeat());
                float reid_dist = 1.0f - reid_sim;

                if (reid_dist < reid_thresh_)
                    out_matrix(i, j) = reid_dist;
            }
        }
    }

    /**
     * @brief Computes the association cost for a single pair using strictly visual features.
     * 
     * Evaluates gating conditions if a gate validator is present, then calculates and thresholds 
     * the visual feature distance.
     * 
     * @param a Source tracklet for association.
     * @param b Candidate tracklet for association.
     * @return float Calculated association cost based on visual features.
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
            std::vector<bool> in_gate = gate_validator_->getGateFlags(
                a->getMean(), a->getCovariance(), meas_workspace, gating_thresh_);
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
     * 
     * Integrates IoU and cosine distance metrics subject to Mahalanobis 
     * distance validation gating.
     * 
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
        for (const auto& det : b)
            meas_workspace_.push_back(det->getMeasurement());

        const float gate = std::max(1e-3f, gating_thresh_);

        for (size_t i = 0; i < a.size(); ++i)
        {
            std::vector<bool> in_gate = gate_validator_->getGateFlags(
                a[i]->getMean(), a[i]->getCovariance(), meas_workspace_, gating_thresh_);

            std::vector<float> sq_dists = gate_validator_->getGateSqDistances(
                a[i]->getMean(), a[i]->getCovariance(), meas_workspace_);

            for (size_t j = 0; j < b.size(); ++j)
            {
                // Validation gate: Mahalanobis distance exceeding the chi-squared threshold 
                // results in a sentinel cost penalty.
                if (!in_gate[j])
                {
                    SUSHITRACK_LOG_DEBUG("[Distance] Gating: Track %llu gated out detection %zu (Mahalanobis).", (unsigned long long)a[i]->getTrackId(), j);
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
                        out_matrix(i, j) = iou_weight_ * motion_d + (1.0f - iou_weight_) * cos_d;
                    }
                }
                else
                {
                    out_matrix(i, j) = motion_d;
                }
            }
        }
    }

    /**
     * @brief Computes the aggregated spatial and visual association cost for a single pair.
     * 
     * Applies Mahalanobis distance validation gating, then integrates spatial overlap (IoU) and 
     * visual similarity based on configured weights.
     * 
     * @param a Source tracklet for association.
     * @param b Candidate tracklet for association.
     * @return float Aggregated association cost.
     */
    float FusedCostCalculator::calculate_single(const TrackletPtr& a, const TrackletPtr& b) const
    {
        KalmanTypes::DetectBoxVector meas_workspace = { b->getMeasurement() };
        std::vector<bool> in_gate = gate_validator_->getGateFlags(
            a->getMean(), a->getCovariance(), meas_workspace, gating_thresh_);

        if (!in_gate[0])
        {
            return 1e4f;
        }

        std::vector<float> sq_dists = gate_validator_->getGateSqDistances(
            a->getMean(), a->getCovariance(), meas_workspace);
        const float gate = std::max(1e-3f, gating_thresh_);
        float motion_d = sq_dists[0] / gate;

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
            return iou_weight_ * motion_d + (1.0f - iou_weight_) * cos_d;
        }
        
        return motion_d;
    }

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
     * 
     * Incorporates statistical gating into the IoU-based cost matrix to restrict 
     * the assignment search space based on motion prediction uncertainty.
     * 
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
        meas_workspace_.reserve(b.size());
        for (const auto& det : b)
            meas_workspace_.push_back(det->getMeasurement());

        const bool fuse_motion = (motion_cost_weight_ > 1e-6f);
        const float gate = std::max(1e-3f, gating_thresh_);

        for (size_t i = 0; i < a.size(); ++i)
        {
            std::vector<bool> in_gate = gate_validator_->getGateFlags(
                a[i]->getMean(), a[i]->getCovariance(), meas_workspace_, gating_thresh_);

            std::vector<float> sq_dists;
            if (fuse_motion)
                sq_dists = gate_validator_->getGateSqDistances(
                    a[i]->getMean(), a[i]->getCovariance(), meas_workspace_);

            for (size_t j = 0; j < b.size(); ++j)
            {
                // Validation gate: Mahalanobis distance exceeding the chi-squared threshold 
                // results in a sentinel cost penalty.
                if (!in_gate[j])
                {
                    SUSHITRACK_LOG_DEBUG("[Distance] Gating: Track %llu gated out detection %zu (Mahalanobis).", (unsigned long long)a[i]->getTrackId(), j);
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
    }

    /**
     * @brief Computes the spatial overlap cost with motion-based gating for a single pair.
     * 
     * Applies statistical gating and calculates the IoU cost. Conditionally fuses a normalized 
     * motion prediction penalty.
     * 
     * @param a Source tracklet for association.
     * @param b Candidate tracklet for association.
     * @return float Calculated spatial overlap cost with gating penalties.
     */
    float GatedIoUCostCalculator::calculate_single(const TrackletPtr& a, const TrackletPtr& b) const
    {
        KalmanTypes::DetectBoxVector meas_workspace = { b->getMeasurement() };
        std::vector<bool> in_gate = gate_validator_->getGateFlags(
            a->getMean(), a->getCovariance(), meas_workspace, gating_thresh_);

        if (!in_gate[0])
        {
            return 1e4f;
        }

        float iou_d = iou_calc_->calculate_single(a, b);

        const bool fuse_motion = (motion_cost_weight_ > 1e-6f);
        if (fuse_motion)
        {
            std::vector<float> sq_dists = gate_validator_->getGateSqDistances(
                a->getMean(), a->getCovariance(), meas_workspace);
            
            const float gate = std::max(1e-3f, gating_thresh_);
            float motion = sq_dists[0] / gate;
            if (motion > 1.0f) motion = 1.0f;

            float motion_penalty = motion_cost_weight_ * motion;
            return 1.0f - (1.0f - iou_d) * (1.0f - motion_penalty);
        }

        return iou_d;
    }

} // namespace SushiTrack