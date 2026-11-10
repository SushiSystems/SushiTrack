/**************************************************************************/
/* config_host.hpp                                                        */
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
 * @file config_host.hpp
 * @brief Declares ConfigHost, the port through which the configuration search reads the machine,
 *        and its two adapters.
 * @author Mustafa Garip
 */

#pragma once

#include <istream>
#include <map>
#include <memory>
#include <string>
#include <system_error>

#include <SushiTrack/c_api/sushitrack_export.hpp>

namespace SushiTrack
{
    /**
     * @brief Names what the configuration search reads from the machine: files, environment
     *        variables and the current directory.
     */
    class SUSHITRACK_API ConfigHost
    {
        public:
            virtual ~ConfigHost() = default;

            /// @brief Returns whether a file or folder exists at @p path.
            [[nodiscard]] virtual bool exists(const std::string& path) const = 0;

            /// @brief Opens the file at @p path, or returns null when it cannot be opened.
            [[nodiscard]] virtual std::unique_ptr<std::istream>
            open(const std::string& path) const = 0;

            /// @brief Returns the value of the variable @p name, or an empty string when it is
            ///        unset or empty.
            [[nodiscard]] virtual std::string variable(const std::string& name) const = 0;

            /// @brief Returns the current directory, or an empty string with @p ec set when it
            ///        cannot be read.
            [[nodiscard]] virtual std::string working_directory(std::error_code& ec) const = 0;
    };

    /**
     * @brief Reads the real file system, the process environment and the process directory.
     */
    class SUSHITRACK_API SystemConfigHost final : public ConfigHost
    {
        public:
            [[nodiscard]] bool exists(const std::string& path) const override;
            [[nodiscard]] std::unique_ptr<std::istream> open(const std::string& path) const override;
            [[nodiscard]] std::string variable(const std::string& name) const override;
            [[nodiscard]] std::string working_directory(std::error_code& ec) const override;
    };

    /**
     * @brief Holds files, variables and a current directory in memory, so a search runs against
     *        a machine the caller describes.
     *
     * Paths compare in their generic form, so a backslash and a slash name one file on Windows.
     * A folder exists when a file lies below it.
     */
    class SUSHITRACK_API InMemoryConfigHost final : public ConfigHost
    {
        public:
            /// @brief Adds a file at @p path holding @p text.
            void add_file(const std::string& path, const std::string& text);

            /// @brief Sets the variable @p name to @p value; an empty value reads as unset.
            void set_variable(const std::string& name, const std::string& value);

            /// @brief Sets the current directory.
            void set_working_directory(const std::string& path);

            [[nodiscard]] bool exists(const std::string& path) const override;
            [[nodiscard]] std::unique_ptr<std::istream> open(const std::string& path) const override;
            [[nodiscard]] std::string variable(const std::string& name) const override;
            [[nodiscard]] std::string working_directory(std::error_code& ec) const override;

        private:
            /// @brief The files by generic path.
            std::map<std::string, std::string> files_;
            /// @brief The variables by name.
            std::map<std::string, std::string> variables_;
            /// @brief The current directory.
            std::string working_directory_;
    };
} // namespace SushiTrack
