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

#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

namespace lint {

  void fill_git_info(runtime_context &context) {
    spdlog::trace("Enter fill_git_info()");
    assert(context.repo == nullptr && "given context already has a repository");
    assert(context.target_commit == nullptr && "given context already has a target commit");
    assert(context.source_commit == nullptr && "given context already has a source commit");
    assert(context.patches.empty() && "given context already has patches");
    assert(context.deltas.empty() && "given context already has deltas");
    assert(context.changed_files.empty() && "given context already has changed files");

    assert(!context.repo_path.empty() && "repo_path of context is empty()");
    assert(!context.target.empty() && "target of context is empty()");
    assert(!context.source.empty() && "source of context is empty()");

    context.repo          = git::repo::open(context.repo_path);
    context.target_commit = git::revparse::commit(*context.repo, context.target);
    context.source_commit = git::revparse::commit(*context.repo, context.source);
    auto diff       = git::diff::get(*context.repo, *context.target_commit, *context.source_commit);
    context.patches = git::patch::create_from_diff(*diff);
    context.deltas  = git::diff::deltas(*diff);
    context.changed_files = git::patch::changed_files(context.patches);
  }

  void print_context(const runtime_context &ctx) {
    spdlog::debug("Runtime Context:");
    spdlog::debug("--------------------------------------------------");
    spdlog::debug("enable step summary: {}", ctx.enable_step_summary);
    spdlog::debug("enable comment on issue: {}", ctx.enable_comment_on_issue);
    spdlog::debug("enable pull request review: {}", ctx.enable_pull_request_review);
    spdlog::debug("enable action output: {}", ctx.enable_action_output);
    spdlog::debug("disable errors: {}", ctx.disable_errors);
    spdlog::debug("repository path: {}", ctx.repo_path);
    spdlog::debug("repository: {}", ctx.repo_pair);
    spdlog::debug("repository token: {}", ctx.token.empty() ? "" : "***");
    spdlog::debug("repository event_name: {}", ctx.event_name);
    spdlog::debug("repository target: {}", ctx.target);
    spdlog::debug("repository source: {}", ctx.source);
    spdlog::debug("repository pull-request number: {}", ctx.pr_number);
    spdlog::debug("repository target commit: {}", git::commit::id_str(*ctx.target_commit));
    spdlog::debug("repository source commit: {}", git::commit::id_str(*ctx.source_commit));
    spdlog::debug("{} changed files:", ctx.changed_files.size());
    for (const auto &file: ctx.changed_files) {
      spdlog::debug("{}", file);
    }
    spdlog::debug("");
  }
} // namespace lint
