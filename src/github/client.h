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
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <thread>
#include <vector>

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "common.h"
#include "context.h"
#include "utils/common.h"
#include "utils/error.h"

namespace lint::github {
  using namespace std::string_literals;

  class client {
    static constexpr auto github_accept_json = "application/vnd.github+json";
    static constexpr auto http_content_json  = "application/json";
    static constexpr auto github_user_agent  = "cpp-lint-action";
    static constexpr auto github_api_version = "2022-11-28";
    static constexpr auto max_retry_attempts = 3;

    static auto make_headers(const runtime_context &ctx) -> httplib::Headers {
      return {
        {"Accept", github_accept_json},
        {"Authorization", fmt::format("Bearer {}", ctx.token)},
        {"User-Agent", github_user_agent},
        {"X-GitHub-Api-Version", github_api_version},
      };
    }

    void configure_client() {
      http_client_.set_connection_timeout(10);
      http_client_.set_read_timeout(30);
      http_client_.set_write_timeout(30);
    }

    static auto rate_limit_info(const httplib::Response &response) -> std::string {
      auto remaining = response.get_header_value("X-RateLimit-Remaining");
      auto reset     = response.get_header_value("X-RateLimit-Reset");
      auto retry_after = response.get_header_value("Retry-After");

      auto parts = std::vector<std::string>{};
      if (!remaining.empty()) {
        parts.emplace_back(fmt::format("remaining={}", remaining));
      }
      if (!reset.empty()) {
        parts.emplace_back(fmt::format("reset={}", reset));
      }
      if (!retry_after.empty()) {
        parts.emplace_back(fmt::format("retry-after={}", retry_after));
      }
      if (parts.empty()) {
        return "";
      }
      return fmt::format(", rate-limit: {}", concat(parts, ','));
    }

    static auto is_retryable(const httplib::Result &response) -> bool {
      if (!response) {
        return true;
      }
      return response->status == 429 || response->status / 100 == 5;
    }

    template <class Fn>
    static auto request_with_retry(Fn &&fn) -> httplib::Result {
      auto response = fn();
      for (auto attempt = 1; attempt < max_retry_attempts && is_retryable(response); ++attempt) {
        std::this_thread::sleep_for(std::chrono::milliseconds{10 * (1 << (attempt - 1))});
        response = fn();
      }
      return response;
    }

  public:
    client()
      : http_client_{github_api} {
      configure_client();
    }

    explicit client(const std::string &api_url)
      : http_client_{api_url} {
      configure_client();
    }

    static void check_http_response(const httplib::Result &response) {
      throw_unless(static_cast<bool>(response), "github http request failed");

      auto code          = response->status / 100;
      const auto &reason = response->reason;
      throw_unless(code == 1 || code == 2,
                   fmt::format("http response error: http status code: {}, reason: {}{}, body: {}",
                               response->status,
                               reason,
                               rate_limit_info(*response),
                               response->body));
    }

    static auto is_our_comment(const nlohmann::json &comment) -> bool {
      if (!comment.contains("/body"_json_pointer)) {
        return false;
      }

      auto body = comment["body"].get<std::string>();
      return body.starts_with(github_comment_identifier);
    }

    void get_issue_comment_id(const runtime_context &ctx) {
      spdlog::debug("Start to get issue comment id for pull request: {}.", ctx.pr_number);
      assert(ranges::contains(github_pull_request_events, ctx.event_name));

      auto path    = fmt::format("/repos/{}/issues/{}/comments", ctx.repo_pair, ctx.pr_number);
      auto headers = make_headers(ctx);
      spdlog::debug("Http request path: {}", path);

      auto best_comment_id = std::int64_t{-1};
      auto matched_comments = std::size_t{0};
      constexpr auto per_page = 100;
      for (auto page = 1;; ++page) {
        auto page_path = fmt::format("{}?per_page={}&page={}", path, per_page, page);
        auto response = request_with_retry([&] { return http_client_.Get(page_path, headers); });

        check_http_response(response);
        spdlog::trace("Get github response body: {}", response->body);

        // data type check
        auto comments = nlohmann::json::parse(response->body);
        if (comments.is_null()) {
          spdlog::debug("The pull request number {} doesn't have any comments yet", ctx.pr_number);
          break;
        }
        throw_unless(comments.is_array(), "issue comments are not an array");
        if (comments.empty()) {
          spdlog::debug("The pull request number {} doesn't have any comments yet", ctx.pr_number);
          break;
        }

        for (const auto &comment: comments) {
          if (!is_our_comment(comment)) {
            continue;
          }
          auto id = std::int64_t{};
          comment["id"].get_to(id);
          best_comment_id = std::max(best_comment_id, id);
          ++matched_comments;
        }

        if (comments.size() < per_page) {
          break;
        }
      }

      if (best_comment_id == -1) {
        spdlog::debug("The cpp-lint doesn't comments on pull request number {} yet", ctx.pr_number);
        return;
      }

      comment_id_ = best_comment_id;
      if (matched_comments > 1) {
        spdlog::debug("Found {} cpp-lint-action comments; updating newest id {}",
                      matched_comments,
                      comment_id_);
      }
      spdlog::debug("Successfully got comment id {} of pr {}", comment_id_, ctx.pr_number);
    }

    void add_issue_comment(const runtime_context &ctx, const std::string &body) {
      spdlog::debug("Start to add issue comment for pr {}", ctx.pr_number);

      const auto path = fmt::format("/repos/{}/issues/{}/comments", ctx.repo_pair, ctx.pr_number);
      const auto headers = make_headers(ctx);
      spdlog::debug("Http request path: {}", path);

      auto json_body    = nlohmann::json{};
      json_body["body"] = github_comment_identifier + body;
      spdlog::trace("Http request body:\n{}", json_body.dump());

      auto response = request_with_retry(
        [&] { return http_client_.Post(path, headers, json_body.dump(), http_content_json); });
      check_http_response(response);
      spdlog::trace("Get github response body: {}", response->body);

      auto comment = nlohmann::json::parse(response->body);
      throw_unless(comment.is_object(), "comment isn't object");

      comment["id"].get_to(comment_id_);
      spdlog::info("Successfully add new comment for pull-request {}, the new "
                   "added comment id is {}",
                   ctx.pr_number,
                   comment_id_);
    }

    void update_issue_comment(const runtime_context &ctx, const std::string &body) {
      throw_if(comment_id_ == -1, "the client doesn't have comment_id yet");
      throw_if(ctx.pr_number == -1, "the context doesn't have pr-number yet");
      spdlog::debug("Start to update issue comment");

      const auto path    = fmt::format("/repos/{}/issues/comments/{}", ctx.repo_pair, comment_id_);
      const auto headers = make_headers(ctx);
      spdlog::debug("Http request path: {}", path);

      auto json_body    = nlohmann::json{};
      json_body["body"] = github_comment_identifier + body;
      spdlog::trace("Http request body:\n{}", json_body.dump());

      auto response = request_with_retry(
        [&] { return http_client_.Patch(path, headers, json_body.dump(), http_content_json); });
      check_http_response(response);
      spdlog::trace("Get github response body: {}", response->body);
      spdlog::info("Successfully updated comment {} of pr {}", comment_id_, ctx.pr_number);
    }

    void add_or_update_issue_comment(const runtime_context &ctx, const std::string &body) {
      if (comment_id_ == -1) {
        add_issue_comment(ctx, body);
      } else {
        update_issue_comment(ctx, body);
      }
    }

    void post_pull_request_review(const runtime_context &ctx, const std::string &body) {
      spdlog::debug("Start to post pull request review for pr number {}", ctx.pr_number);

      const auto path    = fmt::format("/repos/{}/pulls/{}/reviews", ctx.repo_pair, ctx.pr_number);
      const auto headers = make_headers(ctx);
      spdlog::debug("Http request path: {}", path);
      spdlog::trace("Http request body:\n{}", body);

      auto response = request_with_retry(
        [&] { return http_client_.Post(path, headers, body, http_content_json); });
      check_http_response(response);
      spdlog::trace("Get github response body: {}", response->body);

      spdlog::info("Successfully post pull_request_review for pull-request {}", ctx.pr_number);
    }

  private:
    std::int64_t comment_id_ = -1;
    httplib::Client http_client_;
  };
} // namespace lint::github
