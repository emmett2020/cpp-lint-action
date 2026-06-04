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

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <git2/diff.h>
#include <iostream>

#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <spdlog/spdlog.h>

#include "context.h"
#include "test_common.h"
#include "utils/git_utils.h"

using namespace lint;
using namespace std::string_literals;
using namespace std::string_view_literals;

const auto default_branch = "master"s;

namespace {
  auto clean_commit_id(std::string id) -> std::string {
    id.erase(std::find(id.begin(), id.end(), '\0'), id.end());
    return id;
  }

  auto commit_changes(repo_t &repo) -> std::string {
    return clean_commit_id(repo.commit_changes());
  }
} // namespace

TEST_CASE("Create repo should work", "[cpp-lint-action][git2][repo]") {
  create_temp_repo_dir();
  auto guard = scope_guard{remove_temp_repo_dir};

  auto repo = git::repo::init(get_temp_repo_dir(), false);
  REQUIRE(git::repo::is_empty(*repo));
  auto temp_repo_dir_with_git = get_temp_repo_dir() / ".git/";
  REQUIRE(std::filesystem::weakly_canonical(git::repo::path(*repo))
          == std::filesystem::weakly_canonical(temp_repo_dir_with_git));
}

TEST_CASE("Set config should work", "[cpp-lint-action][git2][config]") {
  create_temp_repo_dir();
  auto guard = scope_guard{remove_temp_repo_dir};

  auto repo   = git::repo::init(get_temp_repo_dir(), false);
  auto origin = git::repo::config(*repo);
  SECTION("set_string") {
    git::config::set_string(*origin, "user.name", "test");
  }
  SECTION("set_bool") {
    git::config::set_bool(*origin, "core.filemode", true);
  }

  auto config = git::repo::config_snapshot(*repo);
  SECTION("get_bool") {
    REQUIRE(git::config::get_bool(*config, "core.filemode") == true);
  }
}

TEST_CASE("Compare with head", "[cpp-lint-action][git2][status]") {
  create_temp_repo_dir();
  auto guard = scope_guard{remove_temp_repo_dir};

  auto repo = git::repo::init(get_temp_repo_dir(), false);
  REQUIRE(git::repo::is_empty(*repo));

  // Default is HEAD
  auto options     = git::status::default_options();
  auto status_list = git::status::gather(*repo, options);
  REQUIRE(git::status::entry_count(*status_list) == 0);
}

TEST_CASE("Commit two new files step by step", "[cpp-lint-action][git2][commit]") {
  create_temp_repo_dir();
  auto guard = scope_guard{remove_temp_repo_dir};

  // Create repository instance
  auto repo   = git::repo::init(get_temp_repo_dir(), false);
  auto config = git::repo::config(*repo);
  git::config::set_string(*config, "user.name", "test");
  git::config::set_string(*config, "user.email", "test@email.com");

  // Add files to repository
  create_temp_file("file1.cpp", "hello world");
  create_temp_file("file2.cpp", "hello world");
  auto index = git::repo::index(*repo);
  git::index::add_by_path(*index, "file1.cpp");
  git::index::add_by_path(*index, "file2.cpp");
  auto index_tree_oid = git::index::write_tree(*index);

  // Get repository status
  auto options     = git::status::default_options();
  auto status_list = git::status::gather(*repo, options);
  REQUIRE(git::status::entry_count(*status_list) == 2);

  const auto *entry0 = git::status::get_by_index(*status_list, 0);
  const auto *entry1 = git::status::get_by_index(*status_list, 1);
  REQUIRE(entry0->status == git_status_t::GIT_STATUS_INDEX_NEW);
  REQUIRE(entry1->status == git_status_t::GIT_STATUS_INDEX_NEW);

  // We did't have any branches yet.
  auto index_tree_obj = git::tree::lookup(*repo, index_tree_oid);
  auto sig            = git::sig::create_default(*repo);
  auto commit_oid =
    git::commit::create(*repo, "HEAD", *sig, *sig, "Initial commit", *index_tree_obj, {});
  REQUIRE(git::branch::current_name(*repo) == default_branch);
}

TEST_CASE("Add three files to index by our utility", "[cpp-lint-action][git2][index]") {
  create_temp_repo_dir();
  auto guard = scope_guard{remove_temp_repo_dir};

  auto repo = init_basic_repo();

  // Stage two files.
  const auto files = std::vector<std::string>{"file1.cpp", "file2.cpp"};
  create_temp_files(files, "hello world");
  git::index::add_files(*repo, files);
  auto options     = git::status::default_options();
  auto status_list = git::status::gather(*repo, options);
  REQUIRE(git::status::entry_count(*status_list) == 2);

  // Stage a new files.
  create_temp_file("file3.cpp", "hello world");
  git::index::add_files(*repo, {"file3.cpp"});
  status_list = git::status::gather(*repo, options);
  REQUIRE(git::status::entry_count(*status_list) == 3);
}

TEST_CASE("Delete two files to index by our utility", "[cpp-lint-action][git2][index]") {
  create_temp_repo_dir();
  auto guard = scope_guard{remove_temp_repo_dir};
  auto repo  = init_basic_repo();

  // Firstly, we create two files and commit it.
  const auto files = std::vector<std::string>{"file1.cpp", "file2.cpp"};
  create_temp_files(files, "hello world");
  auto ret = git::index::add_files(*repo, files);
  git::commit::create_head(*repo, "Add two files", *std::get<1>(ret));

  // Then we remove the files just created.
  git::index::remove_files(*repo, get_temp_repo_dir(), files);
  auto options     = git::status::default_options();
  auto status_list = git::status::gather(*repo, options);
  REQUIRE(git::status::entry_count(*status_list) == 2);

  const auto file1_path = get_temp_repo_dir() / "file1.cpp";
  const auto file2_path = get_temp_repo_dir() / "file2.cpp";
  REQUIRE(!std::filesystem::exists(file1_path));
  REQUIRE(!std::filesystem::exists(file2_path));
}

TEST_CASE("Parse single uses revparse", "[cpp-lint-action][git2][revparse]") {
  create_temp_repo_dir();
  auto guard = scope_guard{remove_temp_repo_dir};

  const auto files = std::vector<std::string>{"file1.cpp", "file2.cpp"};
  create_temp_files(files, "hello world");

  auto repo                    = init_basic_repo();
  auto [index_oid, index_tree] = git::index::add_files(*repo, files);
  auto commit_oid              = git::commit::create_head(*repo, "Init", *index_tree);

  SECTION("get commit id by head") {
    auto ret = git::revparse::single(*repo, default_branch);
    REQUIRE_FALSE(git::object::id_str(*ret).empty());
  }
}

TEST_CASE("Get HEAD", "[cpp-lint-action][git2][commit]") {
  create_temp_repo_dir();
  auto guard = scope_guard{remove_temp_repo_dir};

  auto repo        = init_basic_repo();
  const auto files = std::vector<std::string>{"file0.cpp", "file1.cpp"};
  create_temp_files(files, "hello world");

  auto [index_oid, index_tree] = git::index::add_files(*repo, files);
  auto commit_oid              = git::commit::create_head(*repo, "Init", *index_tree);
  auto commit                  = git::commit::lookup(*repo, commit_oid);

  auto ref         = git::repo::head(*repo);
  auto head_commit = git::ref::peel<git::commit_ptr>(*ref);
  REQUIRE(git::commit::id_str(*head_commit) == git::commit::id_str(*commit));
  remove_temp_repo_dir();
}

TEST_CASE("Push two commits and get diff files", "[cpp-lint-action][git2][diff]") {
  create_temp_repo_dir();
  auto guard = scope_guard{remove_temp_repo_dir};

  const auto files = std::vector<std::string>{"file1.cpp", "file2.cpp"};
  create_temp_files(files, "hello world");
  auto repo                 = init_basic_repo();
  auto [index_oid1, index1] = git::index::add_files(*repo, files);
  auto commit_oid1          = git::commit::create_head(*repo, "Init", *index1);
  auto commit1              = git::commit::lookup(*repo, commit_oid1);

  auto head_commit = git::repo::head_commit(*repo);
  std::cout << git::commit::id_str(*head_commit) << "\n";
  REQUIRE(git::commit::id_str(*head_commit) == git::commit::id_str(*commit1));

  append_content_to_file("file1.cpp", "hello world2");
  auto [index_oid2, index2] = git::index::add_files(*repo, {"file1.cpp"});
  auto commit_oid2          = git::commit::create_head(*repo, "Two", *index2);
  auto commit2              = git::commit::lookup(*repo, commit_oid2);
  auto head_commit2         = git::repo::head_commit(*repo);
  REQUIRE(git::commit::id_str(*head_commit2) == git::commit::id_str(*commit2));

  auto changed_files = git::diff::changed_files(*repo, "HEAD~1", "HEAD");
  REQUIRE(changed_files.size() == 1);
}

TEST_CASE("Runtime git context tracks added modified deleted and renamed paths",
          "[cpp-lint-action][git2][diff][context]") {
  auto repo = repo_t{};

  repo.add_file("modified.cpp", "int old_value = 0;\n");
  repo.add_file("deleted.cpp", "int removed_value = 0;\n");
  repo.add_file("renamed_old.cpp", "int renamed_value = 0;\n");
  auto target = commit_changes(repo);

  repo.rewrite_file("modified.cpp", "int new_value = 1;\n");
  repo.remove_file("deleted.cpp");
  repo.remove_file("renamed_old.cpp");
  repo.add_file("renamed_new.cpp", "int renamed_value = 0;\n");
  repo.add_file("added.cpp", "int added_value = 0;\n");
  auto source = commit_changes(repo);

  auto context      = runtime_context{};
  context.repo_path = repo.get_path();
  context.target    = target;
  context.source    = source;
  fill_git_info(context);

  REQUIRE(ranges::contains(context.changed_files, "modified.cpp"));
  REQUIRE(ranges::contains(context.changed_files, "deleted.cpp"));
  REQUIRE(ranges::contains(context.changed_files, "renamed_old.cpp"));
  REQUIRE(ranges::contains(context.changed_files, "renamed_new.cpp"));
  REQUIRE(ranges::contains(context.changed_files, "added.cpp"));

  REQUIRE(context.deltas.at("modified.cpp").status == GIT_DELTA_MODIFIED);
  REQUIRE(context.deltas.at("deleted.cpp").status == GIT_DELTA_DELETED);
  REQUIRE(context.deltas.at("renamed_old.cpp").status == GIT_DELTA_DELETED);
  REQUIRE(context.deltas.at("renamed_new.cpp").status == GIT_DELTA_ADDED);
  REQUIRE(context.deltas.at("added.cpp").status == GIT_DELTA_ADDED);
}

TEST_CASE("Simple use of patch ", "[cpp-lint-action][git2][patch]") {
  create_temp_repo_dir();
  auto guard = scope_guard{remove_temp_repo_dir};

  const auto files = std::vector<std::string>{"file1.cpp", "file2.cpp"};
  create_temp_files(files, "hello world");
  auto repo                 = init_basic_repo();
  auto [index_oid1, index1] = git::index::add_files(*repo, files);
  auto commit_oid1          = git::commit::create_head(*repo, "Init", *index1);
  auto commit1              = git::commit::lookup(*repo, commit_oid1);

  auto head_commit = git::repo::head_commit(*repo);
  std::cout << git::commit::id_str(*head_commit) << "\n";
  REQUIRE(git::commit::id_str(*head_commit) == git::commit::id_str(*commit1));

  append_content_to_file("file1.cpp", "hello world2");
  auto [index_oid2, index2] = git::index::add_files(*repo, {"file1.cpp"});
  auto commit_oid2          = git::commit::create_head(*repo, "Two", *index2);
  auto commit2              = git::commit::lookup(*repo, commit_oid2);
  auto head_commit2         = git::repo::head_commit(*repo);
  REQUIRE(git::commit::id_str(*head_commit2) == git::commit::id_str(*commit2));

  auto diff  = git::diff::commit_to_commit(*repo, *commit1, *commit2);
  auto patch = git::patch::create_from_diff(*diff, 0);
}

TEST_CASE("Create patch from buffers", "[cpp-lint-action][git2][patch]") {
  auto old_content = "int n = 2;"s;
  auto new_content = "double n = 2;"s;
  auto opt         = git::diff::init_option();
  auto patch =
    git::patch::create_from_buffers(old_content, "temp.cpp", new_content, "temp.cpp", opt);
  std::cout << git::patch::to_str(*patch);
}

TEST_CASE("Get file content from a specific commit", "[cpp-lint-action][git2][blob]") {
  create_temp_repo_dir();
  auto guard = scope_guard{remove_temp_repo_dir};

  const auto files = std::vector<std::string>{"file1.cpp"};
  create_temp_files(files, "hello world");
  auto repo               = init_basic_repo();
  auto [index_oid, index] = git::index::add_files(*repo, files);
  auto commit_oid         = git::commit::create_head(*repo, "Init", *index);
  auto commit             = git::commit::lookup(*repo, commit_oid);
  auto content            = git::blob::get_raw_content(*repo, *commit, "file1.cpp");
  REQUIRE(content == "hello world");
}

TEST_CASE("Get lines in a hunk", "[cpp-lint-action][git2][patch]") {
  create_temp_repo_dir();
  auto guard = scope_guard{remove_temp_repo_dir};

  const auto files = std::vector<std::string>{"file1.cpp"};
  create_temp_files(files, "hello world\nhello world2\n");
  auto repo                 = init_basic_repo();
  auto [index_oid1, index1] = git::index::add_files(*repo, files);
  auto commit_oid1          = git::commit::create_head(*repo, "Init", *index1);
  auto commit1              = git::commit::lookup(*repo, commit_oid1);

  append_content_to_file("file1.cpp", "hello world3");
  auto [index_oid2, index2] = git::index::add_files(*repo, {"file1.cpp"});
  auto commit_oid2          = git::commit::create_head(*repo, "Two", *index2);
  auto commit2              = git::commit::lookup(*repo, commit_oid2);
  auto head_commit2         = git::repo::head_commit(*repo);

  auto diff     = git::diff::commit_to_commit(*repo, *commit1, *commit2);
  auto patch    = git::patch::create_from_diff(*diff, 0);
  auto [hunk, num_lines] = git::patch::get_hunk(*patch, 0);
  REQUIRE(num_lines > 0);
  REQUIRE(hunk.new_lines > 0);
  REQUIRE(git::hunk::is_row_in_hunk(hunk, hunk.new_start));
  REQUIRE(git::hunk::is_row_in_hunk(hunk, hunk.new_start + hunk.new_lines - 1));
  REQUIRE_FALSE(git::hunk::is_row_in_hunk(hunk, hunk.new_start + hunk.new_lines));

  auto contents = git::patch::get_lines_in_hunk(*patch, 0);
  REQUIRE(contents[0] == "hello world\n");
  REQUIRE(contents[1] == "hello world2\n");
  REQUIRE(contents[2] == "hello world3");
}

TEST_CASE("Compare from buffer", "[cpp-lint-action][git2][patch]") {
  // Compare original content with formatted result of a file.

  std::string before = R"""(
namespace {
intt;
intt1;
intt2;
intt3;
int x = 1.1;
  int y = 1.1;
    int z = 1.1;
}
intu1;
intu2;
intu3;
)""";

  std::string after = R"""(
namespace {
intt;
intt1;
intt2;
intt3;
int x = 1.1;
int y = 1.1;
int z = 1.1;
}
intu1;
intu2;
intu3;
)""";

  auto opts          = git::diff::init_option();
  opts.context_lines = 0;
  auto patch         = git::patch::create_from_buffers(before, "name", after, "name", opts);

  auto num_hunks = git::patch::num_hunks(*patch);
  REQUIRE(num_hunks == 1);

  auto lines = git::patch::get_target_lines_in_hunk(*patch, 0);
  REQUIRE(lines.size() == 2);
}
