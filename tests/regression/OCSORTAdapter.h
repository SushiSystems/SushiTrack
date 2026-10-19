/**************************************************************************/
/* OCSORTAdapter.h                                                        */
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
 * @file OCSORTAdapter.h
 * @brief Defines the regression test adapter for OC-SORT.
 * @author Mustafa Garip
 */

#pragma once

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <Eigen/Dense>

#include "OCSort.hpp"
#include "TestInfra.h"
#include "TrackerAdapter.h"

namespace TestInfrastructure
{
    /**
     * @brief Implementation of TrackerAdapter for the OC-SORT algorithm.
     *
     * Interfaces with the OC-SORT library to provide a standardized update
     * mechanism for benchmarking.
     */
    class OCSORTAdapter final : public TrackerAdapter
    {
        public:
            /**
             * @brief Initializes the adapter and loads configuration.
             *
             * @param config_path Optional path to a JSON configuration file.
             */
            explicit OCSORTAdapter(const std::string& config_path = "")
                : det_thresh_(0.4f),
                  max_age_(30),
                  min_hits_(3),
                  iou_threshold_(0.3f),
                  delta_t_(3),
                  inertia_(0.2f),
                  use_byte_(false)
            {
                if (!config_path.empty() && std::filesystem::exists(config_path))
                {
                    JsonDoc doc = parse_json(config_path);
                    auto it = doc.find("ocsort");
                    if (it != doc.end())
                    {
                        const JsonSection& s = it.value();
                        det_thresh_ = json_get(s, "det_thresh", det_thresh_);
                        max_age_ =
                            static_cast<int>(json_get(s, "max_age", static_cast<float>(max_age_)));
                        min_hits_ = static_cast<int>(
                            json_get(s, "min_hits", static_cast<float>(min_hits_)));
                        iou_threshold_ = json_get(s, "iou_threshold", iou_threshold_);
                        delta_t_ =
                            static_cast<int>(json_get(s, "delta_t", static_cast<float>(delta_t_)));
                        inertia_ = json_get(s, "inertia", inertia_);
                        use_byte_ = json_get(s, "use_byte", 0.0f) > 0.5f;
                        std::cout << "[ OCSORTAdapter ] config loaded from " << config_path << "\n";
                    }
                }

                tracker_ = std::make_unique<ocsort::OCSort>(det_thresh_, max_age_, min_hits_,
                                                            iou_threshold_, delta_t_, "iou",
                                                            inertia_, use_byte_);
            }

            /**
             * @brief Retrieves the tracker name.
             *
             * @return "ocsort"
             */
            std::string name() const override { return "ocsort"; }

            /**
             * @brief Processes detections using the OC-SORT algorithm.
             *
             * @param dets Collection of input detections.
             * @param dt Time interval (unused).
             * @return Collection of tracking results.
             */
            std::vector<TrackResult> update(const std::vector<MOTDetection>& dets,
                                            float dt = 1.0f) override
            {
                if (dets.empty())
                {
                    // Update with empty matrix if necessary, but ocsort might handle empty input
                    Eigen::MatrixXf empty_dets(0, 6);
                    tracker_->update(empty_dets);
                    return {};
                }

                Eigen::MatrixXf eigen_dets(dets.size(), 6);
                for (size_t i = 0; i < dets.size(); ++i)
                {
                    eigen_dets(i, 0) = dets[i].x;
                    eigen_dets(i, 1) = dets[i].y;
                    eigen_dets(i, 2) = dets[i].x + dets[i].w;
                    eigen_dets(i, 3) = dets[i].y + dets[i].h;
                    eigen_dets(i, 4) = dets[i].score;
                    eigen_dets(i, 5) = static_cast<float>(dets[i].label);
                }

                auto tracks = tracker_->update(eigen_dets);

                std::vector<TrackResult> results;
                results.reserve(tracks.size());
                for (const auto& t : tracks)
                {
                    TrackResult r;
                    // OC-SORT returns [x1, y1, x2, y2, track_id, seed_id, label]
                    r.x = t(0);
                    r.y = t(1);
                    r.w = t(2) - t(0);
                    r.h = t(3) - t(1);
                    r.track_id = static_cast<int>(t(4));
                    r.score = 1.0f; // Score not explicitly returned in the same way
                    results.push_back(r);
                }
                return results;
            }

        private:
            float det_thresh_;
            int max_age_;
            int min_hits_;
            float iou_threshold_;
            int delta_t_;
            float inertia_;
            bool use_byte_;

            std::unique_ptr<ocsort::OCSort> tracker_;
    };

} // namespace TestInfrastructure
