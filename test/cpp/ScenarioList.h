/**
 * @file      ScenarioList.h
 * @brief     Registry of benchmark sequences; defines the dataset scenarios (MOT17/MOT20) used for automated tracking evaluation.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      30.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */
 
#pragma once

#include <string>

namespace TestInfrastructure
{

	struct Scenario
	{
		std::string name;
		std::string dataset;
	};

	// clang-format off
	static const Scenario kScenarios[] =
	{
		{ "MOT17-02-DPM",   "MOT17" },
		{ "MOT17-02-FRCNN", "MOT17" },
		{ "MOT17-02-SDP",   "MOT17" },
		{ "MOT17-04-DPM",   "MOT17" },
		{ "MOT17-04-FRCNN", "MOT17" },
		{ "MOT17-04-SDP",   "MOT17" },
		{ "MOT17-05-DPM",   "MOT17" },
		{ "MOT17-05-FRCNN", "MOT17" },
		{ "MOT17-05-SDP",   "MOT17" },
		{ "MOT17-09-DPM",   "MOT17" },
		{ "MOT17-09-FRCNN", "MOT17" },
		{ "MOT17-09-SDP",   "MOT17" },
		{ "MOT17-10-DPM",   "MOT17" },
		{ "MOT17-10-FRCNN", "MOT17" },
		{ "MOT17-10-SDP",   "MOT17" },
		{ "MOT17-11-DPM",   "MOT17" },
		{ "MOT17-11-FRCNN", "MOT17" },
		{ "MOT17-11-SDP",   "MOT17" },
		{ "MOT17-13-DPM",   "MOT17" },
		{ "MOT17-13-FRCNN", "MOT17" },
		{ "MOT17-13-SDP",   "MOT17" },

		{ "MOT20-01", "MOT20" },
		{ "MOT20-02", "MOT20" },
		{ "MOT20-03", "MOT20" },
		{ "MOT20-05", "MOT20" },
	};
	// clang-format on

} // namespace TestInfrastructure
