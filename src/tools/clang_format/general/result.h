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

#include <vector>

#include "tools/base_result.h"

namespace lint::tool::clang_format {
  struct replacement_t {
    int offset;       // character offset in unformatted file
    int length;       // how may characters to be removed
    std::string data; // new data to be inserted
    int row;          // row number in unformatted file
    int col;          // col number in unformatted file
  };

  // using replacements_t = std::vector<replacement_t>;

  // row -> replacements
  using replacements_t = std::unordered_map<std::size_t, std::vector<replacement_t>>;

  struct per_file_result : per_file_result_base {
    replacements_t replacements;
    std::string formatted_source_code;
  };

  using result_t = multi_files_result_base<per_file_result>;
} // namespace lint::tool::clang_format
