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

#include <boost/program_options.hpp>
#include <boost/program_options/detail/parsers.hpp>

#include "context.h"

namespace lint::program_options {
  using options_description = boost::program_options::options_description;
  using variables_map       = boost::program_options::variables_map;
  using boost::program_options::notify;
  using boost::program_options::parse_command_line;
  using boost::program_options::store;
  using boost::program_options::value;

  /// Create description of cpp-lint-action command line options.
  auto create_desc() -> options_description;

  /// Parse user inputs based on the given options description.
  auto parse(int argc, char **argv, const options_description &desc) -> variables_map;

  /// Fill runtime context by program options.
  void fill_context(const variables_map &variables, runtime_context &ctx);

  /// Some options must be specified on the given condition, check it.
  void must_specify(const std::string &condition,
                    const variables_map &variables,
                    const std::initializer_list<const char *> &options);

  /// Some options mustn't be specified on the given condition, check it.
  void must_not_specify(const std::string &condition,
                        const variables_map &variables,
                        const std::initializer_list<const char *> &options);
} // namespace lint::program_options
