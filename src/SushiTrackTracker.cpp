/**
 * @file      SushiTrackTracker.cpp
 * @brief     Implementation of the core tracking algorithm orchestration.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#include "SushiTrack/SushiTrackTracker.h"
#include "SushiTrack/KalmanFilter.h"
#include "SushiTrack/DistanceStrategies.h"
#include "SushiTrack/AssignmentSolvers.h"
#include "SushiTrack/TimingPolicies.h"
#include "SushiTrack/InteractionGraph.h"
#include "SushiTrack/Logger.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>

namespace SushiTrack
{
    /**
     * @brief Initializes a new instance of the SushiTrackTracker.
     * 
     * @param config Configuration parameters for the tracking algorithm.
     * @param kf_factory Factory for generating Kalman filter instances.
     * @param solver Implementation of the bipartite assignment solver.
     * @param high_conf_metric Metric for high-confidence track-to-detection matching.
     * @param low_conf_metric Metric for low-confidence track-to-detection matching.
     * @param tentative_metric Metric for matching unconfirmed tentative tracks.
     * @param duplicate_metric Metric for identifying redundant tracklets.
     * @param lost_recovery_metric Metric for visual-based lost track recovery.
     */
    SushiTrackTracker::SushiTrackTracker(const TrackerConfig& config,
                                    std::shared_ptr<IKalmanFactory> kf_factory,
                                    std::unique_ptr<IAssignmentSolver> solver,
                                    std::shared_ptr<ICostCalculator> high_conf_metric,
                                    std::shared_ptr<ICostCalculator> low_conf_metric,
                                    std::shared_ptr<ICostCalculator> tentative_metric,
                                    std::shared_ptr<ICostCalculator> duplicate_metric,
                                    std::shared_ptr<ICostCalculator> lost_recovery_metric)
        : config_(config),
        timing_policy_(config.time_aware
            ? std::shared_ptr<ITimingPolicy>(std::make_shared<WallClockTimingPolicy>(config.frame_rate))
            : std::shared_ptr<ITimingPolicy>(std::make_shared<FrameTimingPolicy>())),
        kf_factory_(kf_factory),
        solver_(std::move(solver)),
        high_conf_metric_(high_conf_metric),
        low_conf_metric_(low_conf_metric),
        tentative_metric_(tentative_metric),
        duplicate_metric_(duplicate_metric),
        lost_recovery_metric_(lost_recovery_metric),
        frame_id_(0),
        track_id_count_(0),
        current_visit_id_(0)
    {
        // The interaction graph is instantiated only when the feature is
        // explicitly enabled. Keeping the pointer null when disabled allows
        // every downstream consumer to short-circuit on a single pointer
        // check, which is both cheaper and easier to reason about than a
        // dedicated boolean flag duplicated alongside the pointer.
        if (config_.enable_interaction_context)
        {
            interaction_graph_ = std::make_unique<BottomYInteractionGraph>(
                config_.interaction_iou_thresh);
        }
    }

    /**
     * @brief Generates a new state predictor instance using the configured factory.
     * 
     * @return Unique pointer to the initialized state predictor.
     */
    std::unique_ptr<IStatePredictor> SushiTrackTracker::createKalmanFilter() const
    {
        return kf_factory_->create();
    }

    namespace
    {
        /**
         * @brief Returns the validity-window multiplier for an occluder's
         *        kinematic state.
         *
         * A stationary occluder stays where it was at snapshot time, so the
         * validity window can be extended. A maneuvering occluder may have
         * moved out of frame, so the window contracts. Cruising is neutral;
         * coasting is slightly contracted to reflect reduced positional
         * confidence.
         *
         * Applied identically in applyOccluderPenalty() and cleanupLost()
         * so the penalty and the snapshot expiry share the same effective
         * window.
         *
         * @param s Occluder kinematic state.
         * @return Multiplier applied to interaction_occluder_validity_seconds.
         */
        inline float validityScale(KinematicState s)
        {
            switch (s)
            {
                case KinematicState::Stationary:  return 1.5f;
                case KinematicState::Cruising:    return 1.0f;
                case KinematicState::Maneuvering: return 0.5f;
                case KinematicState::Coasting:    return 0.7f;
            }
            return 1.0f;
        }

        /**
         * @brief Returns the penalty multiplier driven by the occludee's
         *        kinematic state.
         *
         * A stationary occludee will reappear at roughly the same position,
         * so stronger suppression reduces identity churn. A maneuvering
         * occludee may emerge well away from the occluder, so lighter
         * suppression avoids rejecting a legitimate reacquisition. Cruising
         * is neutral; coasting is slightly reduced.
         *
         * @param s Occludee kinematic state.
         * @return Penalty multiplier applied to interaction_occluder_penalty.
         */
        inline float occludeeStateScale(KinematicState s)
        {
            switch (s)
            {
                case KinematicState::Stationary:  return 1.3f;
                case KinematicState::Cruising:    return 1.0f;
                case KinematicState::Maneuvering: return 0.6f;
                case KinematicState::Coasting:    return 0.8f;
            }
            return 1.0f;
        }

        /**
         * @brief Returns the penalty multiplier driven by velocity alignment
         *        between the occludee and the occluder.
         *
         * When both tracks share the same heading the occlusion is likely to
         * persist, so the multiplier is increased. When they are heading in
         * opposite directions the occluder is sweeping past and the occludee
         * is about to reappear, so the multiplier is reduced. The cosine of
         * the angle between the two unit velocity vectors is mapped to
         * [0.6, 1.4] via 1 + 0.4 * cos(θ). When either velocity is below
         * the noise floor the function returns 1.0 (identity).
         *
         * @param vax X-component of the occludee velocity (px/s).
         * @param vay Y-component of the occludee velocity (px/s).
         * @param vbx X-component of the occluder velocity (px/s).
         * @param vby Y-component of the occluder velocity (px/s).
         * @return Alignment multiplier in the closed range [0.6, 1.4].
         */
        inline float velocityAlignmentScale(float vax, float vay,
                                            float vbx, float vby)
        {
            const float na = std::sqrt(vax * vax + vay * vay);
            const float nb = std::sqrt(vbx * vbx + vby * vby);
            // When either velocity is below the noise floor the alignment
            // signal is meaningless. Falling back to the identity multiplier
            // preserves the configured base penalty.
            if (na < 1e-3f || nb < 1e-3f) return 1.0f;
            const float dot = (vax * vbx + vay * vby) / (na * nb);
            // Map the cosine into a multiplicative factor centred at one
            // with an amplitude of 0.4. Aligned vectors strengthen the
            // penalty; anti-aligned vectors weaken it.
            return 1.0f + 0.4f * dot;
        }
    }

    /**
     * @brief Applies the occluder-aware cost penalty to a cost matrix.
     *
     * For each row with a valid occluder snapshot, every detection column
     * whose bounding box overlaps the snapshotted occluder geometry above
     * the configured DIoU threshold receives an additive penalty.
     *
     * Snapshot validity is evaluated using interaction_occluder_validity_seconds
     * scaled by validityScale() for the occluder's kinematic state. In
     * frame-based mode the seconds value is converted to frames via frame_rate.
     * This matches the expiry logic in cleanupLost() exactly, so the penalty
     * stops firing on the same frame the snapshot is cleared.
     *
     * The penalty magnitude is further modulated by the occludee's kinematic
     * state (occludeeStateScale) and the heading alignment between occludee
     * and occluder (velocityAlignmentScale). Both multipliers are bounded and
     * reduce to identity when kinematic information is unavailable.
     *
     * Cells already at the sentinel cost are left untouched.
     *
     * @param tracks    Track rows of the cost matrix.
     * @param dets      Detection columns of the cost matrix.
     * @param matrix    Cost matrix to be modified in place.
     * @param only_lost When true, only rows in the Lost state are considered.
     */
    void SushiTrackTracker::applyOccluderPenalty(const std::vector<TrackletPtr>& tracks,
                                                const std::vector<TrackletPtr>& dets,
                                                CostMatrix& matrix,
                                                bool only_lost) const
    {
        if (!interaction_graph_) return;
        if (config_.interaction_occluder_penalty <= 0.0f) return;
        if (tracks.empty() || dets.empty()) return;

        constexpr float kSentinel = 1e3f;
        const int reference_fps = std::max(1, config_.frame_rate);

        auto snapshot_valid = [&](const TrackletPtr& t) -> bool
        {
            if (t->getOccluderId() < 0) return false;
            const size_t occ_id = static_cast<size_t>(t->getOccluderId());
            if (!interaction_graph_->contains(occ_id))
                return false;

            // Bridge 2: scale the validity window by the occluder's
            // kinematic regime. A stationary occluder is a reliable visual
            // obstacle long after the snapshot was taken; a maneuvering
            // occluder is not.
            const float scale = validityScale(
                interaction_graph_->getKinematicState(occ_id));

            if (config_.time_aware)
            {
                const float elapsed = t->getAccumulatedLostTime() - t->getOccluderSnapshotTime();
                return elapsed <= config_.interaction_occluder_validity_seconds * scale;
            }
            const size_t snap_frame = t->getOccluderSnapshotFrame();
            if (frame_id_ < snap_frame) return false;
            const size_t elapsed_frames = frame_id_ - snap_frame;
            // Convert the seconds-based validity window to frames and apply
            // the kinematic scale, matching the cleanupLost expiry path exactly.
            const float scaled_frames =
                config_.interaction_occluder_validity_seconds
                * static_cast<float>(std::max(1, config_.frame_rate))
                * scale;
            return elapsed_frames <= static_cast<size_t>(std::lround(scaled_frames));
        };

        for (size_t ti = 0; ti < tracks.size(); ++ti)
        {
            const auto& t = tracks[ti];
            if (!t) continue;
            if (only_lost && t->getTrackState() != TrackState::Lost) continue;
            if (!snapshot_valid(t)) continue;

            const size_t occ_id = static_cast<size_t>(t->getOccluderId());

            // Bridge 3a: penalty multiplier from the occludee's own
            // kinematic regime, captured by the FSM at the time the
            // tracklet went Lost.
            const float state_factor = occludeeStateScale(t->getLastSeenState());

            // Bridge 3b: penalty multiplier from the alignment between the
            // occludee's and the occluder's velocity vectors. Velocities
            // are stored in the Kalman state in per-frame units; the
            // multiplier itself depends only on the relative direction, so
            // the frame-rate scaling cancels out. We still convert into
            // per-second units to make the noise-floor check readable.
            const auto t_mean = t->getMean();
            const float t_vx = t_mean(4) * static_cast<float>(reference_fps);
            const float t_vy = t_mean(5) * static_cast<float>(reference_fps);

            // The occluder velocity is read from the live Tracklet pool
            // rather than the graph snapshot: the snapshot only captured
            // bounding-box geometry and kinematic state, not raw velocity.
            // Locating the occluder by identifier across the active and
            // lost pools keeps the bridge consistent regardless of which
            // pool the occluder currently resides in.
            float occ_vx = 0.0f, occ_vy = 0.0f;
            bool occ_velocity_known = false;
            auto find_occluder = [&](const std::vector<TrackletPtr>& pool) -> bool
            {
                for (const auto& p : pool)
                {
                    if (p && p->getTrackId() == occ_id)
                    {
                        const auto m = p->getMean();
                        occ_vx = m(4) * static_cast<float>(reference_fps);
                        occ_vy = m(5) * static_cast<float>(reference_fps);
                        return true;
                    }
                }
                return false;
            };
            occ_velocity_known = find_occluder(tracked_tracklets_)
                              || find_occluder(lost_tracklets_);

            const float alignment_factor = occ_velocity_known
                ? velocityAlignmentScale(t_vx, t_vy, occ_vx, occ_vy)
                : 1.0f;

            const float row_penalty = config_.interaction_occluder_penalty
                                    * state_factor
                                    * alignment_factor;

            for (size_t dj = 0; dj < dets.size(); ++dj)
            {
                float c = matrix(ti, dj);
                if (c >= kSentinel) continue;

                const auto& r = dets[dj]->getRect();
                const float overlap = interaction_graph_->occluderOverlap(
                    occ_id, r.x(), r.y(), r.width(), r.height());

                if (overlap >= config_.interaction_iou_thresh)
                {
                    matrix(ti, dj) = c + row_penalty;
                    SUSHITRACK_LOG_DEBUG(
                        "[Tracker] Occluder penalty: track %llu vs det %zu "
                        "overlaps occluder %llu (DIoU=%.2f), "
                        "state_factor=%.2f alignment_factor=%.2f, "
                        "cost %.3f -> %.3f.",
                        (unsigned long long)t->getTrackId(),
                        dj,
                        (unsigned long long)occ_id,
                        overlap, state_factor, alignment_factor,
                        c, matrix(ti, dj));
                }
            }
        }
    }

    /**
     * @brief Updates the tracker state with a new set of observations.
     * 
     * Orchestrates track prediction, multi-stage matching (high-confidence, 
     * low-confidence, lost recovery, and tentative), and track lifecycle 
     * management for the current timestamp.
     * 
     * @param objects Collection of detected objects in the current frame.
     * @param dt Time interval elapsed since the previous update.
     * @return Collection of tracklets across all lifecycle states (Tracked, Lost, Tentative, Removed).
     */
    std::vector<SushiTrackTracker::TrackletPtr>
    SushiTrackTracker::update(const ObjectVector& objects, float dt)
    {
        float effective_dt = dt;
        if (!config_.time_aware)
        {
            effective_dt = 1.0f / static_cast<float>(std::max(1, config_.frame_rate));
        }

        SUSHITRACK_LOG_DEBUG("[Tracker] Frame %llu: Updating with %zu detections, dt=%.4f", 
            (unsigned long long)frame_id_ + 1, objects.size(), effective_dt);

        frame_id_++;

        std::vector<TrackletPtr> activated_tracklets;
        std::vector<TrackletPtr> refind_tracklets;
        std::vector<TrackletPtr> lost_tracklets_curr;
        std::vector<TrackletPtr> removed_tracklets_curr;

        std::vector<TrackletPtr> det_high;
        std::vector<TrackletPtr> det_low;

        for (const auto& obj : objects)
        {
            TrackletPtr tracklet = std::make_shared<Tracklet>(nullptr, obj.rect, obj.label, obj.prob, obj.feature, config_.feature_momentum, config_.enable_xyah, config_.max_feature_history);

            tracklet->setTimingPolicy(timing_policy_);

            if (config_.enable_fsm)
                tracklet->setFSM(std::make_unique<TrackFSM>(config_));

            if (obj.prob >= config_.track_thresh)
                det_high.push_back(tracklet);
            else
                det_low.push_back(tracklet);
        }

        std::vector<TrackletPtr> tracklet_pool;
        std::vector<TrackletPtr> tentative_pool;
        std::vector<TrackletPtr> track_remaining;
        std::vector<TrackletPtr> det_high_remaining;
        std::vector<TrackletPtr> det_for_new_init;
        std::vector<TrackletPtr> det_low_remaining;
        std::vector<TrackletPtr> non_active_tracklets;

        if (config_.enable_tentative)
        {
            predictTracks(tracklet_pool, tentative_pool, effective_dt);
        }
        else
        {
            std::vector<TrackletPtr> active_tracklets;
            for (auto& s : tracked_tracklets_)
            {
                if (s->isConfirmed())
                    active_tracklets.push_back(s);
                else
                    non_active_tracklets.push_back(s);
            }
            tracklet_pool = jointTracklets(active_tracklets, lost_tracklets_);

            for (auto& s : tracklet_pool)
                s->predict(effective_dt);
            for (auto& s : non_active_tracklets)
                s->predict(effective_dt);
        }

        SUSHITRACK_LOG_DEBUG("[Tracker] Pools: tracklet=%zu, tentative=%zu, non_active=%zu",
            tracklet_pool.size(), tentative_pool.size(), non_active_tracklets.size());

        // Build the scene-level interaction graph once per frame, immediately
        // after propagation and before any association decision is taken. The
        // graph is computed over the union of confirmed and lost tracklets so
        // that occluder snapshots taken inside matchLowConfidence and queried
        // inside matchLostRecovery refer to the same world view.
        if (interaction_graph_)
        {
            interaction_graph_->build(tracklet_pool);
        }

        std::vector<TrackletPtr> active_pool;
        std::vector<TrackletPtr> lost_pool;
        
        if (lost_recovery_metric_)
        {
            for (auto& t : tracklet_pool)
            {
                if (t->getTrackState() == TrackState::Tracked)
                    active_pool.push_back(t);
                else
                    lost_pool.push_back(t);
            }
        }
        else
        {
            active_pool = tracklet_pool;
        }

        matchHighConfidence(active_pool, det_high,
                            activated_tracklets, refind_tracklets,
                            track_remaining, det_high_remaining);

        SUSHITRACK_LOG_DEBUG("[Tracker] High-conf match: matches=%zu, remaining_tracks=%zu, remaining_dets=%zu",
            activated_tracklets.size() + refind_tracklets.size(), track_remaining.size(), det_high_remaining.size());

        matchLowConfidence(track_remaining, det_low,
                        activated_tracklets,
                        lost_tracklets_curr,
                        det_low_remaining,
                        effective_dt);

        SUSHITRACK_LOG_DEBUG("[Tracker] Low-conf match: newly_lost=%zu, remaining_dets=%zu",
            lost_tracklets_curr.size(), det_low_remaining.size());

        std::vector<TrackletPtr> det_final_remaining;
        if (lost_recovery_metric_ && !lost_pool.empty())
        {
            matchLostRecovery(lost_pool, det_high_remaining, refind_tracklets, det_final_remaining);

            for (auto& t : lost_pool)
            {
                if (t->getTrackState() == TrackState::Lost)
                    lost_tracklets_curr.push_back(t);
            }
            SUSHITRACK_LOG_DEBUG("[Tracker] Lost recovery: recovered=%zu", refind_tracklets.size());
        }
        else
        {
            det_final_remaining = det_high_remaining;
        }

        if (config_.enable_tentative)
        {
            std::vector<TrackletPtr> tentative_next;
            std::vector<TrackletPtr> removed_tentative_curr;

            matchTentative(tentative_pool, det_final_remaining, det_low_remaining,
                        activated_tracklets,
                        tentative_next,
                        removed_tentative_curr,
                        det_for_new_init,
                        effective_dt);

            std::vector<TrackletPtr> newborn_tentative;
            initializeTentative(det_for_new_init, newborn_tentative);
            tentative_next = jointTracklets(tentative_next, newborn_tentative);

            removed_tracklets_curr = jointTracklets(removed_tracklets_curr, removed_tentative_curr);
            tentative_tracklets_ = tentative_next;

            SUSHITRACK_LOG_DEBUG("[Tracker] Tentative stage: promoted=%zu, remaining=%zu, removed=%zu, new_inits=%zu",
                activated_tracklets.size(), tentative_tracklets_.size(), removed_tentative_curr.size(), det_for_new_init.size());
        }
        else
        {
            matchUnconfirmed(non_active_tracklets, det_final_remaining, activated_tracklets, removed_tracklets_curr);
            tentative_tracklets_.clear();
        }

        cleanupLost(removed_tracklets_curr);

        finalizeLists(activated_tracklets, refind_tracklets,
                    lost_tracklets_curr, removed_tracklets_curr);

        SUSHITRACK_LOG_DEBUG("[Tracker] Cycle complete: tracked=%zu, lost=%zu, tentative=%zu, removed=%zu",
            tracked_tracklets_.size(), lost_tracklets_.size(), tentative_tracklets_.size(), removed_tracklets_curr.size());

        std::vector<TrackletPtr> output;
        output.reserve(tracked_tracklets_.size() + lost_tracklets_.size() + 
                      tentative_tracklets_.size() + removed_tracklets_curr.size());

        output.insert(output.end(), tracked_tracklets_.begin(), tracked_tracklets_.end());
        output.insert(output.end(), lost_tracklets_.begin(), lost_tracklets_.end());
        output.insert(output.end(), tentative_tracklets_.begin(), tentative_tracklets_.end());
        output.insert(output.end(), removed_tracklets_curr.begin(), removed_tracklets_curr.end());

        return output;
    }

    /**
     * @brief Propagates all tracked and lost tracklets to the current timestamp.
     * 
     * @param tracklet_pool Output collection of predicted tracked and lost tracklets.
     * @param tentative_tracks Output collection of predicted tentative tracklets.
     * @param dt Time interval for state propagation.
     */
    void SushiTrackTracker::predictTracks(std::vector<TrackletPtr>& tracklet_pool,
                                        std::vector<TrackletPtr>& tentative_tracks,
                                        float dt)
    {
        std::vector<TrackletPtr> tracked_pool;
        tracked_pool.reserve(tracked_tracklets_.size());

        for (auto& s : tracked_tracklets_)
        {
            if (s->getTrackState() == TrackState::Tracked && s->isConfirmed())
                tracked_pool.push_back(s);
        }

        tracklet_pool = jointTracklets(tracked_pool, lost_tracklets_);

        for (auto& s : tracklet_pool)
            s->predict(dt);

        tentative_tracks = tentative_tracklets_;
        for (auto& s : tentative_tracks)
            s->predict(dt);
    }

    /**
     * @brief Performs association between tracks and high-confidence detections.
     *
     * Two strategies are supported, selected by config_.primary_assoc_mode:
     *   0 = Age-cascade (legacy). Tracks are bucketed by frames-since-last-seen
     *       and each bucket is solved as an independent LAPJV. This favours the
     *       freshest tracks in contested matches and tends to suppress ID
     *       switches in heavy occlusion regimes at the cost of denying the
     *       global optimum to high-confidence clean detections.
     *   1 = Global LAPJV (ByteTrack-identical). A single cost matrix is built
     *       over the union of all candidate tracks and all high-confidence
     *       detections and solved with one LAPJV call. This matches the
     *       reference BYTETracker pipeline (3rdparty/bytetrack/BYTETracker.cpp
     *       lines 82-118) and preserves the global optimum, which improves
     *       DetA on clean / high-confidence sequences.
     *
     * @param tracklet_pool Collection of candidate tracklets for matching.
     * @param det_high Collection of high-confidence observations.
     * @param activated_tracklets Output list for successfully updated tracks.
     * @param refind_tracklets Output list for successfully recovered lost tracks.
     * @param track_remaining Output list for unmatched tracklets.
     * @param det_high_remaining Output list for unmatched high-confidence detections.
     */
    void SushiTrackTracker::matchHighConfidence(std::vector<TrackletPtr>& tracklet_pool,
                                            std::vector<TrackletPtr>& det_high,
                                            std::vector<TrackletPtr>& activated_tracklets,
                                            std::vector<TrackletPtr>& refind_tracklets,
                                            std::vector<TrackletPtr>& track_remaining,
                                            std::vector<TrackletPtr>& det_high_remaining)
    {
        if (config_.primary_assoc_mode == 1)
        {
            // ----- Global LAPJV (ByteTrack-identical) -----
            // Single cost matrix and single solve over the entire candidate set.
            // Mirrors byte_track::BYTETracker::update step 2 exactly so that, with
            // matching cost calculator and thresholds, the assignment is bit-for-bit
            // identical to the reference implementation.

            if (tracklet_pool.empty())
            {
                det_high_remaining = det_high;
                return;
            }

            if (det_high.empty())
            {
                for (auto& t : tracklet_pool)
                {
                    if (t->getTrackState() == TrackState::Tracked)
                        track_remaining.push_back(t);
                }
                det_high_remaining.clear();
                return;
            }

            std::vector<std::vector<int>> matches;
            std::vector<int> u_track, u_det;

            high_conf_metric_->calculate(tracklet_pool, det_high, main_cost_matrix_);

            // Occluder-aware suppression. Only Lost-state rows are eligible
            // here so that confirmed Tracked rows continue to compete for
            // their natural matches without bias. The penalty is identical to
            // the one applied inside matchLostRecovery: the two stages share a
            // single private helper to guarantee policy consistency.
            applyOccluderPenalty(tracklet_pool, det_high, main_cost_matrix_, /*only_lost=*/true);

            solver_->solve(main_cost_matrix_,
                        (int)tracklet_pool.size(),
                        (int)det_high.size(),
                        config_.match_thresh,
                        matches, u_track, u_det);

            for (auto& m : matches)
            {
                auto& track = tracklet_pool[m[0]];
                auto& det   = det_high[m[1]];

                if (track->getTrackState() == TrackState::Tracked)
                {
                    track->update(*det, frame_id_);
                    activated_tracklets.push_back(track);
                }
                else
                {
                    track->reActivate(*det, frame_id_);
                    refind_tracklets.push_back(track);
                    SUSHITRACK_LOG_DEBUG("[Tracker] Track %llu recovered from Lost state (Global LAPJV).",
                        (unsigned long long)track->getTrackId());
                }
            }

            for (int idx : u_track)
            {
                if (tracklet_pool[idx]->getTrackState() == TrackState::Tracked)
                {
                    track_remaining.push_back(tracklet_pool[idx]);
                }
            }

            det_high_remaining.clear();
            det_high_remaining.reserve(u_det.size());
            for (int idx : u_det)
            {
                det_high_remaining.push_back(det_high[idx]);
            }
            return;
        }

        // ----- Age-cascade (legacy mode 0) -----
        det_high_remaining = det_high;

        std::map<size_t, std::vector<TrackletPtr>> tracks_by_age;
        for (auto& t : tracklet_pool)
        {
            size_t age = frame_id_ - t->getFrameId();
            tracks_by_age[age].push_back(t);
        }

        for (auto const& [age, track_group] : tracks_by_age)
        {
            if (det_high_remaining.empty())
            {
                for (auto& t : track_group)
                {
                    if (t->getTrackState() == TrackState::Tracked)
                        track_remaining.push_back(t);
                }
                continue;
            }

            std::vector<std::vector<int>> matches;
            std::vector<int> u_track, u_det;

            high_conf_metric_->calculate(track_group, det_high_remaining, main_cost_matrix_);

            solver_->solve(main_cost_matrix_,
                        (int)track_group.size(),
                        (int)det_high_remaining.size(),
                        config_.match_thresh,
                        matches, u_track, u_det);

            std::vector<TrackletPtr> next_det_remaining;
            std::set<size_t> matched_det_indices;

            for (auto& m : matches)
            {
                auto& track = track_group[m[0]];
                auto& det   = det_high_remaining[m[1]];

                if (track->getTrackState() == TrackState::Tracked)
                {
                    track->update(*det, frame_id_);
                    activated_tracklets.push_back(track);
                }
                else
                {
                    track->reActivate(*det, frame_id_);
                    refind_tracklets.push_back(track);
                    SUSHITRACK_LOG_DEBUG("[Tracker] Track %llu recovered from Lost state (High-conf cascade, age=%zu).", (unsigned long long)track->getTrackId(), age);
                }
                matched_det_indices.insert(static_cast<size_t>(m[1]));
            }

            for (int idx : u_track)
            {
                if (track_group[idx]->getTrackState() == TrackState::Tracked)
                {
                    track_remaining.push_back(track_group[idx]);
                }
            }

            for (size_t i = 0; i < det_high_remaining.size(); ++i)
            {
                if (matched_det_indices.find(i) == matched_det_indices.end())
                {
                    next_det_remaining.push_back(det_high_remaining[i]);
                }
            }
            det_high_remaining = next_det_remaining;
        }
    }

    /**
     * @brief Performs association between unmatched tracks and low-confidence detections.
     *
     * The dt parameter is forwarded to Tracklet::markAsLost so the marking frame
     * is counted in the lost-time budget. Without this, the wall-clock and
     * frame-counted expirations diverge by one frame, biasing the time-aware
     * mode toward keeping stale tracks alive longer.
     *
     * @param track_remaining Collection of tracks remaining after high-confidence matching.
     * @param det_low Collection of low-confidence observations.
     * @param activated_tracklets Output list for successfully updated tracks.
     * @param lost_tracklets_curr Output list for tracks marked as lost during this stage.
     * @param det_low_remaining Output list for unmatched low-confidence detections.
     * @param dt Time interval of the current frame.
     */
    void SushiTrackTracker::matchLowConfidence(std::vector<TrackletPtr>& track_remaining,
                                            std::vector<TrackletPtr>& det_low,
                                            std::vector<TrackletPtr>& activated_tracklets,
                                            std::vector<TrackletPtr>& lost_tracklets_curr,
                                            std::vector<TrackletPtr>& det_low_remaining,
                                            float dt)
    {
        std::vector<std::vector<int>> matches_iou;
        std::vector<int> u_track_iou, u_det_iou;

        low_conf_metric_->calculate(track_remaining, det_low, iou_cost_matrix_);

        solver_->solve(iou_cost_matrix_,
                    (int)track_remaining.size(),
                    (int)det_low.size(),
                    config_.iou_match_thresh,
                    matches_iou, u_track_iou, u_det_iou);

        for (auto& m : matches_iou)
        {
            auto& track = track_remaining[m[0]];
            auto& det   = det_low[m[1]];
            if (track->getTrackState() == TrackState::Tracked)
            {
                track->update(*det, frame_id_, false);
                activated_tracklets.push_back(track);
            }
        }

        for (int idx : u_track_iou)
        {
            auto& track = track_remaining[idx];
            if (track->getTrackState() != TrackState::Lost)
            {
                track->markAsLost(dt);

                // Capture the front-most occluder identified by the
                // scene-level interaction graph at the precise frame in which
                // the track transitions to Lost. The snapshot is consumed by
                // matchLostRecovery to suppress premature re-association
                // against detections that coincide with the occluder while
                // the occluder is still in front of the track's predicted
                // position. The wall-clock stamp is the accumulated lost time
                // recorded by markAsLost(dt), which equals the time interval
                // of the marking frame and is the canonical timestamp for
                // the start of the occlusion event.
                if (interaction_graph_)
                {
                    const int occluder_id = interaction_graph_->getFrontOccluder(track->getTrackId());
                    if (occluder_id != IInteractionGraph::kNoOccluder)
                    {
                        track->setOccluderSnapshot(
                            occluder_id,
                            frame_id_,
                            track->getAccumulatedLostTime());
                        SUSHITRACK_LOG_DEBUG(
                            "[Tracker] Track %llu marked as Lost behind occluder %d.",
                            (unsigned long long)track->getTrackId(), occluder_id);
                    }
                }

                lost_tracklets_curr.push_back(track);
                SUSHITRACK_LOG_DEBUG("[Tracker] Track %llu marked as Lost.", (unsigned long long)track->getTrackId());
            }
        }

        for (int idx : u_det_iou)
        {
            det_low_remaining.push_back(det_low[idx]);
        }
    }

    /**
     * @brief Recovers lost tracks using visual feature similarity.
     * 
     * @param lost_tracks Collection of tracklets currently in the Lost state.
     * @param det_remaining Collection of unmatched high-confidence detections.
     * @param activated_tracklets Output list for successfully recovered tracklets.
     * @param det_final_remaining Output list for detections remaining after recovery.
     */
    void SushiTrackTracker::matchLostRecovery(std::vector<TrackletPtr>& lost_tracks,
                                            std::vector<TrackletPtr>& det_remaining,
                                            std::vector<TrackletPtr>& activated_tracklets,
                                            std::vector<TrackletPtr>& det_final_remaining)
    {
        det_final_remaining = det_remaining;

        if (lost_tracks.empty() || det_final_remaining.empty())
        {
            return;
        }

        std::map<size_t, std::vector<TrackletPtr>> tracks_by_age;
        for (auto& t : lost_tracks)
        {
            size_t age = frame_id_ - t->getFrameId();
            tracks_by_age[age].push_back(t);
        }

        for (auto const& [age, track_group] : tracks_by_age)
        {
            if (det_final_remaining.empty())
            {
                break;
            }

            std::vector<std::vector<int>> matches;
            std::vector<int> u_lost, u_det;

            lost_recovery_metric_->calculate(track_group, det_final_remaining, main_cost_matrix_);

            // Every row in this call site is already in the Lost state by
            // construction, so the only_lost filter does not change which
            // rows participate. The shared helper guarantees that the
            // validity check and penalty application stay consistent with
            // the high-confidence call site.
            applyOccluderPenalty(track_group, det_final_remaining, main_cost_matrix_, /*only_lost=*/false);

            solver_->solve(main_cost_matrix_,
                        (int)track_group.size(),
                        (int)det_final_remaining.size(),
                        config_.reid_recovery_thresh,
                        matches, u_lost, u_det);

            std::vector<TrackletPtr> next_det_remaining;
            std::set<size_t> matched_det_indices;

            for (auto& m : matches)
            {
                auto& track = track_group[m[0]];
                auto& det   = det_final_remaining[m[1]];

                track->reActivate(*det, frame_id_);
                activated_tracklets.push_back(track);
                SUSHITRACK_LOG_DEBUG("[Tracker] Track %llu recovered from Lost state (Visual Recovery, age=%zu).", (unsigned long long)track->getTrackId(), age);
                matched_det_indices.insert(static_cast<size_t>(m[1]));
            }

            for (size_t i = 0; i < det_final_remaining.size(); ++i)
            {
                if (matched_det_indices.find(i) == matched_det_indices.end())
                {
                    next_det_remaining.push_back(det_final_remaining[i]);
                }
            }
            det_final_remaining = next_det_remaining;
        }
    }

    /**
     * @brief Processes association and lifecycle for unconfirmed tentative tracks.
     *
     * Each tentative track is matched first against high-confidence detections,
     * then, if still unmatched, against low-confidence detections. A successful
     * match increments hits_ and calls confirm() if the required hit count is
     * reached. Unmatched tracks are aged out when their accumulated lost-time
     * exceeds tentative_max_age_seconds.
     *
     * @param tentative_tracks        Predicted tentative tracklets for this frame.
     * @param det_high_remaining      Unclaimed high-confidence detections.
     * @param det_low_remaining       Unclaimed low-confidence detections.
     * @param activated_tracklets     Output: tracks promoted to confirmed state.
     * @param tentative_next          Output: tracks that remain tentative.
     * @param removed_tentative_curr  Output: tracks that aged out this frame.
     * @param det_for_new_init        Output: detections eligible for new track init.
     * @param dt                      Time interval of the current frame in seconds.
     *                                Used to compute the age of unmatched tentatives
     *                                at the end of this call: accumulated_lost_time_
     *                                has already been incremented by predict(dt), and
     *                                the current unmatched frame adds one more dt.
     */
    void SushiTrackTracker::matchTentative(std::vector<TrackletPtr>& tentative_tracks,
                                        std::vector<TrackletPtr>& det_high_remaining,
                                        std::vector<TrackletPtr>& det_low_remaining,
                                        std::vector<TrackletPtr>& activated_tracklets,
                                        std::vector<TrackletPtr>& tentative_next,
                                        std::vector<TrackletPtr>& removed_tentative_curr,
                                        std::vector<TrackletPtr>& det_for_new_init,
                                        float dt)
    {
        auto get_dynamic_hits = [&](float score) -> int {
            if (score >= config_.tentative_fast_thresh) return config_.tentative_fast_hits;
            if (score >= config_.high_thresh) return config_.tentative_confirm_hits;
            if (score >= config_.tentative_slow_thresh) return config_.tentative_confirm_hits + config_.tentative_slow_hits_offset;
            return config_.tentative_confirm_hits + config_.tentative_vslow_hits_offset;
        };

        std::vector<std::vector<int>> matches_high;
        std::vector<int> u_tent_high, u_det_high;

        tentative_metric_->calculate(tentative_tracks, det_high_remaining, iou_cost_matrix_);

        solver_->solve(iou_cost_matrix_,
                    (int)tentative_tracks.size(),
                    (int)det_high_remaining.size(),
                    config_.tentative_match_thresh,
                    matches_high, u_tent_high, u_det_high);

        for (auto& m : matches_high)
        {
            auto& t = tentative_tracks[m[0]];
            auto& d = det_high_remaining[m[1]];

            // keep_state=true: do not flip is_activated_/is_confirmed_/state_
            // until the dynamic hit threshold is met. Without this, a tentative
            // track would leak as an activated output on the first match,
            // bypassing the validation window entirely.
            t->update(*d, frame_id_, /*update_features=*/true, /*keep_state=*/true);

            int required_hits = get_dynamic_hits(t->getScore());

            if (t->getHits() >= required_hits)
            {
                t->confirm(frame_id_, t->getTrackId());
                activated_tracklets.push_back(t);
                SUSHITRACK_LOG_DEBUG("[Tracker] Tentative track %llu confirmed.", (unsigned long long)t->getTrackId());
            }
            else
            {
                tentative_next.push_back(t);
            }
        }

        for (int idx : u_det_high)
        {
            det_for_new_init.push_back(det_high_remaining[idx]);
        }

        std::vector<TrackletPtr> tentative_remaining;
        tentative_remaining.reserve(u_tent_high.size());
        for (int idx : u_tent_high)
        {
            tentative_remaining.push_back(tentative_tracks[idx]);
        }

        std::vector<std::vector<int>> matches_low;
        std::vector<int> u_tent_low, u_det_low;

        tentative_metric_->calculate(tentative_remaining, det_low_remaining, iou_cost_matrix_);

        solver_->solve(iou_cost_matrix_,
                    (int)tentative_remaining.size(),
                    (int)det_low_remaining.size(),
                    config_.tentative_match_thresh,
                    matches_low, u_tent_low, u_det_low);

        for (auto& m : matches_low)
        {
            auto& t = tentative_remaining[m[0]];
            auto& d = det_low_remaining[m[1]];

            // keep_state=true: same rationale as the high-confidence loop above
            // — defer activation/confirmation to confirm() once required_hits
            // is reached, so unvalidated tentatives never appear in the output.
            t->update(*d, frame_id_, /*update_features=*/false, /*keep_state=*/true);

            int required_hits = get_dynamic_hits(t->getScore());

            if (t->getHits() >= required_hits)
            {
                t->confirm(frame_id_, t->getTrackId());
                activated_tracklets.push_back(t);
            }
            else
            {
                tentative_next.push_back(t);
            }
        }

        for (int idx : u_tent_low)
        {
            auto& t = tentative_remaining[idx];

            // Age is measured using accumulated_lost_time_, which predict() increments
            // by the real dt on every frame where state == Tentative. update() resets it
            // to zero on every successful match, so it always reflects the time elapsed
            // since the last observation. In frame-based mode dt = 1/fps per frame, so
            // accumulated_lost_time_ counts seconds just as in time-aware mode — the
            // single tentative_max_age_seconds threshold therefore works identically in
            // both modes without any frame-rate conversion at the call site.
            // The current unmatched frame's dt is added here because predict() has already
            // run before matchTentative is called, but the miss on this frame hasn't yet
            // been reflected in accumulated_lost_time_ (update() would have zeroed it on
            // a match; no match occurred, so we account for this frame manually).
            const float accumulated = t->getAccumulatedLostTime() + dt;

            if (accumulated > config_.tentative_max_age_seconds)
            {
                t->markAsRemoved();
                removed_tentative_curr.push_back(t);
                SUSHITRACK_LOG_DEBUG("[Tracker] Tentative track %llu aged out (lost_time=%.3fs, limit=%.3fs).",
                    (unsigned long long)t->getTrackId(), accumulated, config_.tentative_max_age_seconds);
            }
            else
            {
                tentative_next.push_back(t);
            }
        }
    }

    /**
     * @brief Initializes new tentative tracks from unclaimed high-confidence detections.
     * 
     * @param det_high_remaining Collection of qualifying detections for initiation.
     * @param tentative_curr Output collection for the newly created tentative tracklets.
     */
    void SushiTrackTracker::initializeTentative(const std::vector<TrackletPtr>& det_high_remaining,
                                            std::vector<TrackletPtr>& tentative_curr)
    {
        for (auto& det : det_high_remaining)
        {
            if (det->getScore() < config_.high_thresh) continue;

            size_t initial_id = ++track_id_count_;
            
            det->activateTentative(createKalmanFilter(), frame_id_, initial_id);
            tentative_curr.push_back(det);
            SUSHITRACK_LOG_DEBUG("[Tracker] Initialized new tentative track %llu.", (unsigned long long)initial_id);
        }
    }

    /**
     * @brief Handles matching and state promotion for unconfirmed tracks in 
     * non-tentative mode.
     * 
     * @param non_active_tracklets Tracks that have not yet reached confirmation threshold.
     * @param det_high_remaining Unclaimed high-confidence detections.
     * @param activated_tracklets Output list for successfully updated tracks.
     * @param removed_tracklets_curr Output list for tracks qualifying for removal.
     */
    void SushiTrackTracker::matchUnconfirmed(std::vector<TrackletPtr>& non_active_tracklets,
                                            std::vector<TrackletPtr>& det_high_remaining,
                                            std::vector<TrackletPtr>& activated_tracklets,
                                            std::vector<TrackletPtr>& removed_tracklets_curr)
    {
        std::vector<std::vector<int>> matches;
        std::vector<int> u_unconfirmed, u_det;

        tentative_metric_->calculate(non_active_tracklets, det_high_remaining, iou_cost_matrix_);

        solver_->solve(iou_cost_matrix_,
                    (int)non_active_tracklets.size(),
                    (int)det_high_remaining.size(),
                    config_.unconfirmed_iou_thresh,
                    matches, u_unconfirmed, u_det);

        for (auto& m : matches)
        {
            auto& track = non_active_tracklets[m[0]];
            auto& det   = det_high_remaining[m[1]];

            track->update(*det, frame_id_);
            
            activated_tracklets.push_back(track);
            SUSHITRACK_LOG_DEBUG("[Tracker] Unconfirmed track %llu updated.", (unsigned long long)track->getTrackId());
        }

        for (int idx : u_unconfirmed)
        {
            auto& track = non_active_tracklets[idx];
            track->markAsRemoved();
            removed_tracklets_curr.push_back(track);
            SUSHITRACK_LOG_DEBUG("[Tracker] Unconfirmed track %llu aged out and removed.", (unsigned long long)track->getTrackId());
        }

        for (int idx : u_det)
        {
            auto& track = det_high_remaining[idx];
            if (track->getScore() < config_.high_thresh)
                continue;
            
            size_t initial_id = ++track_id_count_;
            track->activate(createKalmanFilter(), frame_id_, initial_id);
            activated_tracklets.push_back(track);
            SUSHITRACK_LOG_DEBUG("[Tracker] New track %llu activated.", (unsigned long long)initial_id);
        }
    }

    /**
     * @brief Removes lost tracks that have exceeded their maximum age and
     *        clears stale occluder snapshots.
     *
     * Two independent actions are taken per lost track:
     *
     *   1. Snapshot expiry. The occluder snapshot is cleared when either the
     *      snapshotted occluder is no longer present in the scene or the
     *      elapsed time since the snapshot exceeds the configured validity
     *      window. The window is derived from interaction_occluder_validity_seconds
     *      scaled by validityScale() for the occluder's kinematic state. In
     *      frame-based mode the seconds value is converted to frames via
     *      frame_rate. This mirrors the check in applyOccluderPenalty()
     *      precisely, so the snapshot is cleared on the same frame the penalty
     *      would stop firing and InteractionState stays consistent.
     *
     *   2. Track expiry. The configured track_buffer is scaled by the
     *      occludee's last-seen kinematic state (stationary tracks get a
     *      longer buffer, maneuvering tracks a modest extension). When the
     *      accumulated lost time exceeds the scaled buffer the track is marked
     *      Removed.
     *
     * @param removed_tracklets_curr Output collection for expired tracklets.
     */
    void SushiTrackTracker::cleanupLost(std::vector<TrackletPtr>& removed_tracklets_curr)
    {
        for (auto& s : lost_tracklets_)
        {
            // Drop the occluder snapshot once it has outlived its validity
            // window or the snapshotted occluder has itself left the scene.
            // The window is scaled by the occluder's kinematic state using the
            // same validityScale() call used in applyOccluderPenalty(), so the
            // snapshot is cleared at exactly the frame the penalty would stop
            // firing. Without this alignment the InteractionState could read
            // Free while the penalty was still being applied, or vice versa.
            if (s->getOccluderId() >= 0)
            {
                const size_t occ_id = static_cast<size_t>(s->getOccluderId());
                const bool occluder_present = interaction_graph_ &&
                    interaction_graph_->contains(occ_id);

                // Resolve the kinematic scale from the live graph if available;
                // fall back to 1.0 (neutral) when the occluder has already left
                // the scene so the snapshot can still expire on schedule rather
                // than being cleared immediately by the presence check alone.
                const float scale = (interaction_graph_ && occluder_present)
                    ? validityScale(interaction_graph_->getKinematicState(occ_id))
                    : 1.0f;

                bool expired = false;
                if (config_.time_aware)
                {
                    const float elapsed = s->getAccumulatedLostTime() - s->getOccluderSnapshotTime();
                    expired = elapsed > config_.interaction_occluder_validity_seconds * scale;
                }
                else
                {
                    const size_t snap_frame = s->getOccluderSnapshotFrame();
                    const size_t elapsed_frames = (frame_id_ >= snap_frame)
                        ? (frame_id_ - snap_frame) : 0;
                    // Convert the seconds-based window to frames and apply the
                    // kinematic scale, mirroring the applyOccluderPenalty path.
                    const float scaled_frames =
                        config_.interaction_occluder_validity_seconds
                        * static_cast<float>(std::max(1, config_.frame_rate))
                        * scale;
                    expired = elapsed_frames > static_cast<size_t>(std::lround(scaled_frames));
                }
                if (expired || !occluder_present)
                {
                    s->clearOccluderSnapshot();
                }
            }

            // State-aware buffer scaling. The kinematic regime captured just
            // before the track went dark drives how long we are willing to
            // wait for a recovery:
            //   - Stationary tracks reliably reappear at the same spot, so a
            //     longer buffer helps reclaim them before identity churn.
            //   - Maneuvering tracks have noisy predictions; a modest extra
            //     budget keeps them recoverable through brief occlusions.
            //   - Cruising / no-FSM tracks use the configured buffer as-is.
            // Defaults collapse to 1.0x when no FSM is attached so the policy
            // is bit-identical to the prior behaviour with the FSM disabled.
            float buffer_scale = 1.0f;
            if (s->getFSM())
            {
                switch (s->getLastSeenState())
                {
                    case KinematicState::Stationary:  buffer_scale = 1.5f; break;
                    case KinematicState::Maneuvering: buffer_scale = 1.2f; break;
                    case KinematicState::Cruising:
                    case KinematicState::Coasting:
                    default:                           buffer_scale = 1.0f; break;
                }
            }

            const int scaled_buffer = std::max(1,
                static_cast<int>(std::round(static_cast<float>(config_.track_buffer) * buffer_scale)));

            const float accumulated = config_.time_aware
                ? s->getAccumulatedLostTime()
                : static_cast<float>(frame_id_ - s->getFrameId());

            if (timing_policy_->isExpired(accumulated, scaled_buffer))
            {
                s->markAsRemoved();
                removed_tracklets_curr.push_back(s);
                SUSHITRACK_LOG_INFO("[Tracker] Track %llu expired after being lost for %.2f seconds (scaled buffer=%d).",
                    (unsigned long long)s->getTrackId(), accumulated, scaled_buffer);
            }
        }
    }

    /**
     * @brief Finalizes the internal state lists of tracked, lost, and removed tracks.
     * 
     * @param activated_tracklets Updated tracked tracklets for the current frame.
     * @param refind_tracklets Recovered lost tracklets.
     * @param lost_tracklets_curr Tracklets newly transitioning to lost state.
     * @param removed_tracklets_curr Tracklets qualifying for removal.
     */
    void SushiTrackTracker::finalizeLists(const std::vector<TrackletPtr>& activated_tracklets,
                                        const std::vector<TrackletPtr>& refind_tracklets,
                                        const std::vector<TrackletPtr>& lost_tracklets_curr,
                                        const std::vector<TrackletPtr>& removed_tracklets_curr)
    {
        tracked_tracklets_ = jointTracklets(activated_tracklets, refind_tracklets);

        std::vector<TrackletPtr> new_lost;
        for (auto& s : lost_tracklets_)
        {
            if (s->getTrackState() == TrackState::Lost)
            {
                new_lost.push_back(s);
            }
        }
        for (auto& s : lost_tracklets_curr)
        {
            new_lost.push_back(s);
        }

        lost_tracklets_ = subTracklets(new_lost, tracked_tracklets_);
        lost_tracklets_ = subTracklets(lost_tracklets_, removed_tracklets_curr);

        std::vector<TrackletPtr> ts_out, ls_out;
        removeDuplicateTracklets(tracked_tracklets_, lost_tracklets_, ts_out, ls_out);
        tracked_tracklets_ = ts_out;
        lost_tracklets_    = ls_out;
    }

    /**
     * @brief Retrieves all tracklets managed by the tracker across all lifecycle states.
     * 
     * @return Collection of managed tracklets (Tracked, Lost, Tentative, Removed).
     */
    std::vector<SushiTrackTracker::TrackletPtr> SushiTrackTracker::getTracks() const
    {
        std::vector<TrackletPtr> output;
        output.reserve(tracked_tracklets_.size() + lost_tracklets_.size() + 
                      tentative_tracklets_.size());

        output.insert(output.end(), tracked_tracklets_.begin(), tracked_tracklets_.end());
        output.insert(output.end(), lost_tracklets_.begin(), lost_tracklets_.end());
        output.insert(output.end(), tentative_tracklets_.begin(), tentative_tracklets_.end());

        return output;
    }

    /**
     * @brief Merges two tracklet collections while ensuring element uniqueness.
     * 
     * @param a First tracklet collection.
     * @param b Second tracklet collection.
     * @return Merged unique collection of tracklets.
     */
    std::vector<SushiTrackTracker::TrackletPtr>
    SushiTrackTracker::jointTracklets(const std::vector<TrackletPtr>& __restrict a, const std::vector<TrackletPtr>& __restrict b) const
    {
        current_visit_id_++;
        std::vector<TrackletPtr> res;
        res.reserve(a.size() + b.size());

        for (const auto& t : a)
        {
            t->visit_id_ = current_visit_id_;
            res.push_back(t);
        }
        for (const auto& t : b)
        {
            if (t->visit_id_ != current_visit_id_)
            {
                t->visit_id_ = current_visit_id_;
                res.push_back(t);
            }
        }
        return res;
    }

    /**
     * @brief Subtracts one tracklet collection from another.
     * 
     * @param a Source tracklet collection.
     * @param b Collection containing elements to be removed from the source.
     * @return Resulting collection after subtraction.
     */
    std::vector<SushiTrackTracker::TrackletPtr>
    SushiTrackTracker::subTracklets(const std::vector<TrackletPtr>& __restrict a, const std::vector<TrackletPtr>& __restrict b) const
    {
        current_visit_id_++;
        for (const auto& t : b)
        {
            t->visit_id_ = current_visit_id_;
        }

        std::vector<TrackletPtr> res;
        res.reserve(a.size());
        for (const auto& t : a)
        {
            if (t->visit_id_ != current_visit_id_)
            {
                res.push_back(t);
            }
        }
        return res;
    }

    /**
     * @brief Eliminates redundant tracklets based on spatial overlap thresholds.
     *
     * Two tracklets that occupy nearly identical regions are treated as
     * duplicates; the longer-lived one is retained. Track lifetime is the
     * span between the current observation frame and the track's start
     * frame, mirroring the reference ByteTrack policy
     * (BYTETracker.cpp:300-310). The previous "freshness" rule
     * (frame_id - getFrameId()) is incorrect for this purpose: a Tracked
     * track always has freshness 0 and would automatically win against any
     * just-recovered Lost track, which churns identities on crowded scenes
     * where a long-tracked target temporarily overlaps a young one.
     *
     * @param a First collection of candidate tracklets.
     * @param b Second collection of candidate tracklets.
     * @param a_res Output collection from the first set after duplication removal.
     * @param b_res Output collection from the second set after duplication removal.
     */
    void SushiTrackTracker::removeDuplicateTracklets(const std::vector<TrackletPtr>& __restrict a,
                                                const std::vector<TrackletPtr>& __restrict b,
                                                std::vector<TrackletPtr>& __restrict a_res,
                                                std::vector<TrackletPtr>& __restrict b_res) const
    {
        duplicate_metric_->calculate(a, b, iou_cost_matrix_);

        std::vector<bool> a_dup(a.size(), false), b_dup(b.size(), false);
        for (size_t i = 0; i < a.size(); ++i)
        {
            for (size_t j = 0; j < b.size(); ++j)
            {
                if (iou_cost_matrix_(i, j) < config_.duplicate_iou_thresh)
                {
                    // Lifetime: how many frames the track has existed for.
                    // Longer-lived track wins; the younger one is dropped.
                    const int ta = (int)(a[i]->getFrameId() - a[i]->getStartFrameId());
                    const int tb = (int)(b[j]->getFrameId() - b[j]->getStartFrameId());
                    if (ta > tb) b_dup[j] = true;
                    else         a_dup[i] = true;
                }
            }
        }

        a_res.clear();
        b_res.clear();
        for (size_t i = 0; i < a.size(); i++)
        {
            if (!a_dup[i]) a_res.push_back(a[i]);
        }
        for (size_t j = 0; j < b.size(); j++)
        {
            if (!b_dup[j]) b_res.push_back(b[j]);
        }
    }

} // namespace SushiTrack
