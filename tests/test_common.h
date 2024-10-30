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

#include <cctype>
#include <filesystem>
#include <fstream>

#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <git2/diff.h>
#include <spdlog/spdlog.h>

#include "utils/git_utils.h"

using namespace lint; // NOLINT

// Get temporary repository directory.
auto get_temp_repo_dir() -> std::filesystem::path;

struct repo_t {
  explicit repo_t(const std::string &path = get_temp_repo_dir())
    : repo_path(path)
    , repo(nullptr, ::git_repository_free) {
    remove();
    std::filesystem::create_directory(path);
    init();
  }

  ~repo_t() {
    remove();
  }

  // This will always remove old file.
  auto add_file(const std::string &file_path, const std::string &content) {
    auto file_full_path = repo_path / file_path;
    if (std::filesystem::exists(file_full_path)) {
      std::filesystem::remove(file_full_path);
    }
    auto file = std::fstream(file_full_path, std::ios::out);
    REQUIRE(file.is_open());
    file << content;
    file.close();
    if (!ranges::contains(modified_or_added_files, file_path)) {
      modified_or_added_files.push_back(file_path);
    }
  }

  // This will not really remove old file till comment_changes.
  auto remove_file(const std::string &file_path) {
    if (!ranges::contains(deleted_files, file_path)) {
      deleted_files.push_back(file_path);
    }
  }

  void append_content_to_exist_file(const std::string &file_path, const std::string &content) {
    auto file_full_path = repo_path / file_path;
    REQUIRE(std::filesystem::exists(file_full_path));

    auto file = std::fstream(file_full_path, std::ios::app);
    REQUIRE(file.is_open());
    file << content;
    file.close();
  }

  // Rewrite exist file's content. This could be safely called by user multiple
  // times. And the last version of it will be finally used.
  auto rewrite_file(const std::string &file_path, const std::string &content) {
    auto file_full_path = repo_path / file_path;
    REQUIRE(std::filesystem::exists(file_full_path));

    auto file = std::fstream(file_full_path, std::ios::out);
    REQUIRE(file.is_open());
    file << content;
    file.close();
    if (!ranges::contains(modified_or_added_files, file_path)) {
      modified_or_added_files.push_back(file_path);
    }
  }

  auto commit_changes() -> std::string {
    auto [index_oid1, index1] = git::index::add_files(*repo, modified_or_added_files);
    auto [index_oid, index]   = git::index::remove_files(*repo, repo_path, deleted_files);
    auto message              = fmt::format("Commit Index {}", commit_idx);
    auto commit_oid           = git::commit::create_head(*repo, message, *index);
    return git::oid::to_str(commit_oid);
  }

  void remove() {
    if (std::filesystem::exists(repo_path)) {
      std::filesystem::remove_all(repo_path);
    }
  }

  auto get_path() -> std::filesystem::path {
    return repo_path;
  }

  void commit_clang_format() {
    auto content  = std::string{};
    content      += "BasedOnStyle: Google\n";
    content      += "AllowShortBlocksOnASingleLine: Never\n";
    add_file(".clang-format", content);
    [[maybe_unused]] auto ret = commit_changes();
  }

  auto commit_clang_tidy() -> std::string {
    const auto *content = R"(
Checks: '
  -*,
  cppcoreguidelines-*,
'
WarningsAsErrors: '*'
    )";
    add_file(".clang-tidy", content);
    return commit_changes();
  }



private:
  void init() {
    using namespace lint;
    repo = git::repo::init(repo_path, false);
    REQUIRE(git::repo::is_empty(*repo));
    auto config = git::repo::config(*repo);
    git::config::set_string(*config, "user.name", user_name);
    git::config::set_string(*config, "user.email", user_email);
  }

  static constexpr auto user_name  = "test";
  static constexpr auto user_email = "test@email.com";

  std::filesystem::path repo_path;
  lint::git::repo_ptr repo;
  std::size_t commit_idx = 1;

  std::vector<std::string> modified_or_added_files;
  std::vector<std::string> deleted_files;
};

// Remove repository.
void remove_temp_repo_dir();

// Create temporary repository. If repository already exists, it'll be
// refreshed.
void create_temp_repo_dir();

// Create file in temporary repository.
void create_temp_file(const std::string &file_path, const std::string &content);

// Create files in temporary repository. Each file contains the same content.
void create_temp_files(const std::vector<std::string> &file_paths, const std::string &content);

// Append the given content into the given file.
void append_content_to_file(const std::string &file, const std::string &content);

// Initialize a basic repo for futhure test.
auto init_basic_repo() -> lint::git::repo_ptr;

class scope_guard {
public:
  explicit scope_guard(std::function<void()> f)
    : func_(std::move(f)) {
  }

  ~scope_guard() {
    if (func_) {
      func_();
    }
  }

  scope_guard(const scope_guard &)            = delete;
  scope_guard &operator=(const scope_guard &) = delete;
  scope_guard(scope_guard &&)                 = delete;
  scope_guard &operator=(scope_guard &&)      = delete;

private:
  std::function<void()> func_;
};
