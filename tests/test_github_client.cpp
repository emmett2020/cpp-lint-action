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

#include "github/client.h"
#include "github/common.h"
#include "github/review_comment.h"

#include <atomic>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#include <catch2/catch_test_macros.hpp>
#include <httplib.h>
#include <nlohmann/json.hpp>

using namespace lint;

namespace {
  constexpr auto comment_id = 5000000000LL;

  struct request_record {
    bool called = false;

    std::string accept;
    std::string authorization;
    std::string content_type;
    std::string user_agent;
    std::string api_version;
    std::string path;
    std::string body;
  };

  class server_runner {
  public:
    explicit server_runner(httplib::Server &server)
      : server_{server} {
      port_ = server_.bind_to_any_port("127.0.0.1");
      REQUIRE(port_ > 0);

      thread_ = std::thread{[this] { server_.listen_after_bind(); }};
      server_.wait_until_ready();
    }

    ~server_runner() {
      server_.stop();
      if (thread_.joinable()) {
        thread_.join();
      }
    }

    auto api_url() const -> std::string {
      return "http://127.0.0.1:" + std::to_string(port_);
    }

  private:
    httplib::Server &server_;
    int port_ = -1;
    std::thread thread_;
  };

  auto make_context() -> runtime_context {
    auto ctx       = runtime_context{};
    ctx.repo_pair = "owner/repo";
    ctx.token     = "github-token";
    ctx.event_name = github::github_event_pull_request;
    ctx.pr_number  = 42;
    return ctx;
  }

  void record_request(const httplib::Request &req,
                      request_record &record,
                      std::mutex &record_mutex) {
    auto lock             = std::lock_guard{record_mutex};
    record.called         = true;
    record.accept         = req.get_header_value("Accept");
    record.authorization  = req.get_header_value("Authorization");
    record.content_type   = req.get_header_value("Content-Type");
    record.user_agent     = req.get_header_value("User-Agent");
    record.api_version    = req.get_header_value("X-GitHub-Api-Version");
    record.path           = req.path;
    record.body           = req.body;
  }

  void check_github_json_request(const request_record &record, const std::string &body) {
    REQUIRE(record.called);
    REQUIRE(record.accept == "application/vnd.github+json");
    REQUIRE(record.authorization == "Bearer github-token");
    REQUIRE(record.user_agent == "cpp-lint-action");
    REQUIRE(record.api_version == "2022-11-28");
    REQUIRE(record.content_type.starts_with("application/json"));

    auto payload = nlohmann::json::parse(record.body);
    REQUIRE(payload["body"].get<std::string>()
            == std::string{github::github_comment_identifier} + body);
  }

  template <class Fn>
  auto capture_runtime_error(Fn &&fn) -> std::string {
    try {
      std::forward<Fn>(fn)();
    } catch (const std::runtime_error &err) {
      return err.what();
    }

    FAIL("expected std::runtime_error");
    return {};
  }

  auto contains_text(const std::string &text, const std::string &needle) -> bool {
    return text.find(needle) != std::string::npos;
  }
} // namespace

TEST_CASE("GitHub client creates issue comments with the REST JSON API",
          "[cpp-lint-action][github]") {
  auto server = httplib::Server{};
  auto mutex  = std::mutex{};
  auto record = request_record{};

  server.Post("/repos/owner/repo/issues/42/comments",
              [&](const httplib::Request &req, httplib::Response &res) {
                record_request(req, record, mutex);

                auto response = nlohmann::json{};
                response["id"] = comment_id;
                res.set_content(response.dump(), "application/json");
              });

  auto runner        = server_runner{server};
  auto github_client = github::client{runner.api_url()};
  github_client.add_or_update_issue_comment(make_context(), "new comment");

  auto lock = std::lock_guard{mutex};
  check_github_json_request(record, "new comment");
}

TEST_CASE("GitHub client updates existing issue comments with PATCH",
          "[cpp-lint-action][github]") {
  auto server = httplib::Server{};
  auto mutex  = std::mutex{};
  auto record = request_record{};

  server.Get("/repos/owner/repo/issues/42/comments",
             [](const httplib::Request &, httplib::Response &res) {
               auto comments = nlohmann::json::array(
                 {{{"id", comment_id},
                   {"body", std::string{github::github_comment_identifier} + "old"}}});
               res.set_content(comments.dump(), "application/json");
             });
  server.Patch("/repos/owner/repo/issues/comments/5000000000",
               [&](const httplib::Request &req, httplib::Response &res) {
                 record_request(req, record, mutex);

                 auto response = nlohmann::json{};
                 response["id"] = comment_id;
                 res.set_content(response.dump(), "application/json");
               });

  auto runner        = server_runner{server};
  auto github_client = github::client{runner.api_url()};
  auto ctx           = make_context();
  github_client.get_issue_comment_id(ctx);
  github_client.add_or_update_issue_comment(ctx, "updated comment");

  auto lock = std::lock_guard{mutex};
  check_github_json_request(record, "updated comment");
}

TEST_CASE("GitHub client creates a comment after reading an empty comment list",
          "[cpp-lint-action][github]") {
  auto server = httplib::Server{};
  auto mutex  = std::mutex{};
  auto record = request_record{};

  server.Get("/repos/owner/repo/issues/42/comments",
             [](const httplib::Request &, httplib::Response &res) {
               res.set_content(nlohmann::json::array().dump(), "application/json");
             });
  server.Post("/repos/owner/repo/issues/42/comments",
              [&](const httplib::Request &req, httplib::Response &res) {
                record_request(req, record, mutex);

                auto response = nlohmann::json{};
                response["id"] = comment_id;
                res.set_content(response.dump(), "application/json");
              });

  auto runner        = server_runner{server};
  auto github_client = github::client{runner.api_url()};
  auto ctx           = make_context();
  github_client.get_issue_comment_id(ctx);
  github_client.add_or_update_issue_comment(ctx, "first comment");

  auto lock = std::lock_guard{mutex};
  check_github_json_request(record, "first comment");
}

TEST_CASE("GitHub client posts pull request reviews with JSON API headers",
          "[cpp-lint-action][github]") {
  auto server = httplib::Server{};
  auto mutex  = std::mutex{};
  auto record = request_record{};

  server.Post("/repos/owner/repo/pulls/42/reviews",
              [&](const httplib::Request &req, httplib::Response &res) {
                record_request(req, record, mutex);
                res.set_content(R"({"id":100})", "application/json");
              });

  auto comments = github::review_comments{};
  comments.push_back(github::review_comment{
    .path     = "src/main.cpp",
    .position = 3,
    .body     = "diagnostic body",
  });

  auto runner        = server_runner{server};
  auto github_client = github::client{runner.api_url()};
  github_client.post_pull_request_review(make_context(), github::make_review_str(comments));

  auto lock = std::lock_guard{mutex};
  REQUIRE(record.called);
  REQUIRE(record.accept == "application/vnd.github+json");
  REQUIRE(record.authorization == "Bearer github-token");
  REQUIRE(record.user_agent == "cpp-lint-action");
  REQUIRE(record.api_version == "2022-11-28");
  REQUIRE(record.content_type.starts_with("application/json"));

  auto payload = nlohmann::json::parse(record.body);
  REQUIRE(payload["event"].get<std::string>() == "COMMENT");
  REQUIRE(payload["comments"].size() == 1);
  REQUIRE(payload["comments"][0]["path"].get<std::string>() == "src/main.cpp");
  REQUIRE(payload["comments"][0]["position"].get<std::size_t>() == 3);
  REQUIRE(payload["comments"][0]["body"].get<std::string>() == "diagnostic body");
}

TEST_CASE("GitHub client includes response body when REST API returns an error",
          "[cpp-lint-action][github]") {
  auto server = httplib::Server{};
  server.Get("/repos/owner/repo/issues/42/comments",
             [](const httplib::Request &, httplib::Response &res) {
               res.status = 404;
               res.reason = "Not Found";
               res.set_content(R"({"message":"missing issue"})", "application/json");
             });

  auto runner        = server_runner{server};
  auto github_client = github::client{runner.api_url()};

  const auto message =
    capture_runtime_error([&github_client] { github_client.get_issue_comment_id(make_context()); });
  REQUIRE(contains_text(message, "404"));
  REQUIRE(contains_text(message, "missing issue"));
}

TEST_CASE("GitHub client paginates issue comments before updating the newest match",
          "[cpp-lint-action][github]") {
  auto server = httplib::Server{};
  auto mutex  = std::mutex{};
  auto record = request_record{};

  server.Get("/repos/owner/repo/issues/42/comments",
             [](const httplib::Request &req, httplib::Response &res) {
               auto comments = nlohmann::json::array();
               if (req.get_param_value("page") == "1") {
                 for (auto index = 0; index < 100; ++index) {
                   comments.push_back({{"id", index + 1}, {"body", "other comment"}});
                 }
               } else {
                 comments.push_back(
                   {{"id", comment_id - 1},
                    {"body", std::string{github::github_comment_identifier} + "older"}});
                 comments.push_back(
                   {{"id", comment_id},
                    {"body", std::string{github::github_comment_identifier} + "newer"}});
               }
               res.set_content(comments.dump(), "application/json");
             });
  server.Patch("/repos/owner/repo/issues/comments/5000000000",
               [&](const httplib::Request &req, httplib::Response &res) {
                 record_request(req, record, mutex);
                 res.set_content(R"({"id":5000000000})", "application/json");
               });

  auto runner        = server_runner{server};
  auto github_client = github::client{runner.api_url()};
  auto ctx           = make_context();
  github_client.get_issue_comment_id(ctx);
  github_client.add_or_update_issue_comment(ctx, "updated newest comment");

  auto lock = std::lock_guard{mutex};
  check_github_json_request(record, "updated newest comment");
  REQUIRE(record.path == "/repos/owner/repo/issues/comments/5000000000");
}

TEST_CASE("GitHub client retries retryable REST API failures", "[cpp-lint-action][github]") {
  auto server = httplib::Server{};
  auto calls  = std::atomic_int{0};
  auto mutex  = std::mutex{};
  auto record = request_record{};

  server.Post("/repos/owner/repo/issues/42/comments",
              [&](const httplib::Request &req, httplib::Response &res) {
                ++calls;
                record_request(req, record, mutex);
                if (calls.load() == 1) {
                  res.status = 500;
                  res.reason = "Internal Server Error";
                  res.set_content(R"({"message":"temporary"})", "application/json");
                  return;
                }

                res.set_content(R"({"id":5000000000})", "application/json");
              });

  auto runner        = server_runner{server};
  auto github_client = github::client{runner.api_url()};
  github_client.add_issue_comment(make_context(), "retry comment");

  REQUIRE(calls.load() == 2);
  auto lock = std::lock_guard{mutex};
  check_github_json_request(record, "retry comment");
}

TEST_CASE("GitHub client reports rate-limit headers on REST API errors",
          "[cpp-lint-action][github]") {
  auto server = httplib::Server{};
  server.Get("/repos/owner/repo/issues/42/comments",
             [](const httplib::Request &, httplib::Response &res) {
               res.status = 403;
               res.reason = "Forbidden";
               res.set_header("X-RateLimit-Remaining", "0");
               res.set_header("X-RateLimit-Reset", "1234567890");
               res.set_content(R"({"message":"rate limit"})", "application/json");
             });

  auto runner        = server_runner{server};
  auto github_client = github::client{runner.api_url()};

  const auto message =
    capture_runtime_error([&github_client] { github_client.get_issue_comment_id(make_context()); });
  REQUIRE(contains_text(message, "403"));
  REQUIRE(contains_text(message, "rate-limit"));
  REQUIRE(contains_text(message, "remaining=0"));
  REQUIRE(contains_text(message, "reset=1234567890"));
}

TEST_CASE("GitHub client reports empty HTTP responses without dereferencing them",
          "[cpp-lint-action][github]") {
  auto github_client = github::client{"http://127.0.0.1:1"};

  const auto message = capture_runtime_error(
    [&github_client] { github_client.add_issue_comment(make_context(), "body"); });
  REQUIRE(contains_text(message, "github http request failed"));
}
