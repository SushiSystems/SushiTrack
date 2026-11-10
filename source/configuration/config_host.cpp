/**************************************************************************/
/* config_host.cpp                                                        */
/**************************************************************************/
/*                          This file is part of:                         */
/*                               SushiTrack                               */
/*               https://github.com/SushiSystems/SushiTrack               */
/*                         https://sushisystems.io                        */
/**************************************************************************/
/* Copyright (c) 2026-present Mustafa Garip & Sushi Systems               */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License"); you    */
/* may not use this file except in compliance with the License. You may   */
/* obtain a copy of the License at                                        */
/*                                                                        */
/*     https://www.apache.org/licenses/LICENSE-2.0                        */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/**************************************************************************/

/**
 * @file config_host.cpp
 * @brief Implements the system and in-memory adapters of ConfigHost.
 * @author Mustafa Garip
 */

#include <SushiTrack/configuration/config_host.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace
{
    /**
     * @brief Returns @p path with every separator in the generic form.
     */
    std::string generic(const std::string& path)
    {
        return fs::path(path).generic_string();
    }
} // namespace

namespace SushiTrack
{
    bool SystemConfigHost::exists(const std::string& path) const
    {
        std::error_code ec;
        return fs::exists(path, ec);
    }

    std::unique_ptr<std::istream> SystemConfigHost::open(const std::string& path) const
    {
        auto file = std::make_unique<std::ifstream>(path);
        if (!file->is_open())
            return nullptr;
        return file;
    }

    std::string SystemConfigHost::variable(const std::string& name) const
    {
        const char* value = std::getenv(name.c_str());
        return value ? std::string(value) : std::string();
    }

    std::string SystemConfigHost::working_directory(std::error_code& ec) const
    {
        const fs::path dir = fs::current_path(ec);
        return ec ? std::string() : dir.string();
    }

    void InMemoryConfigHost::add_file(const std::string& path, const std::string& text)
    {
        files_[generic(path)] = text;
    }

    void InMemoryConfigHost::set_variable(const std::string& name, const std::string& value)
    {
        variables_[name] = value;
    }

    void InMemoryConfigHost::set_working_directory(const std::string& path)
    {
        working_directory_ = path;
    }

    bool InMemoryConfigHost::exists(const std::string& path) const
    {
        const std::string key = generic(path);
        if (files_.count(key) != 0)
            return true;
        const std::string folder = key + "/";
        const auto below = files_.lower_bound(folder);
        return below != files_.end() && below->first.compare(0, folder.size(), folder) == 0;
    }

    std::unique_ptr<std::istream> InMemoryConfigHost::open(const std::string& path) const
    {
        const auto found = files_.find(generic(path));
        if (found == files_.end())
            return nullptr;
        return std::make_unique<std::istringstream>(found->second);
    }

    std::string InMemoryConfigHost::variable(const std::string& name) const
    {
        const auto found = variables_.find(name);
        return found == variables_.end() ? std::string() : found->second;
    }

    std::string InMemoryConfigHost::working_directory(std::error_code& ec) const
    {
        ec.clear();
        return working_directory_;
    }
} // namespace SushiTrack
