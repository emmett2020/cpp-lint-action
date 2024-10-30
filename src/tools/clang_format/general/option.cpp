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
#include "tools/clang_format/general/option.h"

#include <spdlog/spdlog.h>

namespace lint::tool::clang_format {
  void print_option(const option_t& option) {
    spdlog::debug("Clang-format Option: ");
    spdlog::debug("--------------------------------------------------");
    spdlog::debug("enabled: {}", option.enabled);
    spdlog::debug("enabled-fastly-exit: {}", option.enabled_fastly_exit);
    spdlog::debug("version: {}", option.version);
    spdlog::debug("binary: {}", option.binary);
    spdlog::debug("file-filter-iregex: {}", option.file_filter_iregex);
    spdlog::debug("enable-warning-as-error: {}", option.enable_warning_as_error);
    spdlog::debug("");
  }

} // namespace lint::tool::clang_format
