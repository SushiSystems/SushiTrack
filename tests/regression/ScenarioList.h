/**************************************************************************/
/* ScenarioList.h                                                         */
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
 * @file ScenarioList.h
 * @brief Enumerates benchmark sequences for regression testing.
 * @author Mustafa Garip
 */

#pragma once

#include <string>

namespace TestInfrastructure
{

    /**
     * @brief Data structure representing a benchmarking scenario.
     */
    struct Scenario
    {
        std::string name;    ///< Specific sequence name (e.g., "MOT17-02-DPM").
        std::string dataset; ///< Parent dataset identifier (e.g., "MOT17").
    };

	// clang-format off
	static const Scenario kScenarios[] =
	{
		{ "MOT17-02-DPM",   "MOT17" },
		{ "MOT17-02-FRCNN", "MOT17" },
		{ "MOT17-02-SDP",   "MOT17" },
		{ "MOT17-02-YOLOX", "MOT17" },
		{ "MOT17-04-DPM",   "MOT17" },
		{ "MOT17-04-FRCNN", "MOT17" },
		{ "MOT17-04-SDP",   "MOT17" },
		{ "MOT17-04-YOLOX", "MOT17" },
		{ "MOT17-05-DPM",   "MOT17" },
		{ "MOT17-05-FRCNN", "MOT17" },
		{ "MOT17-05-SDP",   "MOT17" },
		{ "MOT17-05-YOLOX", "MOT17" },
		{ "MOT17-09-DPM",   "MOT17" },
		{ "MOT17-09-FRCNN", "MOT17" },
		{ "MOT17-09-SDP",   "MOT17" },
		{ "MOT17-09-YOLOX", "MOT17" },
		{ "MOT17-10-DPM",   "MOT17" },
		{ "MOT17-10-FRCNN", "MOT17" },
		{ "MOT17-10-SDP",   "MOT17" },
		{ "MOT17-10-YOLOX", "MOT17" },
		{ "MOT17-11-DPM",   "MOT17" },
		{ "MOT17-11-FRCNN", "MOT17" },
		{ "MOT17-11-SDP",   "MOT17" },
		{ "MOT17-11-YOLOX", "MOT17" },
		{ "MOT17-13-DPM",   "MOT17" },
		{ "MOT17-13-FRCNN", "MOT17" },
		{ "MOT17-13-SDP",   "MOT17" },
		{ "MOT17-13-YOLOX", "MOT17" },

		{ "MOT20-01", "MOT20" },
		{ "MOT20-01-YOLOX", "MOT20" },
		{ "MOT20-02", "MOT20" },
		{ "MOT20-02-YOLOX", "MOT20" },
		{ "MOT20-03", "MOT20" },
		{ "MOT20-03-YOLOX", "MOT20" },
		{ "MOT20-05", "MOT20" },
		{ "MOT20-05-YOLOX", "MOT20" },

		{ "dancetrack0001", "DanceTrack" },
		{ "dancetrack0002", "DanceTrack" },
		{ "dancetrack0006", "DanceTrack" },
		{ "dancetrack0008", "DanceTrack" },
		{ "dancetrack0012", "DanceTrack" },
		{ "dancetrack0015", "DanceTrack" },
		{ "dancetrack0016", "DanceTrack" },
		{ "dancetrack0020", "DanceTrack" },
		{ "dancetrack0023", "DanceTrack" },
		{ "dancetrack0024", "DanceTrack" },
		{ "dancetrack0027", "DanceTrack" },
		{ "dancetrack0029", "DanceTrack" },
		{ "dancetrack0032", "DanceTrack" },
		{ "dancetrack0033", "DanceTrack" },
		{ "dancetrack0037", "DanceTrack" },
		{ "dancetrack0039", "DanceTrack" },
		{ "dancetrack0044", "DanceTrack" },
		{ "dancetrack0045", "DanceTrack" },
		{ "dancetrack0049", "DanceTrack" },
		{ "dancetrack0051", "DanceTrack" },
		{ "dancetrack0052", "DanceTrack" },
		{ "dancetrack0053", "DanceTrack" },
		{ "dancetrack0055", "DanceTrack" },
		{ "dancetrack0057", "DanceTrack" },
		{ "dancetrack0061", "DanceTrack" },
		{ "dancetrack0062", "DanceTrack" },
		{ "dancetrack0066", "DanceTrack" },
		{ "dancetrack0068", "DanceTrack" },
		{ "dancetrack0069", "DanceTrack" },
		{ "dancetrack0072", "DanceTrack" },
		{ "dancetrack0074", "DanceTrack" },
		{ "dancetrack0075", "DanceTrack" },
		{ "dancetrack0080", "DanceTrack" },
		{ "dancetrack0082", "DanceTrack" },
		{ "dancetrack0083", "DanceTrack" },
		{ "dancetrack0086", "DanceTrack" },
		{ "dancetrack0087", "DanceTrack" },
		{ "dancetrack0096", "DanceTrack" },
		{ "dancetrack0098", "DanceTrack" },
		{ "dancetrack0099", "DanceTrack" },
	};
	// clang-format on

} // namespace TestInfrastructure
