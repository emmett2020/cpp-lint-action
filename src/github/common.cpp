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
#include "common.h"

#include <exception>
#include <fstream>
#include <optional>
#include <vector>

#include <boost/algorithm/string/case_conv.hpp>

#include "utils/env_manager.h"
#include "utils/error.h"

namespace lint::github {
  namespace {
    auto is_debug_enabled() -> bool {
      auto value = boost::algorithm::to_lower_copy(env::get(debug_cpp_lint_action));
      return value == "1" || value == "true" || value == "yes";
    }

    // PR merge branch refs/pull/PULL_REQUEST_NUMBER/merge
    auto parse_pr_number_from_ref(const std::string &ref_name) -> std::optional<std::int32_t> {
      spdlog::trace("Enter parse_pr_number_from_ref()");

      auto parts = ranges::views::split(ref_name, '/') | ranges::to<std::vector<std::string>>();
      if (parts.size() != 4) {
        return std::nullopt;
      }
      auto res = std::int32_t{};
      try {
        res = std::stoi(parts[2]);
      } catch (const std::exception &) {
        return std::nullopt;
      }
      spdlog::trace("parse pr number uses: {} and got pull-request number: {}", ref_name, res);
      return res;
    }

    auto parse_pr_number_from_event_path(const std::string &event_path)
      -> std::optional<std::int32_t> {
      spdlog::trace("Enter parse_pr_number_from_event_path()");
      if (event_path.empty()) {
        return std::nullopt;
      }

      auto file = std::ifstream{event_path};
      if (!file.is_open()) {
        spdlog::debug("Cannot open GitHub event payload: {}", event_path);
        return std::nullopt;
      }

      try {
        auto payload = nlohmann::json::parse(file);
        if (payload.contains("number") && payload["number"].is_number_integer()) {
          return payload["number"].get<std::int32_t>();
        }
      } catch (const std::exception &err) {
        spdlog::debug("Cannot parse GitHub event payload {}: {}", event_path, err.what());
      }
      return std::nullopt;
    }

    void check_base_env(const github_env &env) {
      spdlog::debug("Start to check github environment variables");

      throw_if(env.event_name.empty(), "empty git event, check env: GITHUB_EVENT_NAME");
      throw_if(env.github_sha.empty(), "empty github sha, check env: GITHUB_SHA");
      throw_if(env.workspace.empty(),
               "empty git repository workspace, check env: GITHUB_WORKSPACE");
    }

    void print_env(const github_env &env) {
      spdlog::debug("Github Envionment Variables:");
      spdlog::debug("--------------------------------------------------");
      spdlog::debug("git repository:{}", env.repository);
      spdlog::debug("git token:{}", env.token.empty() ? "" : "***");
      spdlog::debug("git event name:{}", env.event_name);
      spdlog::debug("git base ref:{}", env.base_ref);
      spdlog::debug("git head ref:{}", env.head_ref);
      spdlog::debug("git ref:{}", env.github_ref);
      spdlog::debug("git sha:{}", env.github_sha);
      spdlog::debug("git ref type:{}", env.github_ref_type);
      spdlog::debug("git workspace:{}", env.workspace);
      spdlog::debug("git event path:{}", env.event_path);
      spdlog::debug("");
    }

  } // namespace

  auto read_env() -> github_env {
    spdlog::trace("Enter read_env()");

    auto env = github_env{};

    if (!is_debug_enabled()) {
      // On real user's workflow
      env.repository      = env::get(github_repository);
      env.token           = env::get(github_token);
      env.event_name      = env::get(github_event_name);
      env.base_ref        = env::get(github_base_ref);
      env.head_ref        = env::get(github_head_ref);
      env.github_ref      = env::get(github_ref);
      env.github_sha      = env::get(github_sha);
      env.github_ref_type = env::get(github_ref_type);
      env.workspace       = env::get(github_workspace);
      env.event_path      = env::get(github_event_path);
    } else {
      // Only our test workflow.
      env.repository      = env::get("DEBUG_GITHUB_REPOSITORY");
      env.token           = env::get("DEBUG_GITHUB_TOKEN");
      env.event_name      = env::get("DEBUG_GITHUB_EVENT_NAME");
      env.base_ref        = env::get("DEBUG_GITHUB_BASE_REF");
      env.head_ref        = env::get("DEBUG_GITHUB_HEAD_REF");
      env.github_ref      = env::get("DEBUG_GITHUB_REF");
      env.github_sha      = env::get("DEBUG_GITHUB_SHA");
      env.github_ref_type = env::get("DEBUG_GITHUB_REF_TYPE");
      env.workspace       = env::get("DEBUG_GITHUB_WORKSPACE");
      env.event_path      = env::get("DEBUG_GITHUB_EVENT_PATH");
    }

    print_env(env);
    check_base_env(env);
    return env;
  }

  auto is_pull_request_event(std::string_view event_name) -> bool {
    return event_name == github_event_pull_request
        || event_name == github_event_pull_request_target;
  }

  void fill_context(const github_env &env, runtime_context &ctx) {
    spdlog::trace("Enter fill_context()");

    // Basic
    ctx.repo_path  = env.workspace;
    ctx.event_name = env.event_name;
    ctx.source     = env.github_sha;

    // For reporter
    ctx.token                  = env.token;
    ctx.repo_pair              = env.repository;
    if (is_pull_request_event(ctx.event_name)) {
      auto pr_number = parse_pr_number_from_event_path(env.event_path);
      if (!pr_number) {
        pr_number = parse_pr_number_from_ref(env.github_ref);
      }
      if (pr_number) {
        ctx.pr_number = *pr_number;
      }
    }
  }

  void prepare_reporting_context(runtime_context &ctx) {
    if (!is_pull_request_event(ctx.event_name)) {
      if (ctx.enable_comment_on_issue || ctx.enable_pull_request_review) {
        auto note =
          fmt::format("GitHub API reporting skipped because event '{}' is not a pull request",
                      ctx.event_name);
        spdlog::info("{}", note);
        ctx.report_notes.push_back(note);
      }
      ctx.enable_comment_on_issue    = false;
      ctx.enable_pull_request_review = false;
      return;
    }

    const auto needs_github_api = ctx.enable_comment_on_issue || ctx.enable_pull_request_review;
    if (!needs_github_api) {
      return;
    }

    throw_if(ctx.repo_pair.empty(),
             "empty git repository, check env: GITHUB_REPOSITORY");
    throw_if(ctx.token.empty(), "empty token, check env: GITHUB_TOKEN");
    throw_if(ctx.pr_number == -1,
             "cannot determine pull request number from GITHUB_EVENT_PATH or GITHUB_REF");
  }

} // namespace lint::github
