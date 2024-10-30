/*
 * Copyright (c) 2024 Emmett Zhang
 *
 * Licensed under the Apache License Version 2.0 with LLVM Exceptions
 * (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 *   https://llvm.org/LICENSE.txt
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "tools/clang_tidy/clang_tidy.h"

#include <boost/program_options.hpp>

#include "tools/clang_tidy/general/option.h"
#include "tools/clang_tidy/version/v18.h"
#include "tools/util.h"

namespace lint::tool::clang_tidy {
  namespace {
    constexpr auto enable               = "enable-clang-tidy";
    constexpr auto enable_fastly_exit   = "enable-clang-tidy-fastly-exit";
    constexpr auto version              = "clang-tidy-version";
    constexpr auto binary               = "clang-tidy-binary";
    constexpr auto file_iregex          = "clang-tidy-file-iregex";
    constexpr auto database             = "clang-tidy-database";
    constexpr auto allow_no_checks      = "clang-tidy-allow-no-checks";
    constexpr auto enable_check_profile = "clang-tidy-enable-check-profile";
    constexpr auto checks               = "clang-tidy-checks";
    constexpr auto config               = "clang-tidy-config";
    constexpr auto config_file          = "clang-tidy-config-file";
    constexpr auto header_filter        = "clang-tidy-header-filter";
    constexpr auto line_filter          = "clang-tidy-line-filter";
  } // namespace

  // Get version from clang-tidy output.
  // Example: Ubuntu LLVM version 18.1.3
  auto get_version(const std::string &binary) -> std::string {
    auto [ec, std_out, std_err] = shell::execute(binary, {"--version"});
    if (ec != 0) {
      return "";
    }
    constexpr auto version_regex = R"(version\ (\d+\.\d+\.\d+))";
    auto regex                   = boost::regex{version_regex};
    auto match                   = boost::smatch{};
    auto matched                 = boost::regex_search(std_out, match, regex, boost::match_extra);
    throw_unless(matched, "Can't get clang-tidy version");
    return match[1].str();
  }

  void creator::register_option(program_options::options_description &desc) const {
    using program_options::value;

    const auto *ver    = value<std::string>()->value_name("version");
    const auto *bin    = value<std::string>()->value_name("path");
    const auto *iregex = value<std::string>()->value_name("iregex")->default_value(
      option.file_filter_iregex);
    const auto *db = value<std::string>()->value_name("path")->default_value("build");

    auto boolean = [](bool def) {
      return value<bool>()->value_name("bool")->default_value(def);
    };

    auto str = []() {
      return value<std::string>()->value_name("string")->default_value("");
    };


    // clang-format off
    desc.add_options()
      (enable,                boolean(true),   "Enabel clang-tidy check")
      (enable_fastly_exit,    boolean(false),  "Enabel clang-tidy fastly exit. This means "
                                               "cpp-lint-action will stop all clang-tidy as soon "
                                               "as first file error occurs")
      (version,               ver,             "Set the version of clang-tidy. Don't specify "
                                               "both this option and the clang-tidy-binary option, "
                                               "to avoid ambigous. And the clang-tidy-${version} must "
                                               "exist in your $PATH")
      (binary,                bin,             "Set the full path of clang-format executable binary. "
                                               "Don't spefify both this option and the clang-format-version "
                                               "option to avoid ambigous")
      (file_iregex,           iregex,          "Set the source file filter for clang-format.")
      (database,              db,              "Same as clang-tidy -p option")
      (allow_no_checks,       boolean(false),  "Enabel clang-tidy allow_no_check option")
      (enable_check_profile,  boolean(false),  "Enabel clang-tidy enable_check_profile option")
      (checks,                str(),           "Same as clang-tidy checks option")
      (config,                str(),           "Same as clang-tidy config option")
      (config_file,           str(),           "Same as clang-tidy config-file option")
      (header_filter,         str(),           "Same as clang-tidy header-filter option")
      (line_filter,           str(),           "Same as clang-tidy line-filter option")
    ;
    // clang-format on
  }

  void creator::create_option(const program_options::variables_map &variables) {
    spdlog::trace("Enter create_option()");
    option.enabled = variables[enable].as<bool>();
    if (!option.enabled) {
      // Speed up option creation
      return;
    }

    if (variables.contains(enable_fastly_exit)) {
      option.enabled_fastly_exit = variables[enable_fastly_exit].as<bool>();
    }

    // Get clang-tidy-binary
    if (variables.contains(version)) {
      program_options::must_not_specify("specify clang-tidy-version", variables, {binary});
      auto user_input_version = variables[version].as<std::string>();
      spdlog::debug("user inputs clang-tidy version: {}", user_input_version);

      option.binary = find_clang_tool("clang-tidy", user_input_version);
    } else if (variables.contains(binary)) {
      program_options::must_not_specify("specify clang-tidy-binary", variables, {version});

      option.binary               = variables[binary].as<std::string>();
      auto [ec, std_out, std_err] = shell::which(option.binary);
      throw_unless(ec == 0, fmt::format("Can't find given clang-tidy binary: {}", option.binary));
    } else {
      auto [ec, std_out, std_err] = shell::which("clang-tidy");
      throw_unless(ec == 0, "can't find clang-tidy");
      option.binary = std_out;
    }

    option.version = get_version(option.binary);

    if (variables.contains(file_iregex)) {
      option.file_filter_iregex = variables[file_iregex].as<std::string>();
    }
    if (variables.contains(database)) {
      option.database = variables[database].as<std::string>();
    }
    if (variables.contains(allow_no_checks)) {
      option.allow_no_checks = variables[allow_no_checks].as<bool>();
    }
    if (variables.contains(enable_check_profile)) {
      option.enable_check_profile = variables[enable_check_profile].as<bool>();
    }
    if (variables.contains(checks)) {
      option.checks = variables[checks].as<std::string>();
    }
    if (variables.contains(config)) {
      option.config = variables[config].as<std::string>();
    }
    if (variables.contains(config_file)) {
      option.config_file = variables[config_file].as<std::string>();
    }
    if (variables.contains(header_filter)) {
      option.header_filter = variables[header_filter].as<std::string>();
    }
    if (variables.contains(line_filter)) {
      option.line_filter = variables[line_filter].as<std::string>();
    }
  }

  auto creator::create_tool(const program_options::variables_map &variables) -> tool_base_ptr {
    spdlog::trace("Enter create_tool()");
    create_option(variables);
    if (!option.enabled) {
      return nullptr;
    }
    print_option(option);

    auto version = option.version;
    auto tool    = tool_base_ptr{};
    if (version == version_18_1_3) {
      tool = std::make_unique<clang_tidy_v18_1_3>(option);
    } else if (version == version_18_1_0) {
      tool = std::make_unique<clang_tidy_v18_1_0>(option);
    } else {
      tool = std::make_unique<clang_tidy_general>(option);
    }

    const auto os   = operating_system_t::ubuntu;
    const auto arch = arch_t::x86_64;
    throw_unless(tool->is_supported(os, arch),
                 fmt::format("Create clang-tidy {} instance failed since not "
                             "supported on this platform",
                             version));
    return tool;
  }

  [[nodiscard]] auto creator::get_option() const -> const option_t & {
    return option;
  }

} // namespace lint::tool::clang_tidy
