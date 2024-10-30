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
#pragma once

#include <cstdint>
#include <git2/repository.h>
#include <string>
#include <unordered_map>

#include "utils/git_utils.h"

namespace lint {
  /// The runtime context for all tools.
  struct runtime_context {
    // Theses will be filled by [ program_options::fill_context() ]
    bool enable_step_summary        = false;
    bool enable_comment_on_issue    = false;
    bool enable_pull_request_review = false;
    bool enable_action_output       = false;
    bool disable_errors             = false;

    // Theses will be filled by [ github::fill_context() ]
    std::string repo_path;
    std::string repo_pair;
    std::string token;
    std::string event_name;
    std::string target;
    std::string source;
    std::int32_t pr_number = -1;

    // Theses will be filled by [ fill_git_info ]
    git::repo_ptr repo{nullptr, ::git_repository_free};
    git::commit_ptr target_commit{nullptr, ::git_commit_free};
    git::commit_ptr source_commit{nullptr, ::git_commit_free};

    // The diff patches of source revision to target revision.
    std::unordered_map<std::string, git::patch_ptr> patches;
    std::unordered_map<std::string, git_diff_delta> deltas;
    std::vector<std::string> changed_files;
  };

  void fill_git_info(runtime_context &context);

  void print_context(const runtime_context &ctx);
} // namespace lint
