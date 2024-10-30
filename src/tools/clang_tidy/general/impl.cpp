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
#include "tools/clang_tidy/general/impl.h"

#include <cctype>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <boost/regex.hpp>
#include <spdlog/spdlog.h>
#include <tinyxml2.h>

#include "tools/clang_tidy/general/reporter.h"
#include "utils/common.h"
#include "utils/shell.h"

namespace lint::tool::clang_tidy {
  using namespace std::string_view_literals;

  namespace {
    constexpr auto supported_serverity = {"warning"sv, "info"sv, "error"sv};

    // Parse the header line of clang-tidy. If the given line meets header line
    // rule, parse it. Otherwise return std::nullopt.
    auto parse_diagnostic_header(std::string_view line) -> std::optional<diagnostic_header> {
      auto parts = line | ranges::views::split(':') | ranges::to<std::vector<std::string>>();

      if (std::distance(parts.begin(), parts.end()) != 5) {
        return std::nullopt;
      }
      auto iter             = parts.begin();
      auto file_name        = std::string_view{*iter++};
      auto row_idx          = std::string_view{*iter++};
      auto col_idx          = std::string_view{*iter++};
      auto serverity        = trim_left(std::string_view{*iter++});
      auto diagnostics_type = std::string_view{*iter++};

      if (!ranges::all_of(row_idx, ::isdigit)) {
        return std::nullopt;
      }
      if (!ranges::all_of(col_idx, ::isdigit)) {
        return std::nullopt;
      }
      if (!ranges::contains(supported_serverity, serverity)) {
        return std::nullopt;
      }

      const auto *square_brackets = ranges::find(diagnostics_type, '[');
      if ((square_brackets == diagnostics_type.end())
          || (diagnostics_type.size() < 3)
          || (diagnostics_type.back() != ']')) {
        return std::nullopt;
      }
      auto brief_info = std::string_view{diagnostics_type.begin(), square_brackets};

      // multiple diagnostics without "[" and "]"
      auto diagnostics = std::string_view{square_brackets + 1, diagnostics_type.end() - 1};

      auto header      = diagnostic_header{};
      header.file_name = file_name;
      header.row_idx   = row_idx;
      header.col_idx   = col_idx;
      header.serverity = serverity;
      header.brief     = brief_info;
      header.checks    = diagnostics;
      return header;
    }

    auto execute(const option_t &option, std::string_view repo, std::string_view file)
      -> std::tuple<shell::result, std::string> {
      spdlog::trace("Enter execute()");

      auto opts = std::vector<std::string>{};
      if (!option.database.empty()) {
        opts.emplace_back(fmt::format("-p={}", option.database));
      }
      if (!option.checks.empty()) {
        opts.emplace_back(fmt::format("-checks={}", option.checks));
      }
      if (option.allow_no_checks) {
        opts.emplace_back("--allow-no-checks");
      }
      if (!option.config.empty()) {
        opts.emplace_back(fmt::format("--config={}", option.config));
      }
      if (!option.config_file.empty()) {
        opts.emplace_back(fmt::format("--config-file={}", option.config_file));
      }
      if (option.enable_check_profile) {
        opts.emplace_back("--enable-check-profile");
      }
      if (!option.header_filter.empty()) {
        opts.emplace_back(fmt::format("--header-filter={}", option.header_filter));
      }
      if (!option.line_filter.empty()) {
        opts.emplace_back(fmt::format("--line-filter={}", option.line_filter));
      }

      opts.emplace_back(file);

      auto arg_str = concat(opts, ' ');
      spdlog::info("Running command: {} {}", option.binary, arg_str);

      return {shell::execute(option.binary, opts, repo), arg_str};
    }

    auto parse_stdout(std::string_view std_out) -> diagnostics {
      spdlog::trace("Enter parse_stdout()");
      auto diags         = diagnostics{};
      auto needs_details = false;

      for (auto part: ranges::views::split(std_out, '\n')) {
        auto line = ranges::to<std::string>(part);
        spdlog::trace("Parsing: {}", line);

        auto header_line = parse_diagnostic_header(line);
        if (header_line) {
          spdlog::trace(
            " Result: {}:{}:{}: {}:{}{}",
            header_line->file_name,
            header_line->row_idx,
            header_line->col_idx,
            header_line->serverity,
            header_line->brief,
            header_line->checks);

          diags.emplace_back(std::move(*header_line));
          needs_details = true;
          continue;
        }

        if (needs_details) {
          diags.back().details += line;
        }
      }

      spdlog::debug("Parsed clang tidy stdout, got {} diagnostics.", diags.size());
      return diags;
    }

    constexpr auto warning_and_error  = "^(\\d+) warnings and (\\d+) errors? generated.";
    constexpr auto warnings_generated = "^(\\d+) warnings? generated.";
    constexpr auto errors_generated   = "^(\\d+) errors? generated.";
    constexpr auto suppressed         = R"(Suppressed (\d+) warnings \((\d+) in non-user code\)\.)";
    constexpr auto suppressed_lint =
      R"(Suppressed (\d+) warnings \((\d+) in non-user code, (\d+) NOLINT\)\.)";
    constexpr auto warnings_as_errors = "^(\\d+) warnings treated as errors";

    void try_match(const std::string &line, const char *regex_str, auto callback) {
      auto regex   = boost::regex{regex_str};
      auto match   = boost::smatch{};
      auto matched = boost::regex_match(line, match, regex, boost::match_extra);
      if (matched) {
        callback(match);
      }
    };

    // TODO: remove?
    auto parse_stderr(std::string_view std_err) -> statistic {
      auto stat                 = statistic{};
      auto warning_and_error_cb = [&](boost::smatch &match) {
        spdlog::trace(" Result: {} warnings and {} error(s) generated.",
                      match[1].str(),
                      match[2].str());
        stat.warnings = stoi(match[1].str());
        stat.errors   = stoi(match[2].str());
      };

      auto warnings_generated_cb = [&](boost::smatch &match) {
        spdlog::trace(" Result: {} warning(s) generated.", match[1].str());
        stat.warnings = stoi(match[1].str());
      };

      auto errors_generated_cb = [&](boost::smatch &match) {
        spdlog::trace(" Result: {} error(s) generated.", match[1].str());
        stat.errors = stoi(match[1].str());
      };

      auto suppressed_cb = [&](boost::smatch &match) {
        spdlog::trace(" Result: Suppressed {} warnings ({} in non-user code).",
                      match[1].str(),
                      match[2].str());
        stat.total_suppressed_warnings = stoi(match[1].str());
        stat.non_user_code_warnings    = stoi(match[2].str());
      };

      auto suppressed_and_nolint_cb = [&](boost::smatch &match) {
        spdlog::trace(" Result: Suppressed {} warnings ({} in non-user code, {} NOLINT).",
                      match[1].str(),
                      match[2].str(),
                      match[3].str());
        stat.total_suppressed_warnings = stoi(match[1].str());
        stat.non_user_code_warnings    = stoi(match[2].str());
        stat.no_lint_warnings          = stoi(match[3].str());
      };

      auto warnings_as_errors_cb = [&](boost::smatch &match) {
        spdlog::trace(" Result: {} warnings treated as errors", match[1].str());
        stat.warnings_treated_as_errors = stoi(match[1].str());
      };

      for (auto part: std::views::split(std_err, '\n')) {
        auto line = ranges::to<std::string>(part);
        spdlog::trace("Parsing: {}", line);
        try_match(line, warning_and_error, warning_and_error_cb);
        try_match(line, warnings_generated, warnings_generated_cb);
        try_match(line, errors_generated, errors_generated_cb);
        try_match(line, suppressed, suppressed_cb);
        try_match(line, warnings_as_errors, warnings_as_errors_cb);
        try_match(line, suppressed_lint, suppressed_and_nolint_cb);
      }

      return stat;
    }
  } // namespace

  auto clang_tidy_general::check_single_file(
    [[maybe_unused]] const runtime_context &context,
    const std::string &root_dir,
    const std::string &file) const -> per_file_result {
    spdlog::trace("Enter clang_tidy_general::check_single_file()");

    auto [res, failed_command] = execute(option, root_dir, file);

    auto result        = per_file_result{};
    result.passed      = res.exit_code == 0;
    result.diags       = parse_stdout(res.std_out);
    result.tool_stdout = res.std_out;
    result.tool_stderr = res.std_err;
    result.file_path   = file;
    result.file_option = failed_command;
    return result;
  }

  void clang_tidy_general::check(const runtime_context &context) {
    spdlog::trace("Enter clang_tidy_general::check");
    assert(!option.binary.empty() && "clang-tidy binary is empty");
    assert(!context.repo_path.empty() && "the repo_path of context is empty");

    const auto root_dir = context.repo_path;
    const auto files    = context.changed_files;
    for (const auto &file: files) {
      const auto &delta = context.deltas.at(file);
      if (delta.status == GIT_DELTA_DELETED) {
        continue;
      }
      if (filter_file(option.file_filter_iregex, file)) {
        result.ignored.push_back(file);
        spdlog::debug("file {} is ignored by {}", file, option.binary);
        continue;
      }

      auto per_file_result = check_single_file(context, root_dir, file);
      if (per_file_result.passed) {
        spdlog::info("file: {} passes {} check.", file, option.binary);
        result.passes[file] = std::move(per_file_result);
        continue;
      }

      spdlog::error("file: {} doesn't pass {} check.", file, option.binary);
      result.failed_commands.emplace_back(
        fmt::format("clang-tidy {}", per_file_result.file_option));
      result.fails[file] = std::move(per_file_result);

      if (option.enabled_fastly_exit) {
        spdlog::info("{} fastly exit since check failed", option.binary);
        result.final_passed  = false;
        result.fastly_exited = true;
        return;
      }
    }

    result.final_passed = result.fails.empty();
  }

  auto clang_tidy_general::get_reporter() -> reporter_base_ptr {
    return std::make_unique<reporter_t>(option, result);
  }

} // namespace lint::tool::clang_tidy
