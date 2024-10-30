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

#include "tools/clang_tidy/general/impl.h"

namespace lint::tool::clang_tidy {
  using namespace std::string_view_literals;

  constexpr auto version_18_1_0 = "18.1.0"sv;
  constexpr auto version_18_1_3 = "18.1.3"sv;

  struct clang_tidy_v18_1_0 : clang_tidy_general {
    explicit clang_tidy_v18_1_0(option_t opt)
      : clang_tidy_general(std::move(opt)) {
    }

    constexpr auto version() -> std::string_view override {
      return version_18_1_0;
    }
  };

  struct clang_tidy_v18_1_3 : clang_tidy_general {
    explicit clang_tidy_v18_1_3(option_t opt)
      : clang_tidy_general(std::move(opt)) {
    }

    constexpr auto version() -> std::string_view override {
      return version_18_1_3;
    }
  };

} // namespace lint::tool::clang_tidy
