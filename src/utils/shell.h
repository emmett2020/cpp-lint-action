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

#include <string>
#include <unordered_map>
#include <vector>

namespace lint::shell {
  struct result {
    int exit_code;
    std::string std_out;
    std::string std_err;
  };

  using envrionment = std::unordered_map<std::string, std::string>;
  using options     = std::vector<std::string>;

  auto execute(std::string_view command, const options &opts) -> result;
  auto execute(std::string_view command, const options &opts, std::string_view start_dir) -> result;
  auto execute(std::string_view command, const options &opts, const envrionment &env) -> result;
  auto execute(std::string_view command,
               const options &opts,
               const envrionment &env,
               std::string_view start_dir) -> result;

  auto which(std::string command) -> result;
} // namespace lint::shell
