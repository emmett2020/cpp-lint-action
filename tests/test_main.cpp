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

#include <spdlog/spdlog.h>
#include <catch2/catch_all.hpp>

#include "utils/common.h"
#include "utils/git_utils.h"
#include "utils/env_manager.h"

using namespace lint;

int main(int argc, char *argv[]) {
  auto log_level = env::get("CPP_LINT_ACTION_TEST_LOG_LEVEL");
  if (!log_level.empty()) {
    set_log_level(log_level);
  }

  git::setup();
  int result = Catch::Session().run(argc, argv);
  git::shutdown();
  return result;
}
