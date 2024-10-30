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

namespace lint::tool::clang_tidy {
  /// Represents statistics outputed by clang-tidy. It's usually the stderr
  /// messages of clang-tidy.
  struct statistic {
    std::uint32_t warnings                   = 0;
    std::uint32_t errors                     = 0;
    std::uint32_t warnings_treated_as_errors = 0;
    std::uint32_t total_suppressed_warnings  = 0;
    std::uint32_t non_user_code_warnings     = 0;
    std::uint32_t no_lint_warnings           = 0;
  };

  /// Each diagnostic hase a header line.
  struct diagnostic_header {
    std::string file_name;
    std::string row_idx;
    std::string col_idx;
    std::string serverity;
    std::string brief;
    std::string checks;
  };

  /// Represents one diagnostic which outputed by clang-tidy.
  /// Generally, each diagnostic has a header line and several details line
  /// which give a further detailed explanation.
  struct diagnostic {
    diagnostic_header header;
    std::string details;
  };

  /// Represents all diagnostics which outputed by clang-tidy.
  using diagnostics = std::vector<diagnostic>;

  struct per_file_result : per_file_result_base {
    statistic stat;
    diagnostics diags;
  };

  using result_t = multi_files_result_base<per_file_result>;
} // namespace lint::tool::clang_tidy
