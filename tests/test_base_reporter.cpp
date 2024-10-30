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

#include "tools/clang_tidy/general/reporter.h"
#include "tools/clang_format/general/reporter.h"

#include <vector>

#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace lint;

namespace { } // namespace

TEST_CASE("Test all_passes function", "[cpp-lint-action][tools]") {
  auto tidy_opt = tool::clang_tidy::option_t{};
  auto tidy_res = tool::clang_tidy::result_t{};

  auto format_opt = tool::clang_format::option_t{};
  auto format_res = tool::clang_format::result_t{};

  SECTION("all passes") {
    tidy_res.final_passed   = true;
    format_res.final_passed = true;

    auto reporters = std::vector<tool::reporter_base_ptr>();
    reporters.emplace_back(std::make_unique<tool::clang_tidy::reporter_t>(tidy_opt, tidy_res));
    reporters.emplace_back(
      std::make_unique<tool::clang_format::reporter_t>(format_opt, format_res));
    REQUIRE(tool::all_passed(reporters));
  }

  SECTION("at least one failed") {
    tidy_res.final_passed   = false;
    format_res.final_passed = true;

    auto reporters = std::vector<tool::reporter_base_ptr>();
    reporters.emplace_back(std::make_unique<tool::clang_tidy::reporter_t>(tidy_opt, tidy_res));
    reporters.emplace_back(
      std::make_unique<tool::clang_format::reporter_t>(format_opt, format_res));
    REQUIRE(!tool::all_passed(reporters));
  }
}


