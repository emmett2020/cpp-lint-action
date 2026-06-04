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

#include "tools/clang_tidy/general/reporter.h"
#include "tools/clang_format/general/reporter.h"
#include "test_common.h"
#include "utils/env_manager.h"
#include "github/common.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace lint;

namespace {
  class fake_review_reporter : public tool::reporter_base {
  public:
    explicit fake_review_reporter(std::size_t comment_count)
      : comment_count_{comment_count} {
    }

    auto get_brief_result() -> std::tuple<bool, std::size_t, std::size_t, std::size_t> override {
      return {false, 0, comment_count_, 0};
    }

    auto get_detail_result(const runtime_context &) -> std::string override {
      return {};
    }

    auto make_review_comment(const runtime_context &) -> github::review_comments override {
      auto comments = github::review_comments{};
      for (std::size_t index = 0; index < comment_count_; ++index) {
        comments.push_back(github::review_comment{
          .path     = fmt::format("file_{}.cpp", index),
          .position = index + 1,
          .body     = fmt::format("comment {}", index),
        });
      }
      return comments;
    }

    void write_to_action_output(const runtime_context &) override {
    }

    auto get_failed_commands() -> std::vector<std::string> override {
      return {};
    }

    auto tool_name() -> std::string override {
      return "fake-reviewer";
    }

  private:
    std::size_t comment_count_;
  };

  auto read_text_file(const std::filesystem::path &path) -> std::string {
    auto file = std::ifstream{path};
    REQUIRE(file.is_open());
    return {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
  }

  void write_empty_file(const std::filesystem::path &path) {
    auto file = std::ofstream{path};
    REQUIRE(file.is_open());
  }
} // namespace

TEST_CASE("Test all_passes function", "[cpp-lint-action][tools]") {
  auto tidy_opt = tool::clang_tidy::option_t{};
  auto tidy_res = tool::clang_tidy::result_t{};

  auto format_opt = tool::clang_format::option_t{};
  auto format_res = tool::clang_format::result_t{};

  SECTION("all passes") {
    tidy_res.final_passed   = true;
    format_res.final_passed = true;

    auto reporters = std::vector<tool::reporter_base_ptr>();
    reporters.emplace_back(std::make_unique<tool::clang_tidy::reporter_t>(tidy_opt, tidy_res));
    reporters.emplace_back(
      std::make_unique<tool::clang_format::reporter_t>(format_opt, format_res));
    REQUIRE(tool::all_passed(reporters));
  }

  SECTION("at least one failed") {
    tidy_res.final_passed   = false;
    format_res.final_passed = true;

    auto reporters = std::vector<tool::reporter_base_ptr>();
    reporters.emplace_back(std::make_unique<tool::clang_tidy::reporter_t>(tidy_opt, tidy_res));
    reporters.emplace_back(
      std::make_unique<tool::clang_format::reporter_t>(format_opt, format_res));
    REQUIRE(!tool::all_passed(reporters));
  }
}

TEST_CASE("Reporters write action output and stable summary markdown",
          "[cpp-lint-action][tools][reporter]") {
  auto temp_dir = std::filesystem::temp_directory_path() / "cpp_lint_action_reporter_test";
  std::filesystem::remove_all(temp_dir);
  std::filesystem::create_directories(temp_dir);
  auto guard = scope_guard{[&] { std::filesystem::remove_all(temp_dir); }};

  auto output_file  = temp_dir / "github-output.txt";
  auto summary_file = temp_dir / "github-summary.md";
  write_empty_file(output_file);
  write_empty_file(summary_file);

  env::set_cache({
    {github::github_output, output_file.string()},
    {github::github_step_summary, summary_file.string()},
  });
  auto env_guard = scope_guard{[] { env::set_cache({}); }};

  auto format_opt    = tool::clang_format::option_t{};
  format_opt.binary  = "/tmp/clang-format-fake";
  auto format_res    = tool::clang_format::result_t{};
  format_res.final_passed = false;
  format_res.failed_commands.push_back("clang-format --output-replacements-xml format_fail.cpp");
  auto format_file       = tool::clang_format::per_file_result{};
  format_file.file_path  = "format_fail.cpp";
  format_file.passed     = false;
  format_res.fails["format_fail.cpp"] = format_file;

  auto tidy_opt   = tool::clang_tidy::option_t{};
  tidy_opt.binary = "/tmp/clang-tidy-fake";
  auto tidy_res   = tool::clang_tidy::result_t{};
  tidy_res.final_passed = false;
  tidy_res.failed_commands.push_back("clang-tidy -p=build tidy_fail.cpp");
  auto tidy_file       = tool::clang_tidy::per_file_result{};
  tidy_file.file_path  = "tidy_fail.cpp";
  tidy_file.passed     = false;
  auto diag            = tool::clang_tidy::diagnostic{};
  diag.header.row_idx  = "7";
  diag.header.col_idx  = "3";
  diag.header.serverity = "warning";
  diag.header.brief    = "fake tidy failure";
  diag.header.checks   = "cppcoreguidelines-avoid-magic-numbers";
  tidy_file.diags.push_back(diag);
  tidy_res.fails["tidy_fail.cpp"] = tidy_file;

  auto reporters = std::vector<tool::reporter_base_ptr>{};
  reporters.emplace_back(
    std::make_unique<tool::clang_format::reporter_t>(format_opt, format_res));
  reporters.emplace_back(std::make_unique<tool::clang_tidy::reporter_t>(tidy_opt, tidy_res));

  auto context = runtime_context{};
  tool::write_to_github_action_output(context, reporters);
  tool::write_to_github_step_summary(context, reporters);

  auto output = read_text_file(output_file);
  REQUIRE(output.find("clang_format_failed_number=1") != std::string::npos);
  REQUIRE(output.find("clang_tidy_failed_number=1") != std::string::npos);

  auto summary = read_text_file(summary_file);
  REQUIRE(summary.find("Analysis Report Generated") != std::string::npos);
  REQUIRE(summary.find("| **clang-format-fake** |") != std::string::npos);
  REQUIRE(summary.find("| **clang-tidy-fake** |") != std::string::npos);
  REQUIRE(summary.find("format_fail.cpp") != std::string::npos);
  REQUIRE(summary.find("tidy_fail.cpp") != std::string::npos);
  REQUIRE(summary.find("fake tidy failure") != std::string::npos);
  REQUIRE(summary.find("clang-format --output-replacements-xml format_fail.cpp")
          != std::string::npos);
  REQUIRE(summary.find("clang-tidy -p=build tidy_fail.cpp") != std::string::npos);
  REQUIRE(summary.find("Steps to <strong>reproduce</strong>") != std::string::npos);
}

TEST_CASE("Pull request review comments are capped and empty reviews are skipped",
          "[cpp-lint-action][tools][reporter]") {
  auto context                 = runtime_context{};
  context.max_review_comments = 4;

  auto reporters = std::vector<tool::reporter_base_ptr>{};
  reporters.emplace_back(std::make_unique<fake_review_reporter>(3));
  reporters.emplace_back(std::make_unique<fake_review_reporter>(2));

  auto comments = tool::collect_pull_request_review_comments(context, reporters);
  REQUIRE(comments.size() == 4);
  REQUIRE(context.report_notes.size() == 1);
  REQUIRE(context.report_notes.front().find("truncated from 5 to 4") != std::string::npos);

  REQUIRE_NOTHROW(tool::comment_on_github_pull_request_review(context, github::review_comments{}));
}
