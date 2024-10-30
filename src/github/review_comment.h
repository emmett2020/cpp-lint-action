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

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace lint::github {
  /// A class represent Github pull request review comment.
  /// Details: https://docs.github.com/en/rest/pulls/reviews?apiVersion=2022-11-28#about-pull-request-reviews
  struct review_comment {
    std::string path;
    std::size_t position;
    std::string body;
    std::size_t line; // end_line
    std::string side;
    std::size_t start_line;
    std::string start_side;
  };

  NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(review_comment, path, position, body)

  using review_comments = std::vector<review_comment>;

  // Make review comments string by the given parameters.
  auto make_review_str(const review_comments &comments) -> std::string;
} // namespace lint::github
