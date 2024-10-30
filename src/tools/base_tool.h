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

#include "context.h"
#include "tools/base_reporter.h"
#include "utils/platform.h"

namespace lint::tool {
  /// This is a base class represents lint tools. All specified tools should be
  /// derived from this.
  struct tool_base {
    virtual ~tool_base() = default;

    /// Check whether this tool is supported on the given platform.
    virtual bool is_supported(operating_system_t system, arch_t arch) = 0;

    /// Return unique name of this tool.
    virtual auto name() -> std::string_view = 0;

    /// Return version of this tool.
    virtual auto version() -> std::string_view = 0;

    /// Return binary path of this tool.
    virtual auto binary() -> std::string_view = 0;

    /// Apply this tool to a single file.
    virtual void check(const runtime_context &context) = 0;

    /// Return the result reporter. To get the result, you must first call check().
    virtual auto get_reporter() -> reporter_base_ptr = 0;
  };

  /// An unique pointer for base tool.
  using tool_base_ptr = std::unique_ptr<tool_base>;

  /// Run the given tools one by one and return the reporter of each tool in order.
  inline auto run_tools(const std::vector<tool_base_ptr> &tools, const runtime_context &context)
    -> std::vector<reporter_base_ptr> {
    auto ret = std::vector<reporter_base_ptr>{};
    for (const auto &tool: tools) {
      tool->check(context);
      ret.emplace_back(tool->get_reporter());
    }
    return ret;
  }

} // namespace lint::tool
