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

#include <spdlog/spdlog.h>
#include <string>

#include "utils/common.h"
#include "utils/error.h"
#include "utils/shell.h"

namespace lint::tool {
  // Find the full executable path of clang tools with specific version.
  inline auto find_clang_tool(std::string_view tool, std::string_view version) -> std::string {
    auto command                = fmt::format("{}-{}", tool, version);
    auto [ec, std_out, std_err] = shell::which(command);
    throw_if(ec != 0, fmt::format("find {}-{} failed, error message: {}", tool, version, std_err));
    auto trimmed = trim(std_out);
    throw_if(trimmed.empty(), "got empty clang tool path");
    return {trimmed.data(), trimmed.size()};
  }
} // namespace lint::tool
