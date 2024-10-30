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

#include <memory>
#include <string>

namespace lint::tool {
  /// Provide a base option for all tools.
  struct option_base {
    /// Whether user want to use this tool.
    bool enabled = false;

    /// Exited check process as soon as error occurs.
    bool enabled_fastly_exit = false;

    /// The version of this tool. It's usually the full version of tool.
    std::string version;

    /// The executable binary of this tool.
    std::string binary;

    /// Used to filt files.
    std::string file_filter_iregex = R"(.*\.(cpp|cc|c\+\+|cxx|c|cl|h|hpp|m|mm|inc))";
  };

  using option_base_ptr = std::unique_ptr<option_base>;
} // namespace lint::tool
