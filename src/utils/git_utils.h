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

#include <cstdint>
#include <cstring>
#include <git2/types.h>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <git2.h>

#include <utils/std.h>

/// This is based on v1.8.4

namespace lint::git {
  inline void throw_if(int error, const std::string &msg) {
    if (error < 0) {
      throw std::runtime_error{msg};
    }
  }

  inline void throw_if(int error) {
    if (error < 0) {
      auto *msg = ::git_error_last()->message;
      throw std::runtime_error{msg};
    }
  }

  /// Will throw a GIT_ERROR with given message if condition is true.
  inline void throw_if(bool condition, const std::string &msg) {
    if (condition) {
      throw std::runtime_error{msg};
    }
  }

  /// Will throw a GIT_ERROR with given message if condition isn't true.
  inline void throw_unless(bool condition, const std::string &msg) {
    if (!condition) {
      throw std::runtime_error{msg};
    }
  }

  inline void throw_unsupported() {
    throw std::runtime_error{"unsupported"};
  }

  /// Theses are pointers with RAII mechanism.
  using repo_ptr        = std::unique_ptr<git_repository, decltype(::git_repository_free) *>;
  using config_ptr      = std::unique_ptr<git_config, decltype(::git_config_free) *>;
  using ref_ptr         = std::unique_ptr<git_reference, decltype(::git_reference_free) *>;
  using commit_ptr      = std::unique_ptr<git_commit, decltype(::git_commit_free) *>;
  using diff_ptr        = std::unique_ptr<git_diff, decltype(::git_diff_free) *>;
  using tree_ptr        = std::unique_ptr<git_tree, decltype(::git_tree_free) *>;
  using index_ptr       = std::unique_ptr<git_index, decltype(::git_index_free) *>;
  using blob_ptr        = std::unique_ptr<git_blob, decltype(::git_blob_free) *>;
  using tag_ptr         = std::unique_ptr<git_tag, decltype(::git_tag_free) *>;
  using object_ptr      = std::unique_ptr<git_object, decltype(::git_object_free) *>;
  using signature_ptr   = std::unique_ptr<git_signature, decltype(::git_signature_free) *>;
  using status_list_ptr = std::unique_ptr<git_status_list, decltype(::git_status_list_free) *>;
  using patch_ptr       = std::unique_ptr<git_patch, decltype(::git_patch_free) *>;

  struct time {
    std::int64_t sec;
    int offset;
  };

  struct signature {
    std::string name;
    std::string email;
    time when;
  };

  /// Init the global state.
  /// This must be used before any git operations.
  auto setup() -> int;

  /// Shutdown the global state.
  /// This must be used after all git operations have done.
  auto shutdown() -> int;

  namespace repo {
    /// Creates a new Git repository in the given folder.
    auto init(const std::string &repo_path, bool is_bare) -> repo_ptr;

    /// Open a git repository.
    auto open(const std::string &repo_path) -> repo_ptr;

    /// Determines the status of a git repository - ie, whether an operation
    /// (merge, cherry-pick, etc) is in progress.
    auto state(git_repository &repo) -> int;

    /// Get the path of this repository. This is the path of the .git folder
    /// for normal repositories, or of the repository itself for bare repositories.
    auto path(git_repository &repo) -> std::string;

    /// Check if a repository is empty
    auto is_empty(git_repository &repo) -> bool;

    /// Get the configuration file for this repository.
    /// If a configuration file has not been set, the default config set for
    /// the repository will be returned, including global and system configurations
    /// (if they are available).
    auto config(git_repository &repo) -> config_ptr;

    /// Get a snapshot of the repository's configuration.
    /// Convenience function to take a snapshot from the repository's
    /// configuration. The contents of this snapshot will not change, even if the
    /// underlying config files are modified.
    auto config_snapshot(git_repository &repo) -> config_ptr;

    /// Get the Index file for this repository. If a custom index has not been
    /// set, the default index for the repository will be returned (the one located
    /// in .git/index).
    auto index(git_repository &repo) -> index_ptr;

    /// Retrieve and resolve the reference pointed at by HEAD.
    /// This function may return a nullpointer when HEAD is missing or HEAD pointers
    /// to a non-exists reference.
    auto head(git_repository &repo) -> ref_ptr;

    /// Get the head commit.
    /// This function may return a nullpointer when HEAD is missing or HEAD pointers
    /// to a non-exists reference.
    auto head_commit(git_repository &repo) -> commit_ptr;
  } // namespace repo

  namespace config {
    /// Get the value of a string config variable.
    /// This function can only be used on snapshot config objects. The string
    /// is owned by the config and should not be freed by the user. The pointer
    /// will be valid until the config is freed. All config files will be looked
    /// into, in the order of their defined level. A higher level means a higher
    /// priority. The first occurrence of the variable will be returned here.
    auto get_string(const git_config &cfg, const std::string &key) -> std::string;

    /// Get the value of a boolean config variable.
    /// This function uses the usual C convention of 0 being false and anything
    /// else true. All config files will be looked into, in the order of their
    /// defined level. A higher level means a higher priority. The first occurrence
    /// of the variable will be returned here.
    auto get_bool(const git_config &cfg, const std::string &key) -> bool;

    /// Get the value of an integer config variable.
    /// All config files will be looked into, in the order of their defined
    /// level. A higher level means a higher priority. The first occurrence of the
    /// variable will be returned here.
    auto get_int32(const git_config &cfg, const std::string &key) -> int32_t;

    /// Get the value of a long integer config variable.
    /// All config files will be looked into, in the order of their defined
    /// level. A higher level means a higher priority. The first occurrence of the
    /// variable will be returned here.
    auto get_int64(const git_config &cfg, const std::string &key) -> int64_t;

    /// Set the value of a string config variable in the config file with the
    /// highest level (usually the local one).
    void set_string(git_config &cfg, const std::string &key, const std::string &value);

    /// Set the value of a boolean config variable in the config file with the
    /// highest level (usually the local one).
    void set_bool(git_config &cfg, const std::string &key, bool value);

    /// Set the value of an integer config variable in the config file with the
    /// highest level (usually the local one).
    void set_int32(git_config &cfg, const std::string &key, int32_t value);

    /// Set the value of a long integer config variable in the config file with
    /// the highest level (usually the local one).
    void set_int64(git_config &cfg, const std::string &key, int64_t value);

    /// Create a snapshot of the configuration
    /// Create a snapshot of the current state of a configuration, which allows
    /// you to look into a consistent view of the configuration for looking up
    /// complex values (e.g. a remote, submodule). The string returned when
    /// querying such a config object is valid until it is freed.
    /// All the modification will be wrote back into configuration file when
    /// you use git_config while snapshot won't.
    auto snapshot(git_config &cfg) -> config_ptr;

  } // namespace config

  namespace branch {
    /// Create a new branch pointing at a target commit.
    /// A new direct reference will be created pointing to this target commit.
    /// If force is true and a reference already exists with the given name, it'll
    /// be replaced.
    auto create(git_repository &repo,
                const std::string &branch_name,
                const git_commit &target,
                bool force) -> ref_ptr;

    /// Delete an existing branch reference.
    void del(git_reference &branch);

    /// @brief Get the branch name
    /// @return Pointer to the abbreviated reference name. Owned by ref, do not
    /// free. https://libgit2.org/libgit2/#HEAD/group/branch/git_branch_name
    auto name(const git_reference &ref) -> std::string;

    /// @brief Determine if HEAD points to the given branch
    /// @link https://libgit2.org/libgit2/#HEAD/group/branch/git_branch_is_head
    bool is_head(const git_reference &branch);

    /// Lookup a branch by its name in a repository.
    auto lookup(git_repository &repo, const std::string &name, git_branch_t branch_type) -> ref_ptr;

    /// Get the current branch name indicated by HEAD reference.
    /// If HEAD is missing or HEAD pointers to an non-exist reference, this will
    /// return an empty string.
    auto current_name(git_repository &repo) -> std::string;
  } // namespace branch

  namespace commit {
    /// Create a commit object from the given buffer and signature
    /// Given the unsigned commit object's contents, its signature and the
    /// header field in which to store the signature, attach the signature to the
    /// commit and write it into the given repository.
    auto create_with_signature(git_repository &repo, const std::string &commit_content) -> git_oid;

    /// Create new commit in the repository from a list of git_object pointers.
    /// @param repo Repository where to store the commit
    /// @param update_ref If not NULL, name of the reference that will be
    /// updated to point to this commit. If the reference is not direct, it will be
    /// resolved to a direct reference. Use "HEAD" to update the HEAD of the
    /// current branch and make it point to this commit. If the reference doesn't
    /// exist yet, it will be created. If it does exist, the first parent must be
    /// the tip of this branch.
    /// @param author Signature with author and author time of commit
    /// @param committer Signature with committer and * commit time of commit
    /// @param message Full message for this commit
    /// @param tree An instance of a `git_tree` object that will be used as the
    /// tree for the commit. This tree object must also be owned by the given
    /// `repo`.
    /// @param parent_count Number of parents for this commit
    /// @param parents Array of `parent_count` pointers to `git_commit` objects
    /// that will be used as the parents for this commit. This array may be NULL if
    /// `parent_count` is 0 (root commit). All the given commits must be owned by
    /// the `repo`.
    auto create(
      git_repository &repo,
      const std::string &updated_ref,
      const git_signature &author,
      const git_signature &committer,
      const std::string &message,
      const git_tree &tree,
      std::vector<const git_commit *> parents) -> git_oid;

    /// Create a new commit and set it to HEAD with default signature.
    auto create_head(git_repository &repo, const std::string &message, const git_tree &index_tree)
      -> git_oid;

    /// Get the tree pointed to by a commit.
    auto tree(const git_commit &commit) -> tree_ptr;

    /// Get the id of the tree pointed to by a commit. This differs from
    /// git_commit_tree in that no attempts are made to fetch an object from the
    /// ODB.
    auto tree_id(const git_commit &commit) -> git_oid;

    /// Lookup a commit object from a repository.
    auto lookup(git_repository &repo, const git_oid &id) -> commit_ptr;

    /// Get the author of a commit.
    /// https://libgit2.org/libgit2/#v0.20.0/group/commit/git_commit_author
    auto author(const git_commit &commit) -> signature;

    /// Get the committer of a commit.
    /// https://libgit2.org/libgit2/#v0.20.0/group/commit/git_commit_committer
    auto committer(const git_commit &commit) -> signature;

    /// Get the commit time (i.e. committer time) of a commit.
    /// https://libgit2.org/libgit2/#v0.20.0/group/commit/git_commit_time
    auto time(const git_commit &commit) -> int64_t;

    /// Get the full message of a commit.
    auto message(const git_commit &commit) -> std::string;

    /// Get the commit object that is the <n
    auto nth_gen_ancestor(const git_commit &commit, std::uint32_t n) -> commit_ptr;

    /// Get the specified parent of the commit.
    auto parent(const git_commit &commit, std::uint32_t n) -> commit_ptr;

    /// Get the oid of a specified parent for a commit. This is different from
    /// git_commit_parent, which will attempt to load the parent commit from the
    /// ODB.
    auto parent_id(const git_commit &commit, std::uint32_t n) -> git_oid;

    /// Get the number of parents of this commit
    auto parent_count(const git_commit &commit) -> std::uint32_t;

    /// Get this commit's id and convert it to string.
    auto id_str(const git_commit &commit) -> std::string;
  } // namespace commit

  namespace diff {
    /// Create a diff between the repository index and the workdir directory.
    auto index_to_workdir(git_repository &repo, git_index &index, const git_diff_options &opts)
      -> diff_ptr;

    /// Create a diff with the difference between two tree objects.
    auto tree_to_tree(
      git_repository &repo,
      git_tree &old_tree,
      git_tree &new_tree,
      const git_diff_options &opts) -> diff_ptr;

    /// Create a diff with the difference between two commits.
    auto commit_to_commit(git_repository &repo, git_commit &commit1, git_commit &commit2)
      -> diff_ptr;

    /// An utility to get diff.
    auto get(git_repository &repo, git_commit &commit1, git_commit &commit2) -> diff_ptr;

    /// Initialize diff options structure
    auto init_option() -> git_diff_options;

    /// Query how many diff records are there in a diff.
    auto num_deltas(git_diff &diff) -> std::size_t;

    /// Return the diff delta for an entry in the diff list.
    auto get_delta(const git_diff &diff, size_t idx) -> const git_diff_delta *;

    /// A simple implmentation which uses for_each to get diff delta details.
    // auto deltas(git_diff& diff) -> std::vector<diff_delta_detail>;
    auto deltas(git_diff &diff) -> std::unordered_map<std::string, git_diff_delta>;

    /// A simple implmentation which compares ref1 with ref2's differences.
    auto deltas(git_repository &repo, const std::string &spec1, const std::string &spec2)
      -> std::unordered_map<std::string, git_diff_delta>;

    /// Get changed files between two specs. The spec could be a reference,
    /// branch, tag or commid id. return The modified and new added file names
    /// in source reference.
    auto changed_files(git_repository &repo, const std::string &spec1, const std::string &spec2)
      -> std::vector<std::string>;

    /// Get changed files by deltas.
    auto changed_files(const std::unordered_map<std::string, git_diff_delta> &deltas)
      -> std::vector<std::string>;

    /// Produce the complete formatted text output from a diff into a buffer.
    auto to_str(git_diff &diff, git_diff_format_t format) -> std::string;

  } // namespace diff

  namespace oid {
    /// Format a git_oid into a buffer as a hex format c-string.
    auto to_str(const git_oid &oid) -> std::string;

    /// Compare two oid structures for equality
    auto equal(const git_oid &o1, const git_oid &o2) -> bool;

    /// Parse a hex formatted object id into a git_oid.
    auto from_str(const std::string &str) -> git_oid;
  } // namespace oid

  namespace ref {
    /// Get the type of a reference.
    auto type(const git_reference &ref) -> git_reference_t;

    /// Check if a reference is a local branch. That's to say, the reference
    /// lives in the refs/heads namespace.
    auto is_branch(git_reference &ref) -> bool;

    /// Check if a reference is a remote tracking branch
    auto is_remote(git_reference &ref) -> bool;

    /// Check if a reference is a tag
    auto is_tag(git_reference &ref) -> bool;

    /// Free the given reference.
    void free(git_reference &ref);

    /// Get the full name of a reference. E.g. refs/heads/main
    auto name(const git_reference &ref) -> std::string;

    /// Lookup a reference by name in a repository.
    auto lookup(git_repository &repo, const std::string &name) -> ref_ptr;

    /// Lookup a reference by name and resolve immediately to OID.
    auto name_to_oid(git_repository &repo, const std::string &name) -> git_oid;

    /// Get the reference's short name. This will transform the reference name
    /// into a name "human-readable" version. If no shortname is appropriate, it
    /// will return the full name.
    auto shorthand(const git_reference &ref) -> std::string;

    /// Resolve a symbolic reference to a direct reference.
    /// If a direct reference is passed as an argument, a copy of that
    /// reference is returned.
    auto resolve(const git_reference &symbolic_ref) -> ref_ptr;

    /// Recursively peel reference until object of the specified type is found.
    /// If you pass GIT_OBJECT_ANY as the target type, then the object will be
    /// peeled until a non-tag object is met.
    auto peel(const git_reference &ref, git_object_t obj_type) -> object_ptr;

    /// Like peel(git_reference&, object_t) but automatically convert to given type.
    template <class T>
    auto peel(const git_reference &ref) -> T {
      if constexpr (std::same_as<T, commit_ptr>) {
        auto obj = peel(ref, git_object_t::GIT_OBJECT_COMMIT);
        if (obj == nullptr) {
          return {nullptr, ::git_commit_free};
        }
        auto *raw = obj.release();
        auto ret  = commit_ptr{reinterpret_cast<git_commit *>(raw), ::git_commit_free};
        return ret;
      }
      throw std::runtime_error{"unsupported"};
    }

  } // namespace ref

  namespace revparse {
    /// Find a single object, as specified by a revision string.
    /// https://git-scm.com/docs/git-rev-parse.html#_specifying_revisions
    auto single(git_repository &repo, const std::string &spec) -> object_ptr;

    /// An utility to get commit point by revision string.
    auto commit(git_repository &repo, const std::string &spec) -> commit_ptr;

    /// Find a complete sha based on given short sha
    auto complete_sha(git_repository &repo, const std::string &short_sha) -> std::string;

  }; // namespace revparse

  namespace object {
    /// Close an open object
    void free(git_object &obj);

    /// Get the object type of an object
    auto type(const git_object &obj) -> git_object_t;

    /// Get the id (SHA1) of a repository object
    auto id(const git_object &obj) -> git_oid;

    /// Get the id (SHA1) string of given repository object.
    auto id_str(const git_object &obj) -> std::string;

    /// Lookup a reference to one of the objects in a repository.
    auto lookup(git_repository &repo, const git_oid &oid, git_object_t type) -> object_ptr;

  } // namespace object

  template <typename T>
  auto convert(git_object &obj) -> T {
    auto type = object::type(obj);
    if constexpr (std::same_as<T, git_commit &> || std::same_as<T, const git_commit &>) {
      throw_if(type != git_object_t::GIT_OBJECT_COMMIT,
               "The given object isn't git_commit& or const git_commit&");
      return reinterpret_cast<T>(obj);
    }
    if constexpr (std::same_as<T, git_tree &> || std::same_as<T, const git_tree &>) {
      throw_if(type != git_object_t::GIT_OBJECT_TREE,
               "The given object isn't git_tree& or const git_tree&");
      return reinterpret_cast<T>(obj);
    }
    if constexpr (std::same_as<T, git_blob &> || std::same_as<T, const git_blob &>) {
      throw_if(type != git_object_t::GIT_OBJECT_BLOB,
               "The given object isn't blob_raw_ptr or const git_blob&");
      return reinterpret_cast<T>(obj);
    }
    if constexpr (std::same_as<T, git_tag &> || std::same_as<T, const git_tag &>) {
      throw_if(type != git_object_t::GIT_OBJECT_TAG,
               "The given object isn't tag_raw_ptr or tag_raw_cptr");
      return reinterpret_cast<T>(obj);
    }
    throw_unsupported();
    unreachable(); // Compiler can't well infer the above exception.
  }

  template <typename T>
  auto convert(object_ptr obj) -> T {
    auto *raw = obj.release();
    if constexpr (std::same_as<T, commit_ptr>) {
      auto &ptr = convert<git_commit &>(*raw);
      return {&ptr, ::git_commit_free};
    }
    if constexpr (std::same_as<T, tree_ptr>) {
      auto &ptr = convert<git_tree &>(*raw);
      return {ptr, ::git_tree_free};
    }
    if constexpr (std::same_as<T, blob_ptr>) {
      auto &ptr = convert<git_blob &>(*raw);
      return {ptr, ::git_blob_free};
    }
    if constexpr (std::same_as<T, tag_ptr>) {
      auto &ptr = convert<git_tag &>(*raw);
      return {ptr, ::git_tag_free};
    }
  }

  namespace sig {
    /// This looks up the user.name and user.email from the configuration and
    /// uses the current time as the timestamp, and creates a new signature based
    /// on that information. Either the user.name or user.email must be set.
    auto create_default(git_repository &repo) -> signature_ptr;

  } // namespace sig

  namespace index {
    ///  Write an existing index object from memory back to disk using an atomic
    ///  file lock.
    void write(git_index &index);

    /// Write the index as a tree.
    /// This method will scan the index and write a representation of its
    /// current state back to disk; it recursively creates tree objects for each of
    /// the subtrees stored in the index, but only returns the OID of the root
    /// tree. This is the OID that can be used e.g. to create a commit. The index
    /// instance cannot be bare, and needs to be associated to an existing
    /// repository. The index must not contain any file in conflict.
    [[nodiscard]] auto write_tree(git_index &index) -> git_oid;

    /// Add or update an index entry from a file on disk
    /// The file path must be relative to the repository's working folder and
    /// must be readable.
    /// This method will fail in bare index instances.
    /// This forces the file to be added to the index, not looking at gitignore
    /// rules. Those rules can be evaluated through the git_status APIs (in
    /// status.h) before calling this.
    /// If this file currently is the result of a merge conflict, this file
    /// will no longer be marked as conflicting. The data about the conflict will
    /// be moved to the "resolve undo" (REUC) section
    void add_by_path(git_index &index, const std::string &path);

    /// Remove an index entry corresponding to a file on disk
    /// The file path must be relative to the repository's working folder. It may
    /// exist. If this file currently is the result of a merge conflict, this file
    /// will no longer be marked as conflicting. The data about the conflict will be
    /// moved to the "resolve undo" (REUC) section.
    void remove_by_path(git_index &index, const std::string &path);

    /// A utility to forcely and fastly add all files to staging area.
    /// This function will automatically call write tree to enable this changes.
    auto add_files(git_repository &repo, const std::vector<std::string> &files)
      -> std::tuple<git_oid, tree_ptr>;

    /// A utility to forcely and fastly add remove files to staging area.
    /// This function will automatically call write tree to enable this changes.
    auto remove_files(git_repository &repo,
                      const std::string &repo_path,
                      const std::vector<std::string> &files) -> std::tuple<git_oid, tree_ptr>;

  } // namespace index

  namespace tree {
    /// Lookup a tree object from the repository.
    auto lookup(git_repository &repo, const git_oid &oid) -> tree_ptr;

    /// Get the id of the object pointed by the entry
    auto entry_id(const git_tree_entry &entry) -> git_oid;

    /// Lookup a tree entry by its filename. Return nullptr if not found.
    auto entry_byname(const git_tree &tree, const std::string &filename)
      -> std::tuple<git_oid, const git_tree_entry *>;

  } // namespace tree

  namespace status {
    /// Gather file status information and populate the git_status_list.
    /// Note that if a pathspec is given in the git_status_options to filter
    /// the status, then the results from rename detection (if you enable it) may
    /// not be accurate. To do rename detection properly, this must be called with
    /// no pathspec so that all files can be considered.
    auto gather(git_repository &repo, const git_status_options &options) -> status_list_ptr;

    /// Gets the count of status entries in this list. If there are no changes
    /// in status (at least according the options given when the status list was
    /// created), this can return 0.
    auto entry_count(git_status_list &status_list) -> std::size_t;

    /// Get the default options.
    auto default_options() -> git_status_options;

    /// Get a pointer to one of the entries in the status list.
    /// No need to free this pointer
    auto get_by_index(git_status_list &status_list, std::size_t idx) -> const git_status_entry *;
  } // namespace status

  namespace patch {
    /// Return a specific patch for an entry in the diff.
    auto create_from_diff(git_diff &diff, std::size_t idx) -> patch_ptr;

    /// Return all patches.
    auto create_from_diff(git_diff &diff) -> std::unordered_map<std::string, patch_ptr>;

    /// Directly generate a patch from the difference between two buffers.
    auto create_from_buffers(
      const std::string &old_buffer,
      const std::string &old_as_path,
      const std::string &new_buffer,
      const std::string &new_as_path,
      const git_diff_options &opts) -> patch_ptr;

    /// Get changed files.
    auto changed_files(const std::unordered_map<std::string, patch_ptr> &patches)
      -> std::vector<std::string>;

    /// Get the content of a patch as a single diff text.
    auto to_str(git_patch &patch) -> std::string;

    /// Get the delta associated with a patch. This delta points to internal
    /// data and you do not have to release it when you are done with it.
    auto get_delta(const git_patch &patch) -> const git_diff_delta *;

    /// Get the delta associated with a patch. This delta points to internal
    /// data and you do not have to release it when you are done with it.
    auto num_hunks(const git_patch &patch) -> std::size_t;

    /// Get the information about a hunk in a patch
    /// Given a patch and a hunk index into the patch, this returns detailed
    /// information about that hunk.
    auto get_hunk(git_patch &patch, std::size_t hunk_idx) -> std::tuple<git_diff_hunk, std::size_t>;

    /// Get the number of lines in a hunk.
    auto num_lines_in_hunk(const git_patch &patch, std::size_t hunk_idx) -> std::size_t;

    /// Get data about a line in a hunk of a patch.
    /// Due to libgit2 limitation, patch can't be const qualified.
    auto get_line_in_hunk(git_patch &patch, std::size_t hunk_idx, std::size_t line_idx)
      -> git_diff_line;

    /// A utility to get all lines in a hunk.
    /// Due to libgit2 limitation, patch can't be const qualified.
    auto get_lines_in_hunk(git_patch &patch, std::size_t hunk_idx) -> std::vector<std::string>;

    auto get_target_lines_in_hunk(git_patch &patch, std::size_t hunk_idx)
      -> std::vector<std::string>;

    auto get_source_lines_in_hunk(git_patch &patch, std::size_t hunk_idx)
      -> std::vector<std::string>;

  } // namespace patch

  namespace hunk {
    auto is_old_line(const git_diff_line &line) -> bool;

    auto is_new_line(const git_diff_line &line) -> bool;

    auto get_line_content(const git_diff_line &line) -> std::string;

    auto get_old_line_number(const git_diff_line &line) -> std::optional<std::size_t>;

    auto get_new_line_number(const git_diff_line &line) -> std::optional<std::size_t>;

    bool is_row_in_hunk(const git_diff_hunk &hunk, int row_number) noexcept;
  } // namespace hunk

  namespace blob {
    /// Lookup a blob object from a repository.
    auto lookup(git_repository &repo, const git_oid &oid) -> blob_ptr;

    /// Get a buffer with the raw content of a blob.
    auto get_raw_content(const git_blob &blob) -> std::string;

    /// A utility to get raw content by file name. Return empty if file not found.
    auto get_raw_content(git_repository &repo, const git_tree &tree, const std::string &file_name)
      -> std::string;

    /// A utility to get raw content by file name. Return empty if file not found.
    auto get_raw_content(git_repository &repo,
                         const git_commit &commit,
                         const std::string &file_name) -> std::string;

  } // namespace blob
} // namespace lint::git
