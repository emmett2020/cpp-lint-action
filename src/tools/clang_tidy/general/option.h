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

#include "tools/base_option.h"

namespace lint::tool::clang_tidy {
  struct option_t : option_base {
    bool allow_no_checks      = false;
    bool enable_check_profile = false;
    std::string checks;
    std::string config;
    std::string config_file;
    std::string database;
    std::string header_filter;
    std::string line_filter;
  };

  void print_option(const option_t& option);
} // namespace lint::tool::clang_tidy
