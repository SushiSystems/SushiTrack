/**************************************************************************/
/* main.cpp                                                               */
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
 * @file main.cpp
 * @brief Runs SushiTrack and the reference trackers over the MOT sequences and prints their timing.
 * @author Mustafa Garip
 */

#include <algorithm>
#include <cstring>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#ifdef _WIN32
#include <intrin.h>
#else
#include <fstream>
#endif

#include <byte_track_adapter.hpp>
#include <mot_runner.hpp>
#include <oc_sort_adapter.hpp>
#include <sushitrack_adapter.hpp>
#include <tracker_adapter.hpp>

namespace
{
    /** @brief Holds the folders and the names the command line gives. */
    struct Arguments
    {
            std::filesystem::path data;         /**< The sequences, one folder each. */
            std::filesystem::path settings;     /**< bytetrack.json and ocsort.json. */
            std::vector<std::string> trackers;  /**< sushitrack, bytetrack, ocsort. */
            std::vector<std::string> sequences; /**< Every sequence under data when empty. */
    };

    /** @brief Prints how to call the program on stderr. */
    void print_usage()
    {
        std::cerr << "usage: mot_engine --data DIR --settings DIR [--tracker NAME]... "
                     "[--sequence NAME]...\n";
    }

    /** @brief Reads the command line; returns false after naming what is wrong. */
    bool parse(int argc, char** argv, Arguments& args)
    {
        for (int i = 1; i < argc; ++i)
        {
            const std::string flag = argv[i];
            if (i + 1 >= argc)
            {
                std::cerr << "mot_engine: " << flag << " needs a value\n";
                return false;
            }
            const std::string value = argv[++i];
            if (flag == "--data")
                args.data = value;
            else if (flag == "--settings")
                args.settings = value;
            else if (flag == "--tracker")
                args.trackers.push_back(value);
            else if (flag == "--sequence")
                args.sequences.push_back(value);
            else
            {
                std::cerr << "mot_engine: unknown flag " << flag << "\n";
                return false;
            }
        }
        if (args.trackers.empty())
            args.trackers = {"sushitrack"};
        return !args.data.empty() && !args.settings.empty();
    }

    /** @brief Returns the adapter of @p name, or nullptr for a name it does not know. */
    std::unique_ptr<TrackerBridge::TrackerAdapter> make_adapter(const std::string& name,
                                                                const Arguments& args)
    {
        if (name == "sushitrack")
            return std::make_unique<TrackerBridge::SushiTrackAdapter>();
        if (name == "bytetrack")
            return std::make_unique<TrackerBridge::ByteTrackAdapter>(
                (args.settings / "bytetrack.json").string());
        if (name == "ocsort")
            return std::make_unique<TrackerBridge::OCSORTAdapter>(
                (args.settings / "ocsort.json").string());
        return nullptr;
    }

    /** @brief Returns every sequence folder under @p data that holds det/det.txt, sorted. */
    std::vector<std::string> all_sequences(const std::filesystem::path& data)
    {
        std::vector<std::string> names;
        for (const auto& entry : std::filesystem::directory_iterator(data))
        {
            if (std::filesystem::exists(entry.path() / "det" / "det.txt"))
                names.push_back(entry.path().filename().string());
        }
        std::sort(names.begin(), names.end());
        return names;
    }

    /** @brief Prints the processor's name, so a timing can be read against its machine. */
    void print_hardware_info()
    {
        std::string brand;
#if defined(_WIN32)
        int info[4] = {-1};
        char text[0x40] = {0};
        __cpuid(info, 0x80000000);
        const int top = info[0];
        for (int leaf = 0x80000002; leaf <= top && leaf <= 0x80000004; ++leaf)
        {
            __cpuid(info, leaf);
            std::memcpy(text + (leaf - 0x80000002) * 16, info, 16);
        }
        brand = text;
#else
        std::ifstream cpuinfo("/proc/cpuinfo");
        std::string line;
        while (std::getline(cpuinfo, line))
        {
            if (line.rfind("model name", 0) == 0)
            {
                brand = line.substr(line.find(':') + 2);
                break;
            }
        }
#endif
        std::cout << "[ HARDWARE ] CPU: " << (brand.empty() ? "unknown" : brand) << "\n";
    }
} // namespace

/**
 * @brief Runs each tracker over each sequence and prints one `[ PERF ]` line per pair.
 * @return 0 when every run ran; 1 when an input is missing; 2 for a bad command line.
 */
int main(int argc, char** argv)
{
    Arguments args;
    if (!parse(argc, argv, args))
    {
        print_usage();
        return 2;
    }
    if (!std::filesystem::is_directory(args.data))
    {
        std::cerr << "mot_engine: no data folder at " << args.data.string() << "\n";
        return 1;
    }
    if (args.sequences.empty())
        args.sequences = all_sequences(args.data);

    print_hardware_info();
    for (const std::string& tracker : args.trackers)
    {
        for (const std::string& sequence : args.sequences)
        {
            std::unique_ptr<TrackerBridge::TrackerAdapter> adapter = make_adapter(tracker, args);
            if (!adapter)
            {
                std::cerr << "mot_engine: unknown tracker " << tracker << "\n";
                return 2;
            }
            try
            {
                const MOTEngine::RunResult result =
                    MOTEngine::run_mot_sequence(args.data, sequence, *adapter);
                std::cout << "[ PERF " << adapter->name() << " ] " << sequence << ": "
                          << result.mean_us() << " us/frame  (" << result.frames_processed
                          << " frames)\n";
            }
            catch (const std::exception& error)
            {
                std::cerr << "mot_engine: " << sequence << ": " << error.what() << "\n";
                return 1;
            }
        }
    }
    return 0;
}
