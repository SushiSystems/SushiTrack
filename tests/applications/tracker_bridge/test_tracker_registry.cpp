/**************************************************************************/
/* test_tracker_registry.cpp                                              */
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
 * @file test_tracker_registry.cpp
 * @brief Tests TrackerRegistry.
 * @author Mustafa Garip
 */

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <mot_detection.hpp>
#include <track_result.hpp>
#include <tracker_adapter.hpp>
#include <tracker_registry.hpp>

using namespace TrackerBridge;

namespace
{
    /** @brief Reports no track; names itself "probe". */
    class SilentAdapter final : public TrackerAdapter
    {
        public:
            /// @brief Returns "probe".
            [[nodiscard]] std::string name() const override { return "probe"; }

            /// @brief Returns no track.
            [[nodiscard]] std::vector<TrackResult> update(const std::vector<MOTDetection>&,
                                                          float) override
            {
                return {};
            }

            /// @brief Ignores the frame rate.
            void set_frame_rate(int) override {}
    };
} // namespace

/**
 * @brief A name registered in one case is found in any other.
 */
TEST(Small_TrackerRegistryTest, FindsANameInAnyCase)
{
    TrackerRegistry& registry = TrackerRegistry::get_instance();
    registry.register_tracker("RegistryProbe",
                              []
                              {
                                  return std::make_unique<SilentAdapter>();
                              });

    const std::unique_ptr<TrackerAdapter> adapter = registry.create("REGISTRYPROBE");
    ASSERT_NE(adapter, nullptr);
    EXPECT_EQ(adapter->name(), "probe");
}

/**
 * @brief A name nobody registered builds nothing.
 */
TEST(Small_TrackerRegistryTest, UnknownNameBuildsNothing)
{
    EXPECT_EQ(TrackerRegistry::get_instance().create("no-such-tracker"), nullptr);
}
