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

#include "github/common.h"
#include "test_common.h"
#include "utils/env_manager.h"

#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <utility>

#include <catch2/catch_test_macros.hpp>

using namespace lint;

namespace {
  auto write_event_payload(const std::string &name, const std::string &content)
    -> std::filesystem::path {
    auto path = std::filesystem::temp_directory_path() / name;
    auto file = std::ofstream{path};
    REQUIRE(file.is_open());
    file << content;
    return path;
  }

  void set_debug_github_env(std::unordered_map<std::string, std::string> extra = {}) {
    auto values = std::unordered_map<std::string, std::string>{
      {github::debug_cpp_lint_action, "true"},
      {"DEBUG_GITHUB_REPOSITORY", "owner/repo"},
      {"DEBUG_GITHUB_TOKEN", "github-token"},
      {"DEBUG_GITHUB_EVENT_NAME", github::github_event_pull_request},
      {"DEBUG_GITHUB_BASE_REF", "main"},
      {"DEBUG_GITHUB_HEAD_REF", "feature"},
      {"DEBUG_GITHUB_REF", "refs/pull/9/merge"},
      {"DEBUG_GITHUB_SHA", "1234567890abcdef"},
      {"DEBUG_GITHUB_REF_TYPE", "branch"},
      {"DEBUG_GITHUB_WORKSPACE", "/tmp/repo"},
      {"DEBUG_GITHUB_EVENT_PATH", ""},
    };
    for (auto &[key, value]: extra) {
      values[key] = std::move(value);
    }
    env::set_cache(std::move(values));
  }
} // namespace

TEST_CASE("GitHub environment reads pull request number from event payload first",
          "[cpp-lint-action][github]") {
  auto event_path = write_event_payload("cpp_lint_action_pull_request_event.json", R"({"number":42})");
  auto guard      = scope_guard{[&] {
    std::filesystem::remove(event_path);
    env::set_cache({});
  }};

  set_debug_github_env({{"DEBUG_GITHUB_EVENT_PATH", event_path.string()},
                        {"DEBUG_GITHUB_REF", "refs/pull/9/merge"}});

  auto ctx = runtime_context{};
  auto env = github::read_env();
  github::fill_context(env, ctx);

  REQUIRE(ctx.event_name == github::github_event_pull_request);
  REQUIRE(ctx.pr_number == 42);
}

TEST_CASE("GitHub environment falls back to pull request number from ref",
          "[cpp-lint-action][github]") {
  auto guard = scope_guard{[] { env::set_cache({}); }};
  set_debug_github_env({{"DEBUG_GITHUB_EVENT_PATH", ""}, {"DEBUG_GITHUB_REF", "refs/pull/77/merge"}});

  auto ctx = runtime_context{};
  auto env = github::read_env();
  github::fill_context(env, ctx);

  REQUIRE(ctx.pr_number == 77);
}

TEST_CASE("GitHub reporting is skipped for non pull request events without requiring a token",
          "[cpp-lint-action][github]") {
  auto guard = scope_guard{[] { env::set_cache({}); }};
  set_debug_github_env({{"DEBUG_GITHUB_TOKEN", ""},
                        {"DEBUG_GITHUB_EVENT_NAME", github::github_event_push},
                        {"DEBUG_GITHUB_REF", "refs/heads/main"}});

  auto ctx                        = runtime_context{};
  ctx.enable_comment_on_issue     = true;
  ctx.enable_pull_request_review  = true;
  auto env                        = github::read_env();
  github::fill_context(env, ctx);

  REQUIRE_NOTHROW(github::prepare_reporting_context(ctx));
  REQUIRE(ctx.enable_comment_on_issue == false);
  REQUIRE(ctx.enable_pull_request_review == false);
  REQUIRE(ctx.report_notes.size() == 1);
}

TEST_CASE("GitHub reporting requires token and pull request number for pull request events",
          "[cpp-lint-action][github]") {
  auto guard = scope_guard{[] { env::set_cache({}); }};
  set_debug_github_env({{"DEBUG_GITHUB_TOKEN", ""},
                        {"DEBUG_GITHUB_EVENT_PATH", ""},
                        {"DEBUG_GITHUB_REF", "refs/heads/feature"}});

  auto ctx                       = runtime_context{};
  ctx.enable_comment_on_issue    = true;
  auto env                       = github::read_env();
  github::fill_context(env, ctx);

  REQUIRE_THROWS(github::prepare_reporting_context(ctx));
}
