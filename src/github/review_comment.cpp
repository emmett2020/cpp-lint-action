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
#include "review_comment.h"

namespace lint::github {
  using namespace std::string_view_literals;

  constexpr auto review_event_comment         = "COMMENT"sv;
  constexpr auto review_event_request_changes = "REQUEST_CHANGES"sv;

  auto make_review_str(const review_comments &comments) -> std::string {
    auto res        = nlohmann::json{};
    res["body"]     = "cpp-lint-action suggestion";
    res["event"]    = review_event_comment;
    res["comments"] = comments;
    return res.dump();
  }

} // namespace lint::github
