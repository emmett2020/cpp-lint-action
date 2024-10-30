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

#include "test_common.h"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <git2/diff.h>
#include <ios>
#include <iostream>

#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <spdlog/spdlog.h>

#include "utils/git_utils.h"

using namespace std;
using namespace lint;

const auto temp_repo_dir = std::filesystem::temp_directory_path() / "test_git";

auto get_temp_repo_dir() -> filesystem::path {
  return temp_repo_dir;
}

void remove_temp_repo_dir() {
  if (std::filesystem::exists(temp_repo_dir)) {
    std::filesystem::remove_all(temp_repo_dir);
  }
}

void create_temp_repo_dir() {
  remove_temp_repo_dir();
  std::filesystem::create_directory(temp_repo_dir);
}

void create_temp_file(const std::string &file_path, const std::string &content) {
  auto new_file_path = temp_repo_dir / file_path;
  if (std::filesystem::exists(new_file_path)) {
    std::filesystem::remove(new_file_path);
  }
  auto file = std::fstream(new_file_path, std::ios::out);
  REQUIRE(file.is_open());
  file << content;
  file.close();
}

void create_temp_files(const std::vector<std::string> &file_paths, const std::string &content) {
  for (const auto &file: file_paths) {
    create_temp_file(file, content);
  }
}

void append_content_to_file(const std::string &file, const std::string &content) {
  auto new_file_path = temp_repo_dir / file;
  auto fstream       = std::fstream(new_file_path, std::ios::app);
  REQUIRE(fstream.is_open());
  fstream << content;
  fstream.close();
}

auto init_basic_repo() -> git::repo_ptr {
  auto repo = git::repo::init(temp_repo_dir, false);
  REQUIRE(git::repo::is_empty(*repo));
  auto config = git::repo::config(*repo);
  git::config::set_string(*config, "user.name", "test");
  git::config::set_string(*config, "user.email", "test@email.com");
  return repo;
}

