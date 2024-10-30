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

#include "utils/env_manager.h"
#include "utils/error.h"

namespace lint::github {
  namespace {
    // PR merge branch refs/pull/PULL_REQUEST_NUMBER/merge
    auto parse_pr_number(const std::string &ref_name) -> std::int32_t {
      spdlog::trace("Enter parse_pr_number()");

      auto parts = ranges::views::split(ref_name, '/') | ranges::to<std::vector<std::string>>();
      throw_if(parts.size() != 4, fmt::format("ref_name format error: {}", ref_name));
      auto res = std::stoi(parts[2]);
      spdlog::trace("parse pr number uses: {} and got pull-request number: {}", ref_name, res);
      return res;
    }

    void check_env(const github_env &env) {
      spdlog::debug("Start to check github environment variables");

      throw_if(env.repository.empty(), "empty git repository, check env: GITHUB_REPOSITORY");
      throw_if(env.token.empty(), "empty token, check env: GITHUB_TOKEN");
      throw_if(env.event_name.empty(), "empty git event, check env: GITHUB_EVENT_NAME");
      throw_if(env.github_ref.empty(), "empty github ref, check env: GITHUB_REF");
      throw_if(env.github_sha.empty(), "empty github sha, check env: GITHUB_SHA");
      throw_if(env.github_ref_type.empty(), "empty git ref type, check env: GITHUB_REF_TYPE");
      throw_if(env.workspace.empty(),
               "empty git repository workspace, check env: GITHUB_WORKSPACE");
    }

    void print_env(const github_env &env) {
      spdlog::debug("Github Envionment Variables:");
      spdlog::debug("--------------------------------------------------");
      spdlog::debug("git repository:{}", env.repository);
      spdlog::debug("git token:{}", env.token);
      spdlog::debug("git event name:{}", env.event_name);
      spdlog::debug("git base ref:{}", env.base_ref);
      spdlog::debug("git head ref:{}", env.head_ref);
      spdlog::debug("git ref:{}", env.github_ref);
      spdlog::debug("git sha:{}", env.github_sha);
      spdlog::debug("git ref type:{}", env.github_ref_type);
      spdlog::debug("git workspace:{}", env.workspace);
      spdlog::debug("");
    }

  } // namespace

  auto read_env() -> github_env {
    spdlog::trace("Enter read_env()");

    auto env = github_env{};

    if (env::get(debug_cpp_lint_action).empty()) {
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
      print_env(env);
      check_env(env);
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
    }

    return env;
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
    bool needs_parse_pr_number = ranges::contains(github_pull_request_events, ctx.event_name)
                              && (ctx.enable_comment_on_issue || ctx.enable_pull_request_review);
    if (needs_parse_pr_number) {
      ctx.pr_number = parse_pr_number(env.github_ref);
    }
  }

} // namespace lint::github
