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

#include "context.h"
#include "github/common.h"
#include "program_options.h"
#include "test_common.h"
#include "tools/base_creator.h"
#include "tools/clang_format/clang_format.h"
#include "tools/clang_format/general/impl.h"
#include "tools/clang_tidy/clang_tidy.h"
#include "tools/clang_tidy/general/impl.h"
#include "tools/util.h"
#include "utils/shell.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <tuple>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <range/v3/algorithm/contains.hpp>

using namespace lint;
using namespace lint::tool;

namespace {
  class temporary_dir {
  public:
    explicit temporary_dir(std::string name) {
      static auto counter = std::atomic_uint64_t{0};
      const auto stamp    = std::chrono::steady_clock::now().time_since_epoch().count();
      path_ = std::filesystem::temp_directory_path()
            / (std::move(name) + "_" + std::to_string(stamp) + "_"
               + std::to_string(counter.fetch_add(1)));
      std::filesystem::create_directories(path_);
    }

    ~temporary_dir() {
      std::error_code ec;
      std::filesystem::remove_all(path_, ec);
    }

    temporary_dir(const temporary_dir &)            = delete;
    temporary_dir &operator=(const temporary_dir &) = delete;

    auto path() const -> const std::filesystem::path & {
      return path_;
    }

  private:
    std::filesystem::path path_;
  };

  void write_text_file(const std::filesystem::path &path, const std::string &content) {
    auto file = std::ofstream{path};
    REQUIRE(file.is_open());
    file << content;
  }

  auto read_text_file(const std::filesystem::path &path) -> std::string {
    auto file = std::ifstream{path};
    REQUIRE(file.is_open());
    return {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
  }

  void make_executable(const std::filesystem::path &path) {
    std::filesystem::permissions(path,
                                 std::filesystem::perms::owner_exec
                                   | std::filesystem::perms::group_exec
                                   | std::filesystem::perms::others_exec,
                                 std::filesystem::perm_options::add);
  }

  auto write_fake_clang_format(const std::filesystem::path &dir) -> std::filesystem::path {
    const auto path = dir / "clang-format-fake";
    write_text_file(path, R"sh(#!/bin/sh
log="${FAKE_CLANG_LOG:-}"
if [ -n "$log" ]; then
  printf '%s\n' "$0 $*" >> "$log"
fi

for arg in "$@"; do
  if [ "$arg" = "--version" ]; then
    printf 'Ubuntu clang-format version 22.0.1\n'
    exit 0
  fi
done

last=""
for arg in "$@"; do
  last="$arg"
done
base=${last##*/}

case "$base" in
  format_malformed*)
    printf 'not xml\n'
    exit 0
    ;;
  format_error*)
    printf 'fake clang-format error for %s\n' "$base" >&2
    exit 7
    ;;
  format_fail*)
    printf '%s\n' '<?xml version="1.0"?><replacements xml:space="preserve" incomplete_format="false"><replacement offset="0" length="0"> </replacement></replacements>'
    exit 0
    ;;
  *)
    printf '%s\n' '<?xml version="1.0"?><replacements xml:space="preserve" incomplete_format="false"></replacements>'
    exit 0
    ;;
esac
)sh");
    make_executable(path);
    return path;
  }

  auto write_fake_clang_tidy(const std::filesystem::path &dir) -> std::filesystem::path {
    const auto path = dir / "clang-tidy-fake";
    write_text_file(path, R"sh(#!/bin/sh
log="${FAKE_CLANG_LOG:-}"
if [ -n "$log" ]; then
  printf '%s\n' "$0 $*" >> "$log"
fi

for arg in "$@"; do
  if [ "$arg" = "--version" ]; then
    printf 'Ubuntu LLVM version 22.0.1\n'
    exit 0
  fi
done

last=""
for arg in "$@"; do
  last="$arg"
done
base=${last##*/}

case "$base" in
  tidy_malformed*)
    printf 'this is not a clang-tidy diagnostic\n'
    exit 1
    ;;
  tidy_error*)
    printf 'fake clang-tidy error for %s\n' "$base" >&2
    exit 9
    ;;
  tidy_fail*)
    printf '%s:1:5: warning: fake tidy failure [cppcoreguidelines-avoid-non-const-global-variables]\n' "$last"
    printf 'int n;\n'
    printf '    ^\n'
    printf '1 warning generated.\n' >&2
    exit 1
    ;;
  *)
    exit 0
    ;;
esac
)sh");
    make_executable(path);
    return path;
  }

  auto make_context(repo_t &repo, const std::string &target, const std::string &source)
    -> runtime_context {
    auto context      = runtime_context{};
    context.repo_path = repo.get_path();
    context.target    = target;
    context.source    = source;
    fill_git_info(context);
    return context;
  }

  auto commit_changes(repo_t &repo) -> std::string {
    auto id = repo.commit_changes();
    id.erase(std::find(id.begin(), id.end(), '\0'), id.end());
    return id;
  }

  auto contains_text(const std::string &text, const std::string &needle) -> bool {
    return text.find(needle) != std::string::npos;
  }

  auto get_env_value(const char *name) -> std::string {
    const auto *value = std::getenv(name);
    return value == nullptr ? std::string{} : std::string{value};
  }

  template <class... Opts>
  auto make_opt(Opts &&...opts) -> std::array<char *, sizeof...(Opts) + 1> {
    return {const_cast<char *>("cpp-lint-action"), // NOLINT
            const_cast<char *>(std::forward<Opts &&>(opts))...};
  }

  template <class... Opts>
  auto parse_opt(const program_options::options_description &desc, Opts &&...opts)
    -> program_options::variables_map {
    auto inputs = make_opt(std::forward<Opts &&>(opts)...);
    return program_options::parse(inputs.size(), inputs.data(), desc);
  }

  auto make_tool_desc(const clang_format::creator &format_creator,
                      const clang_tidy::creator &tidy_creator)
    -> program_options::options_description {
    auto desc = program_options::create_desc();
    format_creator.register_option(desc);
    tidy_creator.register_option(desc);
    return desc;
  }
} // namespace

TEST_CASE("Fake clang-format keeps format checks available without a local clang install",
          "[cpp-lint-action][tool][clang_format][fake]") {
  auto fake_dir = temporary_dir{"cpp_lint_action_fake_format"};
  auto binary   = write_fake_clang_format(fake_dir.path());

  auto repo = repo_t{};
  repo.add_file("format_pass.cpp", "int value = 0;\n");
  repo.add_file("format_fail.cpp", "int value = 0;\n");
  auto target = commit_changes(repo);
  repo.rewrite_file("format_pass.cpp", "int value = 1;\n");
  repo.rewrite_file("format_fail.cpp", "int value = 2;\n");
  auto source = commit_changes(repo);

  auto context = make_context(repo, target, source);

  auto option    = clang_format::option_t{};
  option.enabled = true;
  option.binary  = binary.string();
  option.version = "22.0.1";

  auto format = clang_format::clang_format_general{option};
  format.check(context);

  REQUIRE(!format.result.final_passed);
  REQUIRE(format.result.passes.size() == 1);
  REQUIRE(format.result.fails.size() == 1);
  REQUIRE(format.result.passes.contains("format_pass.cpp"));
  REQUIRE(format.result.fails.contains("format_fail.cpp"));
  REQUIRE(format.result.failed_commands.size() == 1);
  REQUIRE(contains_text(format.result.failed_commands.front(), "format_fail.cpp"));
}

TEST_CASE("Fake clang-format covers command errors and malformed XML",
          "[cpp-lint-action][tool][clang_format][fake]") {
  auto fake_dir = temporary_dir{"cpp_lint_action_fake_format_errors"};
  auto binary   = write_fake_clang_format(fake_dir.path());

  auto option    = clang_format::option_t{};
  option.enabled = true;
  option.binary  = binary.string();
  option.version = "22.0.1";

  SECTION("non-zero exit code is reported as a failed file") {
    auto repo = repo_t{};
    repo.add_file("format_error.cpp", "int value = 0;\n");
    auto target = commit_changes(repo);
    repo.rewrite_file("format_error.cpp", "int value = 1;\n");
    auto source = commit_changes(repo);

    auto context = make_context(repo, target, source);
    auto format  = clang_format::clang_format_general{option};
    format.check(context);

    REQUIRE(!format.result.final_passed);
    REQUIRE(format.result.fails.contains("format_error.cpp"));
    REQUIRE(contains_text(format.result.fails.at("format_error.cpp").tool_stderr,
                          "fake clang-format error"));
  }

  SECTION("malformed replacement XML throws a parsing error") {
    auto repo = repo_t{};
    repo.add_file("format_malformed.cpp", "int value = 0;\n");
    auto target = commit_changes(repo);
    repo.rewrite_file("format_malformed.cpp", "int value = 1;\n");
    auto source = commit_changes(repo);

    auto context = make_context(repo, target, source);
    auto format  = clang_format::clang_format_general{option};
    REQUIRE_THROWS(format.check(context));
  }
}

TEST_CASE("Fake clang-tidy keeps tidy checks available without a local clang install",
          "[cpp-lint-action][tool][clang_tidy][fake]") {
  auto fake_dir = temporary_dir{"cpp_lint_action_fake_tidy"};
  auto binary   = write_fake_clang_tidy(fake_dir.path());

  auto repo = repo_t{};
  repo.add_file("tidy_pass.cpp", "int value = 0;\n");
  repo.add_file("tidy_fail.cpp", "int value = 0;\n");
  auto target = commit_changes(repo);
  repo.rewrite_file("tidy_pass.cpp", "int value = 1;\n");
  repo.rewrite_file("tidy_fail.cpp", "int value = 2;\n");
  auto source = commit_changes(repo);

  auto context = make_context(repo, target, source);

  auto option    = clang_tidy::option_t{};
  option.enabled = true;
  option.binary  = binary.string();
  option.version = "22.0.1";

  auto tidy = clang_tidy::clang_tidy_general{option};
  tidy.check(context);

  REQUIRE(!tidy.result.final_passed);
  REQUIRE(tidy.result.passes.size() == 1);
  REQUIRE(tidy.result.fails.size() == 1);
  REQUIRE(tidy.result.passes.contains("tidy_pass.cpp"));
  REQUIRE(tidy.result.fails.contains("tidy_fail.cpp"));
  REQUIRE(tidy.result.fails.at("tidy_fail.cpp").diags.size() == 1);
  REQUIRE(tidy.result.fails.at("tidy_fail.cpp").diags.front().header.serverity == "warning");
  REQUIRE(tidy.result.fails.at("tidy_fail.cpp").diags.front().header.checks
          == "cppcoreguidelines-avoid-non-const-global-variables");
}

TEST_CASE("Fake clang-tidy covers command errors and malformed diagnostics",
          "[cpp-lint-action][tool][clang_tidy][fake]") {
  auto fake_dir = temporary_dir{"cpp_lint_action_fake_tidy_errors"};
  auto binary   = write_fake_clang_tidy(fake_dir.path());

  auto option    = clang_tidy::option_t{};
  option.enabled = true;
  option.binary  = binary.string();
  option.version = "22.0.1";

  SECTION("non-zero exit code is reported as a failed file") {
    auto repo = repo_t{};
    repo.add_file("tidy_error.cpp", "int value = 0;\n");
    auto target = commit_changes(repo);
    repo.rewrite_file("tidy_error.cpp", "int value = 1;\n");
    auto source = commit_changes(repo);

    auto context = make_context(repo, target, source);
    auto tidy    = clang_tidy::clang_tidy_general{option};
    tidy.check(context);

    REQUIRE(!tidy.result.final_passed);
    REQUIRE(tidy.result.fails.contains("tidy_error.cpp"));
    REQUIRE(contains_text(tidy.result.fails.at("tidy_error.cpp").tool_stderr,
                          "fake clang-tidy error"));
  }

  SECTION("malformed diagnostics still produce a failed file with raw stdout") {
    auto repo = repo_t{};
    repo.add_file("tidy_malformed.cpp", "int value = 0;\n");
    auto target = commit_changes(repo);
    repo.rewrite_file("tidy_malformed.cpp", "int value = 1;\n");
    auto source = commit_changes(repo);

    auto context = make_context(repo, target, source);
    auto tidy    = clang_tidy::clang_tidy_general{option};
    tidy.check(context);

    REQUIRE(!tidy.result.final_passed);
    REQUIRE(tidy.result.fails.contains("tidy_malformed.cpp"));
    REQUIRE(tidy.result.fails.at("tidy_malformed.cpp").diags.empty());
    REQUIRE(contains_text(tidy.result.fails.at("tidy_malformed.cpp").tool_stdout,
                          "not a clang-tidy diagnostic"));
  }
}

TEST_CASE("Tool creators accept explicit fake binaries and reject ambiguous tool selection",
          "[cpp-lint-action][program_options][tool][fake]") {
  auto fake_dir      = temporary_dir{"cpp_lint_action_fake_creator"};
  auto format_binary = write_fake_clang_format(fake_dir.path());
  auto tidy_binary   = write_fake_clang_tidy(fake_dir.path());

  auto format_creator = clang_format::creator{};
  auto tidy_creator   = clang_tidy::creator{};
  auto desc           = make_tool_desc(format_creator, tidy_creator);

  SECTION("explicit binaries are enough to create enabled tools") {
    auto vars = parse_opt(desc,
                          "--target-revision=main",
                          ("--clang-format-binary=" + format_binary.string()).c_str(),
                          ("--clang-tidy-binary=" + tidy_binary.string()).c_str());

    auto creators = std::vector<creator_base_ptr>{};
    creators.emplace_back(std::make_unique<clang_format::creator>());
    creators.emplace_back(std::make_unique<clang_tidy::creator>());

    auto tools = create_enabled_tools(creators, vars);
    REQUIRE(tools.size() == 2);
    REQUIRE(tools[0]->version() == "22.0.1");
    REQUIRE(tools[1]->version() == "22.0.1");
  }

  SECTION("specifying both binary and version is rejected") {
    auto vars = parse_opt(desc,
                          "--target-revision=main",
                          "--clang-format-version=22",
                          ("--clang-format-binary=" + format_binary.string()).c_str(),
                          ("--clang-tidy-binary=" + tidy_binary.string()).c_str());
    REQUIRE_THROWS(format_creator.create_tool(vars));
  }
}

TEST_CASE("End-to-end binary smoke test runs tools, writes outputs, and controls exit code",
          "[cpp-lint-action][e2e][fake]") {
  auto fake_dir      = temporary_dir{"cpp_lint_action_e2e"};
  auto format_binary = write_fake_clang_format(fake_dir.path());
  auto tidy_binary   = write_fake_clang_tidy(fake_dir.path());

  auto repo = repo_t{};
  repo.add_file("format_fail.cpp", "int value = 0;\n");
  repo.add_file("tidy_fail.cpp", "int value = 0;\n");
  auto target = commit_changes(repo);
  repo.rewrite_file("format_fail.cpp", "int value = 1;\n");
  repo.rewrite_file("tidy_fail.cpp", "int value = 2;\n");
  auto source = commit_changes(repo);

  auto run_action = [&](bool disable_errors) {
    auto run_dir      = temporary_dir{"cpp_lint_action_e2e_run"};
    auto output_file  = run_dir.path() / "github-output.txt";
    auto summary_file = run_dir.path() / "github-summary.md";
    auto log_file     = run_dir.path() / "fake-tools.log";
    write_text_file(output_file, "");
    write_text_file(summary_file, "");
    write_text_file(log_file, "");

    auto args = std::vector<std::string>{
      "--log-level=error",
      "--target-revision=" + target,
      "--enable-comment-on-issue=false",
      "--enable-pull-request-review=false",
      "--enable-step-summary=true",
      "--enable-action-output=true",
      std::string{"--disable-errors="} + (disable_errors ? "true" : "false"),
      "--jobs=2",
      "--max-review-comments=25",
      "--enable-clang-format=true",
      "--enable-clang-format-fastly-exit=false",
      "--clang-format-binary=" + format_binary.string(),
      "--enable-clang-tidy=true",
      "--enable-clang-tidy-fastly-exit=false",
      "--clang-tidy-binary=" + tidy_binary.string(),
      "--clang-tidy-allow-no-checks=true",
      "--clang-tidy-enable-check-profile=false",
    };

    auto env = shell::envrionment{
      {"PATH", get_env_value("PATH")},
      {"DEBUG_CPP_LINT_ACTION", "1"},
      {"DEBUG_GITHUB_REPOSITORY", "owner/repo"},
      {"DEBUG_GITHUB_TOKEN", "github-token"},
      {"DEBUG_GITHUB_EVENT_NAME", github::github_event_push},
      {"DEBUG_GITHUB_BASE_REF", "main"},
      {"DEBUG_GITHUB_HEAD_REF", "feature"},
      {"DEBUG_GITHUB_REF", "refs/heads/feature"},
      {"DEBUG_GITHUB_REF_TYPE", "branch"},
      {"DEBUG_GITHUB_SHA", source},
      {"DEBUG_GITHUB_WORKSPACE", repo.get_path().string()},
      {github::github_output, output_file.string()},
      {github::github_step_summary, summary_file.string()},
      {"FAKE_CLANG_LOG", log_file.string()},
    };

    auto result =
      shell::execute(CPP_LINT_ACTION_BINARY, args, env, repo.get_path().string());
    return std::tuple{
      result,
      read_text_file(output_file),
      read_text_file(summary_file),
      read_text_file(log_file),
    };
  };

  auto [failed_run, failed_output, failed_summary, failed_log] = run_action(false);
  CAPTURE(failed_run.exit_code);
  CAPTURE(failed_run.std_out);
  CAPTURE(failed_run.std_err);
  REQUIRE(failed_run.exit_code == 1);
  REQUIRE(contains_text(failed_output, "clang_format_failed_number=1"));
  REQUIRE(contains_text(failed_output, "clang_tidy_failed_number=1"));
  REQUIRE(contains_text(failed_summary, "Analysis Report Generated"));
  REQUIRE(contains_text(failed_summary, "format_fail.cpp"));
  REQUIRE(contains_text(failed_summary, "tidy_fail.cpp"));
  REQUIRE(contains_text(failed_summary, "Steps to <strong>reproduce</strong>"));
  REQUIRE(contains_text(failed_log, "clang-format-fake"));
  REQUIRE(contains_text(failed_log, "clang-tidy-fake"));
  REQUIRE(contains_text(failed_log, "format_fail.cpp"));
  REQUIRE(contains_text(failed_log, "tidy_fail.cpp"));

  auto [disabled_run, disabled_output, disabled_summary, disabled_log] = run_action(true);
  REQUIRE(disabled_run.exit_code == 0);
  REQUIRE(contains_text(disabled_output, "clang_format_failed_number=1"));
  REQUIRE(contains_text(disabled_output, "clang_tidy_failed_number=1"));
}
