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

#include <algorithm>
#include <utility>

#include <git2/diff.h>
#include <spdlog/spdlog.h>

#include "context.h"
#include "github/common.h"
#include "tools/base_reporter.h"
#include "tools/clang_format/general/option.h"
#include "tools/clang_format/general/result.h"
#include "utils/env_manager.h"
#include "utils/error.h"

namespace lint::tool::clang_format {
  using namespace std::string_view_literals;

  struct reporter_t : reporter_base {
    reporter_t(option_t opt, result_t res)
      : option(std::move(opt))
      , result(std::move(res)) {
    }

    auto get_brief_result() -> std::tuple<bool, std::size_t, std::size_t, std::size_t> override {
      return {result.final_passed,
              result.passes.size(),
              result.fails.size(),
              result.ignored.size()};
    }

    auto get_detail_result([[maybe_unused]] const runtime_context &context)
      -> std::string override {
      auto content = ""s;
      auto files = std::vector<std::string>{};
      for (const auto &[name, failed]: result.fails) {
        files.push_back(name);
      }
      std::sort(files.begin(), files.end());
      for (const auto &name: files) {
        auto one  = fmt::format("- {}\n", name);
        content  += one;
      }
      return content;
    }

    auto get_failed_commands() -> std::vector<std::string> override {
      return result.failed_commands;
    }

    auto make_review_comment(const runtime_context &context) -> github::review_comments override {
      auto comments = github::review_comments{};
      auto files = std::vector<std::string>{};
      for (const auto &[file, per_file_result]: result.fails) {
        files.push_back(file);
      }
      std::sort(files.begin(), files.end());

      for (const auto &file: files) {
        if (!context.patches.contains(file)) {
          continue;
        }
        const auto &per_file_result = result.fails.at(file);
        const auto &patch           = context.patches.at(file);
        const auto num_hunk         = git::patch::num_hunks(*patch);

        auto rows = std::vector<std::size_t>{};
        for (const auto &[row, replacements]: per_file_result.replacements) {
          rows.push_back(row);
        }
        std::sort(rows.begin(), rows.end());

        for (const auto row: rows) {
          const auto row_number = static_cast<int>(row);
          auto pos = std::size_t{0};
          for (int hunk_idx = 0; hunk_idx < num_hunk; ++hunk_idx) {
            auto [hunk, num_lines] = git::patch::get_hunk(*patch, hunk_idx);
            if (!git::hunk::is_row_in_hunk(hunk, row_number)) {
              pos += num_lines;
              continue;
            }

            auto comment     = github::review_comment{};
            comment.path     = file;
            comment.position = pos + static_cast<std::size_t>(row_number - hunk.new_start) + 1;
            comment.body     = "clang-format would reformat this line.";
            comments.emplace_back(std::move(comment));
            break;
          }
        }
      }
      return comments;
    }

    void write_to_action_output([[maybe_unused]] const runtime_context &ctx) override {
      auto output = env::get(github::github_output);
      auto file   = std::fstream{output, std::ios::app};
      throw_unless(file.is_open(), "error to open output file to write");
      file << fmt::format("clang_format_failed_number={}\n", result.fails.size());
    }

    auto tool_name() -> std::string override {
      auto parts = ranges::views::split(option.binary, '/')
                 | ranges::to<std::vector<std::string>>();
      return parts.back();
    }

    option_t option;
    result_t result;
  };

} // namespace lint::tool::clang_format
