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
#include <unordered_map>
#include <vector>

namespace lint::tool {

  struct per_file_result_base {
    bool passed = false;
    std::string file_path;
    std::string tool_stdout;
    std::string tool_stderr;
    std::string file_option;
  };

  using per_file_result_base_ptr = std::unique_ptr<per_file_result_base>;

  template <class PerFileResult>
  struct multi_files_result_base {
    bool final_passed  = false;
    bool fastly_exited = false;

    std::vector<std::string> ignored;
    std::unordered_map<std::string, PerFileResult> passes;
    std::unordered_map<std::string, PerFileResult> fails;

    std::vector<std::string> failed_commands;
  };

} // namespace lint::tool
