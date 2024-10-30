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
#include "program_options.h"
#include "utils/env_manager.h"

#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace lint;
using namespace lint::program_options;

namespace {
  template <class... Opts>
  auto make_opt(Opts &&...opts) -> std::array<char *, sizeof...(Opts) + 1> {
    return {const_cast<char *>("cpp-lint-action"), // NOLINT
            const_cast<char *>(std::forward<Opts &&>(opts))...};
  }
} // namespace

TEST_CASE("Test create program options descriptions", "[cpp-lint-action][program_options]") {
  auto desc = create_desc();

  SECTION("help") {
    auto opts         = make_opt("--help");
    auto user_options = parse(opts.size(), opts.data(), desc);
    REQUIRE(user_options.contains("help"));
  }

  SECTION("version") {
    auto opts         = make_opt("--version");
    auto user_options = parse(opts.size(), opts.data(), desc);
    REQUIRE(user_options.contains("version"));
  }
}

TEST_CASE("Test must_specify could throw", "[cpp-lint-action][program_options]") {
  auto desc         = program_options::create_desc();
  auto opts         = make_opt("--help");
  auto user_options = parse(opts.size(), opts.data(), desc);
  REQUIRE_NOTHROW(must_specify("test", user_options, {"help"}));
  REQUIRE_THROWS(must_specify("test", user_options, {"version"}));
}

TEST_CASE("Test must_not_specify could throw", "[cpp-lint-action][program_options]") {
  auto desc         = create_desc();
  auto opts         = make_opt("--help");
  auto user_options = parse(opts.size(), opts.data(), desc);
  REQUIRE_THROWS(must_not_specify("test", user_options, {"help"}));
  REQUIRE_NOTHROW(must_not_specify("test", user_options, {"version"}));
}

TEST_CASE("Test fill context by program options", "[cpp-lint-action][program_options]") {
  auto desc    = create_desc();
  auto context = runtime_context{};


  SECTION("user not specifies target-revision with push event should throw exception") {
    auto opts         = make_opt("--log-level=info");
    auto user_options = parse(opts.size(), opts.data(), desc);
    REQUIRE_THROWS(fill_context(user_options, context));
  }

  SECTION("enable_step_summary should be passed into context") {
    auto opts         = make_opt("--target-revision=main", "--enable-step-summary=false");
    auto user_options = parse(opts.size(), opts.data(), desc);
    REQUIRE_NOTHROW(fill_context(user_options, context));
    REQUIRE(context.enable_step_summary == false);
  }

  SECTION("enable_action_output should be passed into context") {
    auto opts         = make_opt("--target-revision=main", "--enable-action-output=false");
    auto user_options = parse(opts.size(), opts.data(), desc);
    REQUIRE_NOTHROW(fill_context(user_options, context));
    REQUIRE(context.enable_action_output == false);
  }

  SECTION("enable_comment_on_issue should be passed into context") {
    auto opts         = make_opt("--target-revision=main", "--enable-step-summary=true");
    auto user_options = parse(opts.size(), opts.data(), desc);
    REQUIRE_NOTHROW(fill_context(user_options, context));
    REQUIRE(context.enable_step_summary == true);
  }

  SECTION("enable_pull_request_review should be passed into context") {
    auto opts         = make_opt("--target-revision=main", "--enable-pull-request-review=true");
    auto user_options = parse(opts.size(), opts.data(), desc);
    REQUIRE_NOTHROW(fill_context(user_options, context));
    REQUIRE(context.enable_pull_request_review == true);
  }

  SECTION("default values should be passed into context") {
    auto opts         = make_opt("--target-revision=main");
    auto user_options = parse(opts.size(), opts.data(), desc);
    REQUIRE_NOTHROW(fill_context(user_options, context));
    REQUIRE(context.enable_step_summary == true);
    REQUIRE(context.enable_comment_on_issue == true);
    REQUIRE(context.enable_pull_request_review == false);
    REQUIRE(context.enable_action_output == true);
  }
}
