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

#include "program_options.h"
#include "tools/base_tool.h"

namespace lint::tool {
  /// This class used to create tool and it's option.
  struct creator_base {
    virtual ~creator_base() = default;

    /// Register program options.
    /// WARN: Should only call spdlog with error log level here since we don't
    /// known what log level user want to use.
    virtual void register_option(program_options::options_description &desc) const = 0;

    /// Create tool instance.
    virtual auto create_tool(const program_options::variables_map &variables) -> tool_base_ptr = 0;
  };

  using creator_base_ptr = std::unique_ptr<creator_base>;

  /// An utility to register options for multiple creators.
  inline auto register_tool_options(const std::vector<creator_base_ptr> &creators,
                                    program_options::options_description &desc) {
    for (const auto &creator: creators) {
      creator->register_option(desc);
    }
  }

  /// An utility to create enabled tools for multiple creators.
  inline auto
  create_enabled_tools(const std::vector<creator_base_ptr> &creators,
                       program_options::variables_map &variables) -> std::vector<tool_base_ptr> {
    auto res = std::vector<tool_base_ptr>{};
    for (const auto &creator: creators) {
      auto tool = creator->create_tool(variables);
      if (tool) {
        res.emplace_back(std::move(tool));
      }
    }
    return res;
  }

} // namespace lint::tool
