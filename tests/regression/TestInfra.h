/**
 * @file      TestInfra.h
 * @brief     Utility functions and data structures for benchmark infrastructure.
 * @author    Mustafa Garip (mustafagarip@sushisystems.io)
 * @date      28.04.2026
 * @version   v1.0.0
 * @copyright (c) 2026 Mustafa Garip & Sushi Systems All rights reserved.
 */

#pragma once

#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include <exception>

#ifdef _WIN32
#   include <intrin.h>
#endif

#include <nlohmann/json.hpp>

namespace TestInfrastructure
{
    /**
     * @brief Input detection structure for Multi-Object Tracking.
     */
    struct MOTDetection
    {
        float x;     ///< Horizontal coordinate.
        float y;     ///< Vertical coordinate.
        float w;     ///< Dimension width.
        float h;     ///< Dimension height.
        float score; ///< Detection confidence.
        int   label; ///< Classification label.
    };

    /**
     * @brief Output tracking result structure.
     */
    struct TrackResult
    {
        int   track_id; ///< Identity identifier.
        float x;        ///< Horizontal coordinate.
        float y;        ///< Vertical coordinate.
        float w;        ///< Dimension width.
        float h;        ///< Dimension height.
        float score;    ///< Identity confidence.
    };

    /**
     * @brief High-resolution timer for execution time measurement.
     */
    class PerformanceTimer
    {
        public:
            /**
             * @brief Initializes the timer with the current timestamp.
             */
            PerformanceTimer() : start_(std::chrono::high_resolution_clock::now()) {}

            /**
             * @brief Resets the start timestamp.
             */
            void reset() { start_ = std::chrono::high_resolution_clock::now(); }

            /**
             * @brief Calculates elapsed time since the last reset.
             * 
             * @return Elapsed time in microseconds.
             */
            double elapsed_us() const
            {
                auto end = std::chrono::high_resolution_clock::now();
                return std::chrono::duration<double, std::micro>(end - start_).count();
            }

        private:
            std::chrono::time_point<std::chrono::high_resolution_clock> start_; ///< Start timestamp.
    };

    /**
     * @brief Detects and prints system CPU information.
     */
    inline void printHardwareInfo()
    {
            std::cout << "\n[ HARDWARE ]\n";

        #if defined(_WIN32)
            int  cpuInfo[4]  = {-1};
            char brand[0x40] = {0};
            __cpuid(cpuInfo, 0x80000000);
            int n = cpuInfo[0];
            for (int i = 0x80000002; i <= n && i <= 0x80000004; ++i)
            {
                __cpuid(cpuInfo, i);
                std::memcpy(brand + (i - 0x80000002) * 16, cpuInfo, 16);
            }
            std::cout << "  CPU : " << brand << "\n";
        #elif defined(__linux__)
            std::ifstream f("/proc/cpuinfo");
            std::string   line;
            while (std::getline(f, line))
            {
                if (line.find("model name") == 0)
                {
                    auto pos = line.find(':');
                    if (pos != std::string::npos)
                        std::cout << "  CPU : " << line.substr(pos + 2) << "\n";
                    break;
                }
            }
        #elif defined(__APPLE__)
            std::cout << "  CPU : ";
            std::fflush(stdout);
            std::system("sysctl -n machdep.cpu.brand_string");
        #endif

            std::cout << "------------------------------\n";
    }

    /**
     * @brief Parses a MOT format detection file.
     * 
     * @param filepath Path to the input detection text file.
     * @return Map of frame identifiers to detection collections.
     */
    inline std::map<int, std::vector<MOTDetection>>
    load_mot_detections(const std::string& filepath)
    {
        std::map<int, std::vector<MOTDetection>> result;
        std::ifstream file(filepath);
        if (!file.is_open())
            return result;

        std::string line;
        while (std::getline(file, line))
        {
            if (line.empty() || line[0] == '#')
                continue;

            std::stringstream  ss(line);
            std::string        token;
            std::vector<float> row;
            while (std::getline(ss, token, ','))
            {
                try { row.push_back(std::stof(token)); }
                catch (...) { break; }
            }

            if (row.size() < 7) continue;

            int          frame_id = static_cast<int>(row[0]);
            MOTDetection det;
            det.x     = row[2];
            det.y     = row[3];
            det.w     = row[4];
            det.h     = row[5];
            det.score = row[6];
            det.label = (row.size() > 7) ? static_cast<int>(row[7]) : 1;
            result[frame_id].push_back(det);
        }

        return result;
    }

    using JsonSection = nlohmann::json;
    using JsonDoc     = nlohmann::json;

    /**
     * @brief Minimalist JSON parser for configuration loading.
     * 
     * @param filepath Path to the input JSON file.
     * @return Map representing the JSON structure.
     */
    inline JsonDoc parse_json(const std::string& filepath)
    {
        JsonDoc result = nlohmann::json::object();
        std::ifstream file(filepath);
        if (!file.is_open()) return result;
        
        try 
        {
            result = nlohmann::json::parse(file, nullptr, true, true);
        } 
        catch (const nlohmann::json::parse_error& e) 
        {
            std::cerr << "[ JSON ERROR ] Parse failed in " << filepath 
                      << " at byte " << e.byte << ": " << e.what() << "\n";
        } 
        catch (const std::exception& e) 
        {
            std::cerr << "[ JSON ERROR ] Error parsing " << filepath << ": " << e.what() << "\n";
        } 
        catch (...) 
        {
            std::cerr << "[ JSON ERROR ] Unknown error parsing " << filepath << "\n";
        }
        return result;
    }

    /**
     * @brief Looks up a value in a JSON section.
     *
     * @param sec  The section to search.
     * @param key  The key to look up.
     * @param def  Value returned when the key is absent.
     * @return The stored value or def if the key is not found.
     */
    inline float json_get(const JsonSection& sec, const std::string& key, float def)
    {
        if (sec.is_object() && sec.contains(key) && sec[key].is_number())
            return sec[key].get<float>();
        return def;
    }

    
} // namespace TestInfrastructure