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
#include "program_options.h"
#include "test_common.h"
#include "tools/base_tool.h"
#include "tools/clang_tidy/clang_tidy.h"
#include "tools/clang_tidy/general/impl.h"
#include "tools/clang_tidy/general/reporter.h"
#include "tools/util.h"
#include "utils/shell.h"

#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

using namespace lint;
using namespace lint::tool;

// TODO: typical clang-tidy tools.

namespace {
  // Pass in c_str
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

  // Check whether local environment contains clang-tidy otherwise some checks
  // will be failed.
  bool has_clang_tidy() {
    auto [ec, std_out, std_err] = shell::which("clang-tidy");
    return ec == 0;
  }

  // Check whether local environment contains specific clang-tidy version
  // otherwise some checks will be failed.
  bool has_clang_tidy(std::string_view version) {
    try {
      find_clang_tool("clang-tidy", version);
      return true;
    } catch (std::runtime_error &err) {
      return false;
    }
  }

  auto create_then_register_tool_desc(const clang_tidy::creator &creator)
    -> program_options::options_description {
    auto desc = program_options::create_desc();
    creator.register_option(desc);
    return desc;
  }

  void check_result(
    tool_base &tool,
    bool expected,
    int expected_passed_num,
    int expected_failed_num,
    int expected_ignored_num) {
    auto [pass, passed, failed, ignored] = tool.get_reporter()->get_brief_result();
    REQUIRE(pass == expected);
    REQUIRE(passed == expected_passed_num);
    REQUIRE(failed == expected_failed_num);
    REQUIRE(ignored == expected_ignored_num);
  }

#define SKIP_IF_NO_CLANG_TIDY                                             \
  if (!has_clang_tidy()) {                                                \
    SKIP("Local environment doesn't have clang-tidy. So skip clang-tidy " \
         "unit tests.");                                                  \
  }

// NOLINTNEXTLINE
#define SKIP_IF_NOT_HAS_CLANG_TIDY_VERSION(version)                        \
  if (!has_clang_tidy(version)) {                                          \
    SKIP("Local environment doesn't have required clang-tidy version. So " \
         "skip clang-tidy unit tests.");                                   \
  }

} // namespace

TEST_CASE("Test register and create clang-tidy option",
          "[cpp-lint-action][program_options][tool][clang_tidy][creator]") {
  SKIP_IF_NO_CLANG_TIDY
  auto creator = std::make_unique<clang_tidy::creator>();
  auto desc    = create_then_register_tool_desc(*creator);

  SECTION("Explicitly enables clang-tidy should work") {
    auto opts = parse_opt(desc, "--target-revision=main", "--enable-clang-tidy=true");
    creator->create_option(opts);
    REQUIRE(creator->get_option().enabled);
  }

  SECTION("Explicitly disable clang-tidy should work") {
    auto opts = parse_opt(desc, "--target-revision=main", "--enable-clang-tidy=false");
    creator->create_option(opts);
    REQUIRE(creator->get_option().enabled == false);
  }

  SECTION("clang-tidy is defaultly enabled") {
    auto opts = parse_opt(desc, "--target-revision=main");
    creator->create_option(opts);
    REQUIRE(creator->get_option().enabled);
  }

  SECTION("Receive an invalid clang-tidy version should throw exception") {
    auto opts = parse_opt(desc, "--target-revision=main", "--clang-tidy-version=18.x.1");
    REQUIRE_THROWS(creator->create_option(opts));
  }

  SECTION("Receive an invalid clang-tidy binary should throw exception") {
    auto opts =
      parse_opt(desc, "--target-revision=main", "--clang-tidy-binary=/usr/bin/clang-tidy-invalid");
    REQUIRE_THROWS(creator->create_option(opts));
  }

  SECTION("Other options should be correctly created based on user input") {
    auto opts = parse_opt(
      desc,
      "--target-revision=main",
      "--enable-clang-tidy-fastly-exit=true",
      "--clang-tidy-file-iregex=*.cpp");
    creator->create_option(opts);
    auto option = creator->get_option();
    REQUIRE(option.enabled_fastly_exit == true);
    REQUIRE(option.file_filter_iregex == "*.cpp");
  }
}

TEST_CASE("Test clang-tidy should get full version even though user input a "
          "simplified version",
          "[cpp-lint-action][tool][clang_tidy][creator]") {
  SKIP_IF_NOT_HAS_CLANG_TIDY_VERSION("18")
  auto creator = std::make_unique<clang_tidy::creator>();
  auto desc    = create_then_register_tool_desc(*creator);

  auto vars = parse_opt(desc, "--target-revision=main", "--clang-tidy-version=18");

  auto context = runtime_context{};
  program_options::fill_context(vars, context);
  auto clang_tidy = creator->create_tool(vars);
  auto version    = clang_tidy->version();
  auto parts      = ranges::views::split(version, '.') | ranges::to<std::vector<std::string>>();
  REQUIRE(parts.size() == 3);
  REQUIRE(parts[0] == "18");
  REQUIRE(ranges::all_of(parts[1], isdigit));
  REQUIRE(ranges::all_of(parts[2], isdigit));
}

TEST_CASE("Create tool of spefific version should work",
          "[cpp-lint-action][tool][clang_tidy][creator]") {
  SKIP_IF_NOT_HAS_CLANG_TIDY_VERSION("18.1.3")
  auto creator = std::make_unique<clang_tidy::creator>();
  auto desc    = create_then_register_tool_desc(*creator);

  SECTION("version 18.1.3") {
    auto vars       = parse_opt(desc, "--target-revision=main", "--clang-tidy-version=18.1.3");
    auto clang_tidy = creator->create_tool(vars);
    auto context    = runtime_context{};
    program_options::fill_context(vars, context);
    REQUIRE(clang_tidy->version() == "18.1.3");
    REQUIRE(clang_tidy->name() == "clang-tidy");
  }
}

namespace {
  auto create_clang_tidy() -> clang_tidy::clang_tidy_general {
    auto option    = clang_tidy::option_t{};
    option.enabled = true;
    option.binary  = "/usr/bin/clang-tidy";
    return clang_tidy::clang_tidy_general{option};
  }

  auto create_runtime_context(const std::string &target, const std::string &source)
    -> runtime_context {
    auto context      = runtime_context{};
    context.repo_path = get_temp_repo_dir();
    context.target    = target;
    context.source    = source;
    fill_git_info(context);
    return context;
  }

} // namespace

TEST_CASE("Test clang-tidy could correctly handle file filter",
          "[cpp-lint-action][tool][clang_tidy][general_version]") {
  SKIP_IF_NO_CLANG_TIDY

  auto clang_tidy = create_clang_tidy();

  clang_tidy.option.file_filter_iregex = ".*.cpp";

  auto repo = repo_t{};
  repo.commit_clang_tidy();
  repo.add_file("file.cpp", "int n = 0;\n");
  repo.add_file("file.test", "int n = 0;\n");
  auto target = repo.commit_changes();
  repo.rewrite_file("file.cpp", "const int m = 0;\n");
  repo.add_file("file.test", "const int m = 0;\n");
  auto source = repo.commit_changes();

  auto context = create_runtime_context(target, source);
  clang_tidy.check(context);
  check_result(clang_tidy, true, 1, 0, 1);
}

TEST_CASE("Test clang-tidy could correctly handle various file level cases",
          "[cpp-lint-action][tool][clang_tidy][general_version]") {
  SKIP_IF_NO_CLANG_TIDY
  auto clang_tidy = create_clang_tidy();

  auto repo = repo_t{};
  repo.commit_clang_tidy();

  SECTION("DELETED files shouldn't be checked") {
    repo.add_file("test1.cpp", "const int n = 1;\n");
    repo.add_file("test2.cpp", "const int n = 1;\n");
    repo.add_file("test3.cpp", "const int n = 1;\n");
    auto target = repo.commit_changes();

    repo.remove_file("test1.cpp");
    repo.remove_file("test2.cpp");
    auto source = repo.commit_changes();

    auto context = create_runtime_context(target, source);
    clang_tidy.check(context);
    check_result(clang_tidy, true, 0, 0, 0);
  }

  SECTION("NEW added files should be checked") {
    repo.add_file("test1.cpp", "const int n = 1;\n");
    repo.add_file("test2.cpp", "const int n = 1;\n");
    auto target = repo.commit_changes();

    repo.add_file("test3.cpp", "int n;\n");
    repo.add_file("test4.cpp", "const int n = 1;\n");
    auto source = repo.commit_changes();

    auto context = create_runtime_context(target, source);
    clang_tidy.check(context);
    check_result(clang_tidy, false, 1, 1, 0);
  }

  SECTION("MODIFIED files should be checked") {
    repo.add_file("test1.cpp", "const int n = 1;\n");
    repo.add_file("test2.cpp", "const int n = 1;\n");
    auto target = repo.commit_changes();

    repo.rewrite_file("test2.cpp", "int n = 0;\n");
    auto source = repo.commit_changes();

    auto context = create_runtime_context(target, source);
    clang_tidy.check(context);
    check_result(clang_tidy, false, 0, 1, 0);
  }

  SECTION("The commit contains only delete file should check nothing") {
    repo.add_file("test1.cpp", "int n = 1;\n");
    auto target = repo.commit_changes();

    repo.remove_file("test1.cpp");
    auto source = repo.commit_changes();

    auto context = create_runtime_context(target, source);
    clang_tidy.check(context);
    check_result(clang_tidy, true, 0, 0, 0);
  }

  SECTION("The commit contains one modified file and insert a new non-cpp file should check "
          "only one files") {
    repo.add_file("test1.cpp", "int n = 1;\n");
    repo.add_file("test2.cpp", "int n = 1;\n");
    repo.add_file("test3.cpp", "int n = 1;\n");
    auto target = repo.commit_changes();

    repo.rewrite_file("test1.cpp", "const int n = 1;\n");
    repo.add_file("test4.unknown", "int n = 1");
    auto source = repo.commit_changes();

    auto context = create_runtime_context(target, source);
    clang_tidy.check(context);
    check_result(clang_tidy, true, 1, 0, 1);
  }

  SECTION("The commit contains one modified file and delete an old file should only "
          "check the modified file") {
    repo.add_file("test1.cpp", "int n = 1;\n");
    repo.add_file("test2.cpp", "int n = 1;\n");
    repo.add_file("test3.cpp", "int n = 1;\n");
    auto target = repo.commit_changes();

    repo.remove_file("test1.cpp");
    repo.rewrite_file("test3.cpp", "const int m = 1;\n");
    auto source = repo.commit_changes();

    auto context = create_runtime_context(target, source);
    clang_tidy.check(context);
    check_result(clang_tidy, true, 1, 0, 0);
  }
}

TEST_CASE("Test clang-tidy could correctly check basic error",
          "[cpp-lint-action][tool][clang_tidy][general_version]") {
  SKIP_IF_NO_CLANG_TIDY
  auto clang_tidy = create_clang_tidy();

  auto repo = repo_t{};
  repo.commit_clang_tidy();

  SECTION("Insert error lines shouldn't pass clang-tidy check") {
    repo.add_file("file.cpp", "int n = 0;\n");
    auto target_id = repo.commit_changes();

    repo.rewrite_file("file.cpp", "int n = 0;\nint m = 2;");
    auto source_id = repo.commit_changes();

    auto context = create_runtime_context(target_id, source_id);
    clang_tidy.check(context);
    check_result(clang_tidy, false, 0, 1, 0);
  }

  SECTION("Insert correct lines should pass clang-tidy check") {
    repo.add_file("file.cpp", "const int n = 0;\n");
    auto target_id = repo.commit_changes();

    repo.rewrite_file("file.cpp", "const int n = 0;\nconst int m = 1;\n");
    auto source_id = repo.commit_changes();

    auto context = create_runtime_context(target_id, source_id);
    clang_tidy.check(context);
    check_result(clang_tidy, true, 1, 0, 0);
  }

  SECTION("Delete all error lines will pass clang-tidy check") {
    auto old_content  = std::string{};
    old_content      += "const int n = 0;\n";
    old_content      += "const int m = 0;\n";
    old_content      += "int p;\n";
    repo.add_file("file.cpp", old_content);
    auto target_id = repo.commit_changes();

    auto new_content  = std::string{};
    new_content      += "const int n = 0;\n";
    new_content      += "const int m = 0;\n";
    repo.rewrite_file("file.cpp", new_content);
    auto source_id = repo.commit_changes();

    auto context = create_runtime_context(target_id, source_id);
    clang_tidy.check(context);
    check_result(clang_tidy, true, 1, 0, 0);
  }

  SECTION("Delete only part of error lines shouldn't pass clang-tidy check") {
    auto old_content  = std::string{};
    old_content      += "const int n = 0;\n";
    old_content      += "int m;\n";
    old_content      += "int p;\n";
    repo.add_file("file.cpp", old_content);
    auto target_id = repo.commit_changes();

    auto new_content  = std::string{};
    new_content      += "const int n = 0;\n";
    new_content      += "int m;\n";
    repo.rewrite_file("file.cpp", new_content);
    auto source_id = repo.commit_changes();

    auto context = create_runtime_context(target_id, source_id);
    clang_tidy.check(context);
    check_result(clang_tidy, false, 0, 1, 0);
  }

  SECTION("Rewrite error lines to error lines shouldn't pass "
          "clang-tidy check") {
    auto old_content  = std::string{};
    old_content      += "const int n = 0;\n";
    old_content      += "int m;\n";
    repo.add_file("file.cpp", old_content);
    auto target_id = repo.commit_changes();

    auto new_content  = std::string{};
    new_content      += "const int n = 0;\n";
    new_content      += "int p;\n";
    repo.rewrite_file("file.cpp", new_content);
    auto source_id = repo.commit_changes();

    auto context = create_runtime_context(target_id, source_id);
    clang_tidy.check(context);
    check_result(clang_tidy, false, 0, 1, 0);
  }
  SECTION("Rewrite error lines to correct lines should pass "
          "clang-tidy check") {
    auto old_content  = std::string{};
    old_content      += "const int n = 0;\n";
    old_content      += "int m;\n";
    repo.add_file("file.cpp", old_content);
    auto target_id = repo.commit_changes();

    auto new_content  = std::string{};
    new_content      += "const int n = 0;\n";
    new_content      += "const int m = 0;\n";
    repo.rewrite_file("file.cpp", new_content);
    auto source_id = repo.commit_changes();

    auto context = create_runtime_context(target_id, source_id);
    clang_tidy.check(context);
    check_result(clang_tidy, true, 1, 0, 0);
  }
  SECTION("Rewrite correct lines to error lines shouldn't pass "
          "clang-tidy check") {
    auto old_content  = std::string{};
    old_content      += "const int n = 0;\n";
    old_content      += "const int m = 0;\n";
    repo.add_file("file.cpp", old_content);
    auto target_id = repo.commit_changes();

    auto new_content  = std::string{};
    new_content      += "const int n = 0;\n";
    new_content      += "int m;\n";
    repo.rewrite_file("file.cpp", new_content);
    auto source_id = repo.commit_changes();

    auto context = create_runtime_context(target_id, source_id);
    clang_tidy.check(context);
    check_result(clang_tidy, false, 0, 1, 0);
  }
  SECTION("Rewrite correct lines to correct lines should pass "
          "clang-tidy check") {
    auto old_content  = std::string{};
    old_content      += "const int n = 0;\n";
    old_content      += "const int m = 0;\n";
    repo.add_file("file.cpp", old_content);
    auto target_id = repo.commit_changes();

    auto new_content  = std::string{};
    new_content      += "const int n = 0;\n";
    new_content      += "const int p = 0;\n";
    repo.rewrite_file("file.cpp", new_content);
    auto source_id = repo.commit_changes();

    auto context = create_runtime_context(target_id, source_id);
    clang_tidy.check(context);
    check_result(clang_tidy, true, 1, 0, 0);
  }
}

TEST_CASE("Test reporter", "[cpp-lint-action][tool][clang_tidy][general_version]") {
  auto option = clang_tidy::option_t{};
  auto result = clang_tidy::result_t{};
}
