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
#include <utility>

#include "tools/base_reporter.h"
#include "tools/base_tool.h"
#include "tools/clang_format/general/option.h"
#include "tools/clang_format/general/result.h"

namespace lint::tool::clang_format {
  /// The general implementation of clang-format.
  struct clang_format_general : tool_base {
    explicit clang_format_general(option_t opt)
      : option(std::move(opt)) {
    }

    bool is_supported(operating_system_t system, arch_t arch) override {
      return system == operating_system_t::ubuntu && arch == arch_t::x86_64;
    }

    constexpr auto name() -> std::string_view override {
      return "clang-format";
    }

    auto version() -> std::string_view override {
      return option.version;
    }

    auto binary() -> std::string_view override {
      return option.binary;
    }

    auto check_single_file(const runtime_context &context,
                           const std::string &root_dir,
                           const std::string &file) const -> per_file_result;

    void check(const runtime_context &context) override;

    auto get_reporter() -> reporter_base_ptr override;

    option_t option;
    result_t result;
  };

} // namespace lint::tool::clang_format
