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
#include "tools/clang_format/clang_format.h"
#include "tools/clang_format/general/impl.h"
#include "tools/clang_format/general/reporter.h"
#include "tools/util.h"
#include "utils/shell.h"

#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <stdexcept>

using namespace lint;
using namespace lint::tool;

// TODO: typical clang-format tools.

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

  // Check whether local environment contains clang-format otherwise some checks
  // will be failed.
  bool has_clang_format() {
    auto [ec, std_out, std_err] = shell::which("clang-format");
    return ec == 0;
  }

  // Check whether local environment contains specific clang-format version
  // otherwise some checks will be failed.
  bool has_clang_format(std::string_view version) {
    try {
      find_clang_tool("clang-format", version);
      return true;
    } catch (std::runtime_error &err) {
      return false;
    }
  }

  auto create_then_register_tool_desc(const clang_format::creator &creator)
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

#define SKIP_IF_NO_CLANG_FORMAT                                               \
  if (!has_clang_format()) {                                                  \
    SKIP("Local environment doesn't have clang-format. So skip clang-format " \
         "unit tests.");                                                      \
  }

// NOLINTNEXTLINE
#define SKIP_IF_NOT_HAS_CLANG_FORMAT_VERSION(version)                        \
  if (!has_clang_format(version)) {                                          \
    SKIP("Local environment doesn't have required clang-format version. So " \
         "skip clang-format unit tests.");                                   \
  }

} // namespace

TEST_CASE("Test register and create clang-format option",
          "[cpp-lint-action][program_options][tool][clang_format][creator]") {
  SKIP_IF_NO_CLANG_FORMAT
  auto creator = std::make_unique<clang_format::creator>();
  auto desc    = create_then_register_tool_desc(*creator);

  SECTION("Explicitly enables clang-format should work") {
    auto opts = parse_opt(desc, "--target-revision=main", "--enable-clang-format=true");
    creator->create_option(opts);
    REQUIRE(creator->get_option().enabled);
  }

  SECTION("Explicitly disable clang-format should work") {
    auto opts = parse_opt(desc, "--target-revision=main", "--enable-clang-format=false");
    creator->create_option(opts);
    REQUIRE(creator->get_option().enabled == false);
  }

  SECTION("clang-format is defaultly enabled") {
    auto opts = parse_opt(desc, "--target-revision=main");
    creator->create_option(opts);
    REQUIRE(creator->get_option().enabled);
  }

  SECTION("Receive an invalid clang-format version should throw exception") {
    auto opts = parse_opt(desc, "--target-revision=main", "--clang-format-version=18.x.1");
    REQUIRE_THROWS(creator->create_option(opts));
  }

  SECTION("Receive an invalid clang-format binary should throw exception") {
    auto opts = parse_opt(desc,
                          "--target-revision=main",
                          "--clang-format-binary=/usr/bin/clang-format-invalid");
    REQUIRE_THROWS(creator->create_option(opts));
  }

  SECTION("Other options should be correctly created based on user input") {
    auto opts = parse_opt(
      desc,
      "--target-revision=main",
      "--enable-clang-format-fastly-exit=true",
      "--clang-format-file-iregex=*.cpp");
    creator->create_option(opts);
    auto option = creator->get_option();
    REQUIRE(option.enabled_fastly_exit == true);
    REQUIRE(option.file_filter_iregex == "*.cpp");
  }
}

TEST_CASE("Test clang-format should get full version even though user input a "
          "simplified version",
          "[cpp-lint-action][tool][clang_format][creator]") {
  SKIP_IF_NOT_HAS_CLANG_FORMAT_VERSION("18")
  auto creator = std::make_unique<clang_format::creator>();
  auto desc    = create_then_register_tool_desc(*creator);

  auto vars = parse_opt(desc, "--target-revision=main", "--clang-format-version=18");

  auto context = runtime_context{};
  program_options::fill_context(vars, context);
  auto clang_format = creator->create_tool(vars);
  auto version      = clang_format->version();
  auto parts        = ranges::views::split(version, '.') | ranges::to<std::vector<std::string>>();
  REQUIRE(parts.size() == 3);
  REQUIRE(parts[0] == "18");
  REQUIRE(ranges::all_of(parts[1], isdigit));
  REQUIRE(ranges::all_of(parts[2], isdigit));
}

TEST_CASE("Create tool of spefific version should work",
          "[cpp-lint-action][tool][clang_format][creator]") {
  SKIP_IF_NOT_HAS_CLANG_FORMAT_VERSION("18.1.3")
  auto creator = std::make_unique<clang_format::creator>();
  auto desc    = create_then_register_tool_desc(*creator);

  SECTION("version 18.1.3") {
    auto vars         = parse_opt(desc, "--target-revision=main", "--clang-format-version=18.1.3");
    auto clang_format = creator->create_tool(vars);
    auto context      = runtime_context{};
    program_options::fill_context(vars, context);
    REQUIRE(clang_format->version() == "18.1.3");
    REQUIRE(clang_format->name() == "clang-format");
  }
}

namespace {
  auto create_clang_format() -> clang_format::clang_format_general {
    auto option    = clang_format::option_t{};
    option.enabled = true;
    option.binary  = "/usr/bin/clang-format";
    return clang_format::clang_format_general{option};
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

TEST_CASE("Test clang-format could correctly handle file filter",
          "[cpp-lint-action][tool][clang_format][general_version]") {
  SKIP_IF_NO_CLANG_FORMAT

  auto clang_format                      = create_clang_format();
  clang_format.option.file_filter_iregex = ".*.test";

  // Create git repository whichi to be checked.
  auto repo = repo_t{};
  repo.commit_clang_format();
  repo.add_file("file.test", "int   n = 0;");
  auto target = repo.commit_changes();
  repo.rewrite_file("file.test", "int n = 0;");
  auto source = repo.commit_changes();

  auto context = create_runtime_context(target, source);

  // Check
  clang_format.check(context);
  check_result(clang_format, true, 1, 0, 0);
}

TEST_CASE("Test clang-format could correctly handle various file level cases",
          "[cpp-lint-action][tool][clang_format][general_version]") {
  SKIP_IF_NO_CLANG_FORMAT
  auto clang_format = create_clang_format();

  // Create git repository whichi to be checked.
  auto repo = repo_t{};
  repo.commit_clang_format();

  SECTION("DELETED files shouldn't be checked") {
    repo.add_file("test1.cpp", "int n =       1;\n");
    repo.add_file("test2.cpp", "int n    = 1;\n");
    repo.add_file("test3.cpp", "int n = 1;\n");
    auto target = repo.commit_changes();

    repo.remove_file("test1.cpp");
    repo.remove_file("test2.cpp");
    auto source = repo.commit_changes();

    auto context = create_runtime_context(target, source);
    clang_format.check(context);
    check_result(clang_format, true, 0, 0, 0);
  }

  SECTION("NEW added files should be checked") {
    repo.add_file("test1.cpp", "int n = 1;\n");
    repo.add_file("test2.cpp", "int n = 1;\n");
    auto target = repo.commit_changes();

    repo.add_file("test3.cpp", "int n    = 1;\n");
    repo.add_file("test4.cpp", "int n = 1;\n");
    auto source = repo.commit_changes();

    auto context = create_runtime_context(target, source);
    clang_format.check(context);
    check_result(clang_format, false, 1, 1, 0);
  }

  SECTION("MODIFIED files should be checked") {
    repo.add_file("test1.cpp", "int n = 1;\n");
    repo.add_file("test2.cpp", "int n = 1;\n");
    auto target = repo.commit_changes();

    repo.rewrite_file("test1.cpp", "int n    = 1;\n");
    auto source = repo.commit_changes();

    auto context = create_runtime_context(target, source);
    clang_format.check(context);
    check_result(clang_format, false, 0, 1, 0);
  }

  SECTION("The commit contains only delete file should check nothing") {
    repo.add_file("test1.cpp", "int n =       1;\n");
    repo.add_file("test2.cpp", "int n    = 1;\n");
    repo.add_file("test3.cpp", "int n = 1;\n");
    auto target = repo.commit_changes();

    repo.remove_file("test1.cpp");
    auto source = repo.commit_changes();

    auto context = create_runtime_context(target, source);
    clang_format.check(context);
    check_result(clang_format, true, 0, 0, 0);
  }

  SECTION("The commit contains one modified file and insert a new non-cpp file should check "
          "only one files") {
    repo.add_file("test1.cpp", "int n =       1;\n");
    repo.add_file("test2.cpp", "int n    = 1;\n");
    repo.add_file("test3.cpp", "int n = 1;\n");
    auto target = repo.commit_changes();

    repo.rewrite_file("test1.cpp", "int n = 1");
    repo.add_file("test4.unknown", "int n = 1");
    auto source = repo.commit_changes();

    auto context = create_runtime_context(target, source);
    clang_format.check(context);
    check_result(clang_format, true, 1, 0, 1);
  }

  SECTION("The commit contains one modified file and delete an old file should only "
          "check the modified file") {
    repo.add_file("test1.cpp", "int n     = 1;\n");
    repo.add_file("test2.cpp", "int n = 1;\n");
    repo.add_file("test3.cpp", "int n = 1;\n");
    auto target = repo.commit_changes();

    repo.remove_file("test1.cpp");
    repo.rewrite_file("test3.cpp", "int m = 1;\n");
    auto source = repo.commit_changes();

    auto context = create_runtime_context(target, source);
    clang_format.check(context);
    check_result(clang_format, true, 1, 0, 0);
  }
}

TEST_CASE("Test clang-format could correctly check basic unformatted error",
          "[cpp-lint-action][tool][clang_format][general_version]") {
  SKIP_IF_NO_CLANG_FORMAT
  auto clang_format = create_clang_format();

  auto repo = repo_t{};
  repo.commit_clang_format();

  SECTION("Insert unformatted lines shouldn't pass clang-format check") {
    repo.add_file("file.cpp", "int n = 0;");
    auto target_id                = repo.commit_changes();
    const auto *const unformatted = R"(int n   = 0;
      int             m = 1;
    )";

    repo.rewrite_file("file.cpp", unformatted);
    auto source_id = repo.commit_changes();

    auto context = create_runtime_context(target_id, source_id);
    clang_format.check(context);
    check_result(clang_format, false, 0, 1, 0);
  }

  SECTION("Insert formatted lines should pass clang-format check") {
    repo.add_file("file.cpp", "int n = 0;");
    auto target_id = repo.commit_changes();

    repo.rewrite_file("file.cpp", "int n = 0;\nint m = 1;\n");
    auto source_id = repo.commit_changes();

    auto context = create_runtime_context(target_id, source_id);
    clang_format.check(context);
    check_result(clang_format, true, 1, 0, 0);
  }

  SECTION("Delete all unformatted lines will pass clang-format check") {
    repo.add_file("file.cpp", R"(int n = 0;
    int m     = 1;
    )");
    auto target_id = repo.commit_changes();

    repo.rewrite_file("file.cpp", "int n = 0;\n");
    auto source_id = repo.commit_changes();

    auto context = create_runtime_context(target_id, source_id);
    clang_format.check(context);
    check_result(clang_format, true, 1, 0, 0);
  }

  SECTION("Delete only part of unformatted lines shouldn't pass clang-format check") {
    auto old_content  = std::string{};
    old_content      += "int n = 0;\n";
    old_content      += "int m     = 0;\n";
    old_content      += "int p     = 0;\n";
    repo.add_file("file.cpp", old_content);
    auto target_id = repo.commit_changes();

    auto new_content  = std::string{};
    new_content      += "int n = 0;\n";
    new_content      += "int m     = 0;\n";
    repo.rewrite_file("file.cpp", new_content);
    auto source_id = repo.commit_changes();

    auto context = create_runtime_context(target_id, source_id);
    clang_format.check(context);
    check_result(clang_format, false, 0, 1, 0);
  }

  SECTION("Rewrite unformatted lines to unformatted lines shouldn't pass "
          "clang-format check") {
    auto old_content  = std::string{};
    old_content      += "int n = 0;\n";
    old_content      += "int m     = 0;\n";
    repo.add_file("file.cpp", old_content);
    auto target_id = repo.commit_changes();

    auto new_content  = std::string{};
    new_content      += "int n = 0;\n";
    new_content      += "int p     = 0;\n";
    repo.rewrite_file("file.cpp", new_content);
    auto source_id = repo.commit_changes();

    auto context = create_runtime_context(target_id, source_id);
    clang_format.check(context);
    check_result(clang_format, false, 0, 1, 0);
  }
  SECTION("Rewrite unformatted lines to formatted lines should pass "
          "clang-format check") {
    auto old_content  = std::string{};
    old_content      += "int n = 0;\n";
    old_content      += "int m     = 0;\n";
    repo.add_file("file.cpp", old_content);
    auto target_id = repo.commit_changes();

    auto new_content  = std::string{};
    new_content      += "int n = 0;\n";
    new_content      += "int m = 0;\n";
    repo.rewrite_file("file.cpp", new_content);
    auto source_id = repo.commit_changes();

    auto context = create_runtime_context(target_id, source_id);
    clang_format.check(context);
    check_result(clang_format, true, 1, 0, 0);
  }
  SECTION("Rewrite formatted lines to unformatted lines shouldn't pass "
          "clang-format check") {
    auto old_content  = std::string{};
    old_content      += "int n = 0;\n";
    old_content      += "int m = 0;\n";
    repo.add_file("file.cpp", old_content);
    auto target_id = repo.commit_changes();

    auto new_content  = std::string{};
    new_content      += "int n = 0;\n";
    new_content      += "int m   = 0;\n";
    repo.rewrite_file("file.cpp", new_content);
    auto source_id = repo.commit_changes();

    auto context = create_runtime_context(target_id, source_id);
    clang_format.check(context);
    check_result(clang_format, false, 0, 1, 0);
  }
  SECTION("Rewrite formatted lines to formatted lines should pass "
          "clang-format check") {
    auto old_content  = std::string{};
    old_content      += "int n = 0;\n";
    old_content      += "int m = 0;\n";
    repo.add_file("file.cpp", old_content);
    auto target_id = repo.commit_changes();

    auto new_content  = std::string{};
    new_content      += "int n = 0;\n";
    new_content      += "int p = 0;\n";
    repo.rewrite_file("file.cpp", new_content);
    auto source_id = repo.commit_changes();

    auto context = create_runtime_context(target_id, source_id);
    clang_format.check(context);
    check_result(clang_format, true, 1, 0, 0);
  }
}

TEST_CASE("Test parse replacements", "[cpp-lint-action][tool][clang_format][general_version]") {
  SKIP_IF_NO_CLANG_FORMAT

  SECTION("Empty replacements") {
  }
  SECTION("One replacement") {
  }
  SECTION("Two replacements") {
  }
}

// TEST_CASE("Test reporter", "[cpp-lint-action][tool][clang_format][general_version]") {
//   auto option   = clang_format::option_t{};
//   auto result   = clang_format::result_t{};
//   auto reporter = clang_format::reporter_t{option, result};
//   auto context  = runtime_context{};
//
//   reporter.result.fails = {
//     {"file1.cpp", {}},
//     {"file2.cpp", {}}
//   };
//
//   SECTION("Test make issue comment") {
//     auto res       = reporter.make_issue_comment(context);
//     auto expected  = std::string{};
//     expected      += "- file1.cpp\n";
//     expected      += "- file2.cpp\n";
//     REQUIRE(res == expected);
//   }
// }
