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

#include "tools/base_creator.h"
#include "tools/base_tool.h"
#include "tools/clang_format/general/option.h"

namespace lint::tool::clang_format {
  /// The creator to create clang-format instance.
  struct creator : creator_base {
    creator()                = default;
    ~creator() override      = default;
    creator(creator &&other) = default;

    /// Register clang-format needed options to program options description.
    void register_option(program_options::options_description &desc) const override;

    /// Create clang-format option struct by user input program options.
    void create_option(const program_options::variables_map &variables);

    /// Create clang-format tool instance.
    auto create_tool(const program_options::variables_map &variables) -> tool_base_ptr override;

    [[nodiscard]] auto get_option() const -> const option_t &;

  private:
    option_t option;
  };

  /// Get clang-format version based on clang-format output
  auto get_version(const std::string &binary) -> std::string;

} // namespace lint::tool::clang_format
