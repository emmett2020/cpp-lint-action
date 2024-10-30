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
#include "git_utils.h"

#include <cassert>
#include <cstring>
#include <filesystem>
#include <git2/buffer.h>
#include <git2/diff.h>
#include <git2/patch.h>
#include <git2/tree.h>
#include <string>

#include <spdlog/spdlog.h>

namespace lint::git {
  namespace {
    inline auto make_str(const char *p, std::size_t len) -> std::string {
      return {p, len};
    }

    auto convert_to_signature(const git_signature &sig) -> signature {
      auto res  = signature{};
      res.name  = sig.name;
      res.email = sig.email;
      res.when  = {.sec = sig.when.time, .offset = sig.when.offset};
      return res;
    }
  } // namespace

  int setup() {
    return ::git_libgit2_init();
  }

  auto shutdown() -> int {
    return ::git_libgit2_shutdown();
  }

  // auto delta_status_t_str(delta_status_t status) -> std::string {
  //   switch (status) {
  //   case delta_status_t::unmodified : return "unmodified";
  //   case delta_status_t::added      : return "added";
  //   case delta_status_t::deleted    : return "deleted";
  //   case delta_status_t::modified   : return "modified";
  //   case delta_status_t::renamed    : return "renamed";
  //   case delta_status_t::copied     : return "copied";
  //   case delta_status_t::ignored    : return "ignored";
  //   case delta_status_t::untracked  : return "untracked";
  //   case delta_status_t::type_change: return "type_change";
  //   case delta_status_t::unreadable : return "unreadable";
  //   case delta_status_t::conflicted : return "conflicted";
  //   case delta_status_t::unknown    : return "unknown";
  //   }
  //   return "unknown";
  // }

  // auto file_mode_t_str(file_mode_t mode) -> std::string {
  //   switch (mode) {
  //   case file_mode_t::unreadable     : return "unreadable";
  //   case file_mode_t::tree           : return "tree";
  //   case file_mode_t::blob           : return "blob";
  //   case file_mode_t::blob_executable: return "blob_executable";
  //   case file_mode_t::link           : return "link";
  //   case file_mode_t::commit         : return "commit";
  //   case file_mode_t::unknown        : return "unknown";
  //   }
  //   return "unknown";
  // }
  //
  // auto file_flag_t_str(std::uint32_t flags) -> std::string {
  //   auto res    = std::string{};
  //   auto concat = [&](std::uint32_t exactor, std::string_view msg) {
  //     if ((flags & exactor) != 0U) {
  //       if (res.empty()) {
  //         res.append(msg);
  //       } else {
  //         res.append(", ").append(msg);
  //       }
  //     }
  //   };
  //   concat(0b0000'0000, "binary");
  //   concat(0b0000'0010, "not_binary");
  //   concat(0b0000'0100, "valid_id");
  //   concat(0b0000'1000, "exists");
  //   concat(0b0001'0000, "valid_size");
  //   return res;
  // }
  //
  // auto diff_line_t_str(diff_line_t tp) -> std::string {
  //   switch (tp) {
  //   case diff_line_t::context      : return "context";
  //   case diff_line_t::addition     : return "addition";
  //   case diff_line_t::deletion     : return "deletion";
  //   case diff_line_t::context_eofnl: return "context_eofnl";
  //   case diff_line_t::add_eofnl    : return "add_eofnl";
  //   case diff_line_t::del_eofnl    : return "del_eofnl";
  //   case diff_line_t::file_hdr     : return "file_hdr";
  //   case diff_line_t::hunk_hdr     : return "hunk_hdr";
  //   case diff_line_t::binary       : return "binary";
  //   case diff_line_t::unknown      : return "unknown";
  //   }
  //   return "unknown";
  // }

  // auto ref_t_str(ref_t tp) -> std::string {
  //   switch (tp) {
  //   case ref_t::direct  : return "direct";
  //   case ref_t::symbolic: return "symbolic";
  //   case ref_t::all     : return "all";
  //   default             : return "invalid";
  //   }
  //   return "invalid";
  // }
  //
  // auto branch_t_str(branch_t tp) -> std::string {
  //   switch (tp) {
  //   case branch_t::local : return "local";
  //   case branch_t::remote: return "remote";
  //   case branch_t::all   : return "all";
  //   }
  //   return "unknown";
  // }
  //
  // auto object_t_str(object_t tp) -> std::string {
  //   switch (tp) {
  //   case object_t::any   : return "any";
  //   case object_t::commit: return "commit";
  //   case object_t::bad   : return "bad";
  //   case object_t::tag   : return "tag";
  //   case object_t::blob  : return "blob";
  //   case object_t::tree  : return "tree";
  //   }
  //   return "unknown";
  // }
  //
  // auto repo_state_str(repo_state_t state) -> std::string {
  //   switch (state) {
  //   case repo_state_t::none                   : return "none";
  //   case repo_state_t::merge                  : return "merge";
  //   case repo_state_t::revert                 : return "revert";
  //   case repo_state_t::revert_sequence        : return "revert_sequence";
  //   case repo_state_t::cherrypick             : return "cherrypick";
  //   case repo_state_t::cherrypick_sequence    : return "cherrypick_sequence";
  //   case repo_state_t::bisect                 : return "bisect";
  //   case repo_state_t::rebase                 : return "rebase";
  //   case repo_state_t::rebase_interactive     : return "rebase_interactive";
  //   case repo_state_t::rebase_merge           : return "rebase_merge";
  //   case repo_state_t::apply_mailbox          : return "apply_mailbox";
  //   case repo_state_t::apply_mailbox_or_rebase: return "apply_mailbox_or_rebase";
  //   }
  //   return "unknown";
  // }

  // auto convert_to_signature(const git_signature &sig_ptr) -> signature {
  //   auto sig  = signature{};
  //   sig.name  = sig_ptr->name;
  //   sig.email = sig_ptr->email;
  //   sig.when  = {.sec = sig_ptr->when.time, .offset = sig_ptr->when.offset};
  //   return sig;
  // }
  //

  namespace repo {
    auto init(const std::string &repo_path, bool is_bare) -> repo_ptr {
      auto *repo = static_cast<git_repository *>(nullptr);
      auto bare  = static_cast<unsigned int>(is_bare);
      auto ret   = ::git_repository_init(&repo, repo_path.c_str(), bare);
      throw_if(ret);
      return {repo, ::git_repository_free};
    }

    auto open(const std::string &repo_path) -> repo_ptr {
      auto *repo = static_cast<git_repository *>(nullptr);
      auto ret   = ::git_repository_open(&repo, repo_path.c_str());
      throw_if(ret);
      return {repo, ::git_repository_free};
    }

    auto state(git_repository &repo) -> int {
      return ::git_repository_state(&repo);
    }

    auto path(git_repository &repo) -> std::string {
      const auto *ret = ::git_repository_path(&repo);
      throw_if(ret == nullptr, "can't get repository path");
      return ret;
    }

    bool is_empty(git_repository &repo) {
      auto ret = ::git_repository_is_empty(&repo);
      throw_if(ret);
      return ret == 1;
    }

    auto config(git_repository &repo) -> config_ptr {
      auto *cfg = static_cast<git_config *>(nullptr);
      auto ret  = ::git_repository_config(&cfg, &repo);
      throw_if(ret);
      return {cfg, ::git_config_free};
    }

    auto config_snapshot(git_repository &repo) -> config_ptr {
      auto *cfg = static_cast<git_config *>(nullptr);
      auto ret  = ::git_repository_config_snapshot(&cfg, &repo);
      throw_if(ret);
      return {cfg, ::git_config_free};
    }

    auto index(git_repository &repo) -> index_ptr {
      auto *idx = static_cast<git_index *>(nullptr);
      auto ret  = ::git_repository_index(&idx, &repo);
      throw_if(ret);
      return {idx, ::git_index_free};
    }

    auto head(git_repository &repo) -> ref_ptr {
      auto *ref = static_cast<git_reference *>(nullptr);
      auto ret  = ::git_repository_head(&ref, &repo);
      if (ret == GIT_EUNBORNBRANCH || ret == GIT_ENOTFOUND) {
        return {nullptr, ::git_reference_free};
      }
      throw_if(ret);
      return {ref, ::git_reference_free};
    }

    auto head_commit(git_repository &repo) -> commit_ptr {
      auto head_ref = head(repo);
      if (head_ref == nullptr) {
        return {nullptr, ::git_commit_free};
      }
      auto obj = git::ref::peel<commit_ptr>(*head_ref);
      return obj;
    }

  } // namespace repo

  namespace config {
    auto get_string(const git_config &cfg, const std::string &key) -> std::string {
      const char *value = nullptr;

      auto ret = ::git_config_get_string(&value, &cfg, key.c_str());
      throw_if(ret);
      return value;
    }

    auto get_bool(const git_config &cfg, const std::string &key) -> bool {
      auto value = 0;
      auto ret   = ::git_config_get_bool(&value, &cfg, key.c_str());
      throw_if(ret);
      return value == 1;
    }

    auto get_int32(const git_config &cfg, const std::string &key) -> int32_t {
      auto value = 0;
      auto ret   = ::git_config_get_int32(&value, &cfg, key.c_str());
      throw_if(ret);
      return value;
    }

    auto get_int64(const git_config &cfg, const std::string &key) -> int64_t {
      auto value = int64_t{0};
      auto ret   = ::git_config_get_int64(&value, &cfg, key.c_str());
      throw_if(ret);
      return value;
    }

    void set_string(git_config &cfg, const std::string &key, const std::string &value) {
      auto ret = ::git_config_set_string(&cfg, key.c_str(), value.c_str());
      throw_if(ret);
    }

    void set_bool(git_config &cfg, const std::string &key, bool value) {
      auto ret = ::git_config_set_bool(&cfg, key.c_str(), static_cast<int>(value));
      throw_if(ret);
    }

    void set_int32(git_config &cfg, const std::string &key, int32_t value) {
      auto ret = ::git_config_set_int32(&cfg, key.c_str(), value);
      throw_if(ret);
    }

    void set_int64(git_config &cfg, const std::string &key, int64_t value) {
      auto ret = ::git_config_set_int64(&cfg, key.c_str(), value);
      throw_if(ret);
    }

    auto snapshot(git_config &cfg) -> config_ptr {
      auto *out = static_cast<git_config *>(nullptr);
      auto ret  = ::git_config_snapshot(&out, &cfg);
      throw_if(ret);
      return {out, ::git_config_free};
    }
  } // namespace config

  namespace branch {
    auto create(git_repository &repo,
                const std::string &branch_name,
                const git_commit &target,
                bool force) -> ref_ptr {
      auto *ptr = static_cast<git_reference *>(nullptr);
      auto ret =
        ::git_branch_create(&ptr, &repo, branch_name.c_str(), &target, static_cast<int>(force));
      throw_if(ret);
      return {ptr, ::git_reference_free};
    }

    void del(git_reference &branch) {
      auto ret = ::git_branch_delete(&branch);
      throw_if(ret);
    }

    auto name(const git_reference &ref) -> std::string {
      const char *name = nullptr;
      auto ret         = ::git_branch_name(&name, &ref);
      throw_if(ret);
      return name;
    }

    bool is_head(const git_reference &branch) {
      auto ret = ::git_branch_is_head(&branch);
      throw_if(ret);
      return ret == 1;
    }

    auto lookup(git_repository &repo, const std::string &name, git_branch_t branch_type)
      -> ref_ptr {
      auto *res = static_cast<git_reference *>(nullptr);
      auto ret  = ::git_branch_lookup(&res, &repo, name.c_str(), branch_type);
      throw_if(ret);
      return {res, ::git_reference_free};
    }

    auto current_name(git_repository &repo) -> std::string {
      auto head_ref = repo::head(repo);
      if (head_ref == nullptr) {
        return "";
      }
      auto ref_type = ref::type(*head_ref);
      throw_if(ref_type == git_reference_t::GIT_REFERENCE_INVALID,
               "get current branch failed since invalid ref type");

      if (ref_type == git_reference_t::GIT_REFERENCE_SYMBOLIC) {
        auto direct_ref = ref::resolve(*head_ref);
        return ref::shorthand(*direct_ref);
      }
      return ref::shorthand(*head_ref);
    }

  } // namespace branch

  namespace commit {
    auto create(
      git_repository &repo,
      const std::string &updated_ref,
      const git_signature &author,
      const git_signature &committer,
      const std::string &message,
      const git_tree &tree,
      std::vector<const git_commit *> parents) -> git_oid {
      auto id  = git_oid{};
      auto ret = ::git_commit_create(
        &id,
        &repo,
        updated_ref.c_str(),
        &author,
        &committer,
        "UTF-8",
        message.c_str(),
        &tree,
        parents.size(),
        parents.data());
      throw_if(ret);
      return id;
    }

    auto create_head(git_repository &repo, const std::string &message, const git_tree &index_tree)
      -> git_oid {
      auto sig         = git::sig::create_default(repo);
      auto head_commit = repo::head_commit(repo);

      if (head_commit == nullptr) {
        return commit::create(repo, "HEAD", *sig, *sig, message, index_tree, {});
      }
      return create(repo, "HEAD", *sig, *sig, message, index_tree, {head_commit.get()});
    }

    auto tree(const git_commit &commit) -> tree_ptr {
      auto *ptr = static_cast<git_tree *>(nullptr);
      auto ret  = ::git_commit_tree(&ptr, &commit);
      throw_if(ret);
      return {ptr, ::git_tree_free};
    }

    auto tree_id(const git_commit &commit) -> git_oid {
      const auto *ret = ::git_commit_tree_id(&commit);
      return *ret;
    }

    auto lookup(git_repository &repo, const git_oid &id) -> commit_ptr {
      auto *ptr = static_cast<git_commit *>(nullptr);
      auto ret  = ::git_commit_lookup(&ptr, &repo, &id);
      throw_if(ret);
      return {ptr, ::git_commit_free};
    }

    auto author(const git_commit &commit) -> signature {
      const auto *sig_ptr = ::git_commit_author(&commit);
      throw_if(sig_ptr == nullptr, "The returned git_signature pointer is null");
      auto sig = convert_to_signature(*sig_ptr);
      return sig;
    }

    auto committer(const git_commit &commit) -> signature {
      const auto *sig_ptr = ::git_commit_committer(&commit);
      throw_if(sig_ptr == nullptr, "The returned git_signature pointer is null");
      auto sig = convert_to_signature(*sig_ptr);
      return sig;
    }

    auto time(const git_commit &commit) -> int64_t {
      auto time = ::git_commit_time(&commit);
      return time;
    }

    auto message(const git_commit &commit) -> std::string {
      const auto *ret = ::git_commit_message(&commit);
      throw_if(ret == nullptr, "get commit message error");
      return ret;
    }

    auto nth_gen_ancestor(const git_commit &commit, std::uint32_t n) -> commit_ptr {
      auto *out = static_cast<git_commit *>(nullptr);
      auto ret  = ::git_commit_nth_gen_ancestor(&out, &commit, n);
      throw_if(ret);
      return {out, ::git_commit_free};
    }

    auto parent(const git_commit &commit, std::uint32_t n) -> commit_ptr {
      auto *out = static_cast<git_commit *>(nullptr);
      auto ret  = ::git_commit_parent(&out, &commit, n);
      throw_if(ret);
      return {out, ::git_commit_free};
    }

    auto parent_id(const git_commit &commit, std::uint32_t n) -> git_oid {
      const auto *ret = ::git_commit_parent_id(&commit, n);
      return *ret;
    }

    auto parent_count(const git_commit &commit) -> std::uint32_t {
      return git_commit_parentcount(&commit);
    }

    auto id_str(const git_commit &commit) -> std::string {
      const auto &obj = reinterpret_cast<const git_object &>(commit);
      return object::id_str(obj);
    }

  } // namespace commit

  namespace diff {
    auto index_to_workdir(git_repository &repo, git_index &index, const git_diff_options &opts)
      -> diff_ptr {
      auto *ptr = static_cast<git_diff *>(nullptr);
      auto ret  = ::git_diff_index_to_workdir(&ptr, &repo, &index, &opts);
      throw_if(ret);
      return {ptr, ::git_diff_free};
    }

    auto tree_to_tree(
      git_repository &repo,
      git_tree &old_tree,
      git_tree &new_tree,
      const git_diff_options &opts) -> diff_ptr {
      auto *ptr = static_cast<git_diff *>(nullptr);
      auto ret  = ::git_diff_tree_to_tree(&ptr, &repo, &old_tree, &new_tree, &opts);
      throw_if(ret);
      return {ptr, ::git_diff_free};
    }

    auto commit_to_commit(git_repository &repo, git_commit &commit1, git_commit &commit2)
      -> diff_ptr {
      auto tree1 = commit::tree(commit1);
      auto tree2 = commit::tree(commit2);
      return tree_to_tree(repo, *tree1, *tree2, init_option());
    }

    auto get(git_repository &repo, git_commit &commit1, git_commit &commit2) -> diff_ptr {
      return commit_to_commit(repo, commit1, commit2);
    }

    auto init_option() -> git_diff_options {
      auto opts = git_diff_options{};
      auto ret  = ::git_diff_options_init(&opts, GIT_DIFF_OPTIONS_VERSION);
      throw_if(ret);
      return opts;
    }

    auto num_deltas(git_diff &diff) -> std::size_t {
      return ::git_diff_num_deltas(&diff);
    }

    auto get_delta(const git_diff &diff, size_t idx) -> const git_diff_delta * {
      return ::git_diff_get_delta(&diff, idx);
    }

    auto deltas(git_diff &diff) -> std::unordered_map<std::string, git_diff_delta> {
      auto res        = std::unordered_map<std::string, git_diff_delta>{};
      auto num_deltas = diff::num_deltas(diff);
      for (int i = 0; i < num_deltas; ++i) {
        const auto *ret = diff::get_delta(diff, i);
        throw_if(ret == nullptr, "get delta failed since null pointer");
        res[ret->new_file.path] = *ret;
      }
      return res;
    }

    auto deltas(git_repository &repo, const std::string &spec1, const std::string &spec2)
      -> std::unordered_map<std::string, git_diff_delta> {
      auto obj1  = revparse::single(repo, spec1);
      auto obj2  = revparse::single(repo, spec2);
      auto type1 = object::type(*obj1);
      auto type2 = object::type(*obj2);
      if (type1 != GIT_OBJECT_COMMIT || type2 != GIT_OBJECT_COMMIT) {
        throw_unsupported();
      }

      const auto &commit1 = convert<const git_commit &>(*obj1);
      const auto &commit2 = convert<const git_commit &>(*obj2);
      auto tree1          = commit::tree(commit1);
      auto tree2          = commit::tree(commit2);

      auto diff = diff::tree_to_tree(repo, *tree1, *tree2, init_option());
      auto ret  = deltas(*diff);
      return ret;
    }

    auto changed_files(git_repository &repo, const std::string &spec1, const std::string &spec2)
      -> std::vector<std::string> {
      auto details = deltas(repo, spec1, spec2);
      auto res     = std::vector<std::string>{};
      for (const auto &[file, delta]: details) {
        res.emplace_back(file);
      }
      return res;
    }

    auto changed_files(const std::unordered_map<std::string, git_diff_delta> &deltas)
      -> std::vector<std::string> {
      auto ret = std::vector<std::string>{};
      for (const auto &[file, delta]: deltas) {
        ret.push_back(file);
      }
      return ret;
    }

    auto to_str(git_diff &diff, git_diff_format_t format) -> std::string {
      auto buf = ::git_buf{};
      auto ret = ::git_diff_to_buf(&buf, &diff, format);
      throw_if(ret);
      auto data = std::string{buf.ptr, buf.size};
      ::git_buf_dispose(&buf);
      return data;
    }

  } // namespace diff

  namespace oid {
    auto to_str(const git_oid &oid) -> std::string {
      auto buffer = std::string{};
      // +1 is for null terminated.
      buffer.resize(GIT_OID_MAX_HEXSIZE + 1);
      ::git_oid_tostr(buffer.data(), buffer.size(), &oid);
      return buffer;
    }

    auto equal(const git_oid &o1, const git_oid &o2) -> bool {
      return ::git_oid_equal(&o1, &o2) == 1;
    }

    auto from_str(const std::string &str) -> git_oid {
      auto oid = git_oid{};
      auto ret = ::git_oid_fromstr(&oid, str.c_str());
      throw_if(ret);
      return oid;
    }

  } // namespace oid

  namespace ref {
    auto type(const git_reference &ref) -> git_reference_t {
      return ::git_reference_type(&ref);
    }

    auto is_branch(git_reference &ref) -> bool {
      return ::git_reference_is_branch(&ref) == 1;
    }

    auto is_remote(git_reference &ref) -> bool {
      return ::git_reference_is_remote(&ref) == 1;
    }

    auto is_tag(git_reference &ref) -> bool {
      return ::git_reference_is_tag(&ref) == 1;
    }

    void free(git_reference &ref) {
      ::git_reference_free(&ref);
    }

    auto name(const git_reference &ref) -> std::string {
      const auto *ret = ::git_reference_name(&ref);
      throw_if(ret == nullptr, "get reference name error");
      return ret;
    }

    auto lookup(git_repository &repo, const std::string &name) -> ref_ptr {
      auto *ref = static_cast< git_reference *>(nullptr);
      auto ret  = ::git_reference_lookup(&ref, &repo, name.c_str());
      throw_if(ret);
      return {ref, ::git_reference_free};
    }

    auto name_to_oid(git_repository &repo, const std::string &name) -> git_oid {
      auto oid = ::git_oid{};
      auto ret = ::git_reference_name_to_id(&oid, &repo, name.c_str());
      throw_if(ret);
      return oid;
    }

    auto shorthand(const git_reference &ref) -> std::string {
      const auto *ret = ::git_reference_shorthand(&ref);
      throw_if(ret == nullptr, "get shorthand error since unexpcetd null pointer");
      return ret;
    }

    auto resolve(const git_reference &symbolic_ref) -> ref_ptr {
      auto *ref = static_cast< git_reference *>(nullptr);
      auto ret  = ::git_reference_resolve(&ref, &symbolic_ref);
      throw_if(ret);
      return {ref, ::git_reference_free};
    }

    auto peel(const git_reference &ref, git_object_t obj_type) -> object_ptr {
      auto *obj = static_cast< git_object *>(nullptr);
      auto ret  = ::git_reference_peel(&obj, &ref, obj_type);
      if (ret == GIT_ENOTFOUND) {
        return {nullptr, ::git_object_free};
      }
      throw_if(ret);
      return {obj, ::git_object_free};
    }

  } // namespace ref

  namespace revparse {
    auto single(git_repository &repo, const std::string &spec) -> object_ptr {
      auto *obj = static_cast< git_object *>(nullptr);
      auto ret  = ::git_revparse_single(&obj, &repo, spec.c_str());
      throw_if(ret);
      return {obj, ::git_object_free};
    }

    auto commit(git_repository &repo, const std::string &spec) -> commit_ptr {
      return convert<commit_ptr>(single(repo, spec));
    }

    auto complete_sha(git_repository &repo, const std::string &short_sha) -> std::string {
      auto obj  = single(repo, short_sha);
      auto type = object::type(*obj);
      throw_unless(type == git_object_t::GIT_OBJECT_COMMIT, "the given sha is not commit");
      return oid::to_str(object::id(*obj));
    }

  }; // namespace revparse

  namespace object {
    auto type(const git_object &obj) -> git_object_t {
      return ::git_object_type(&obj);
    }

    auto id(const git_object &obj) -> git_oid {
      const auto *ret = ::git_object_id(&obj);
      throw_if(ret == nullptr, "get object id failed");
      return *ret;
    }

    auto id_str(const git_object &obj) -> std::string {
      return oid::to_str(id(obj));
    }

    auto lookup(git_repository &repo, const git_oid &oid, git_object_t type) -> object_ptr {
      auto *obj = static_cast< git_object *>(nullptr);
      auto ret  = ::git_object_lookup(&obj, &repo, &oid, type);
      throw_if(ret);
      return {obj, ::git_object_free};
    }

  } // namespace object

  namespace sig {
    auto create_default(git_repository &repo) -> signature_ptr {
      auto *sig = static_cast< git_signature *>(nullptr);
      auto ret  = ::git_signature_default(&sig, &repo);
      throw_if(ret);
      return {sig, ::git_signature_free};
    }

  } // namespace sig

  namespace index {
    void write(git_index &index) {
      auto ret = ::git_index_write(&index);
      throw_if(ret);
    }

    auto write_tree(git_index &index) -> git_oid {
      auto oid = git_oid{};
      auto ret = ::git_index_write_tree(&oid, &index);
      throw_if(ret);
      return oid;
    }

    void add_by_path(git_index &index, const std::string &path) {
      auto ret = ::git_index_add_bypath(&index, path.c_str());
      throw_if(ret);
    }

    void remove_by_path(git_index &index, const std::string &path) {
      auto ret = ::git_index_remove_bypath(&index, path.c_str());
      throw_if(ret);
    }

    auto add_files(git_repository &repo, const std::vector<std::string> &files)
      -> std::tuple<git_oid, tree_ptr> {
      auto index = repo::index(repo);
      for (const auto &file: files) {
        git::index::add_by_path(*index, file);
      }
      auto oid = index::write_tree(*index);
      auto obj = tree::lookup(repo, oid);
      return {oid, std::move(obj)};
    }

    auto remove_files(git_repository &repo,
                      const std::string &repo_path,
                      const std::vector<std::string> &files) -> std::tuple<git_oid, tree_ptr> {
      auto index = repo::index(repo);
      for (const auto &file: files) {
        git::index::remove_by_path(*index, file);
        auto full_path = fmt::format("{}/{}", repo_path, file);
        std::filesystem::remove(full_path);
      }
      auto oid = index::write_tree(*index);
      auto obj = tree::lookup(repo, oid);
      return {oid, std::move(obj)};
    }
  } // namespace index

  namespace tree {
    auto lookup(git_repository &repo, const git_oid &oid) -> tree_ptr {
      auto *tree = static_cast<git_tree *>(nullptr);
      auto ret   = ::git_tree_lookup(&tree, &repo, &oid);
      throw_if(ret);
      return {tree, git_tree_free};
    }

    auto entry_id(const git_tree_entry &entry) -> git_oid {
      return *::git_tree_entry_id(&entry);
    }

    auto entry_byname(const git_tree &tree, const std::string &filename)
      -> std::tuple< git_oid, const git_tree_entry *> {
      const auto *entry = ::git_tree_entry_byname(&tree, filename.c_str());
      if (entry == nullptr) {
        return {git_oid{}, nullptr};
      }
      const auto *oid = ::git_tree_entry_id(entry);
      return {*oid, entry};
    }

  } // namespace tree

  namespace status {
    auto gather(git_repository &repo, const git_status_options &options) -> status_list_ptr {
      auto *list = static_cast<git_status_list *>(nullptr);
      auto ret   = ::git_status_list_new(&list, &repo, &options);
      throw_if(ret);
      return {list, ::git_status_list_free};
    }

    auto entry_count(git_status_list &status_list) -> std::size_t {
      auto ret = ::git_status_list_entrycount(&status_list);
      return ret;
    }

    auto default_options() -> git_status_options {
      auto options = git_status_options{};
      throw_if(::git_status_options_init(&options, GIT_STATUS_OPTIONS_VERSION));
      return options;
    }

    auto get_by_index(git_status_list &status_list, std::size_t idx) -> const git_status_entry * {
      const auto *ret = ::git_status_byindex(&status_list, idx);
      throw_if(ret == nullptr, "get status list error since the given idx is out of range");
      return ret;
    }

  } // namespace status

  namespace patch {
    auto create_from_diff(git_diff &diff, std::size_t idx) -> patch_ptr {
      auto *patch = static_cast<git_patch *>(nullptr);
      auto ret    = ::git_patch_from_diff(&patch, &diff, idx);
      throw_if(ret);
      return {patch, ::git_patch_free};
    }

    auto create_from_diff(git_diff &diff) -> std::unordered_map<std::string, patch_ptr> {
      auto res        = std::unordered_map<std::string, patch_ptr>{};
      auto num_deltas = git::diff::num_deltas(diff);
      for (int i = 0; i < num_deltas; ++i) {
        auto patch        = git::patch::create_from_diff(diff, i);
        const auto *delta = git::patch::get_delta(*patch);
        res.insert({delta->new_file.path, std::move(patch)});
      }

      return res;
    }

    auto create_from_buffers(
      const std::string &old_buffer,
      const std::string &old_as_path,
      const std::string &new_buffer,
      const std::string &new_as_path,
      const git_diff_options &opts) -> patch_ptr {
      auto *patch = static_cast<git_patch *>(nullptr);
      auto ret    = ::git_patch_from_buffers(
        &patch,
        old_buffer.data(),
        old_buffer.size(),
        old_as_path.data(),
        new_buffer.data(),
        new_buffer.size(),
        new_as_path.data(),
        &opts);
      throw_if(ret);
      return {patch, ::git_patch_free};
    }

    auto changed_files(const std::unordered_map<std::string, patch_ptr> &patches)
      -> std::vector<std::string> {
      auto ret = std::vector<std::string>{};
      for (const auto &[file, patch]: patches) {
        ret.push_back(file);
      }
      return ret;
    }

    auto to_str(git_patch &patch) -> std::string {
      auto buf = ::git_buf{};
      auto ret = ::git_patch_to_buf(&buf, &patch);
      throw_if(ret);
      auto data = std::string(buf.ptr, buf.size);
      ::git_buf_dispose(&buf);
      return data;
    }

    auto get_delta(const git_patch &patch) -> const git_diff_delta * {
      return ::git_patch_get_delta(&patch);
    }

    auto num_hunks(const git_patch &patch) -> std::size_t {
      return ::git_patch_num_hunks(&patch);
    }

    auto get_hunk(git_patch &patch, std::size_t hunk_idx)
      -> std::tuple<git_diff_hunk, std::size_t> {
      const auto *hunk_ptr = static_cast<git_diff_hunk *>(nullptr);
      auto line_num        = std::size_t{0};
      auto ret             = ::git_patch_get_hunk(&hunk_ptr, &line_num, &patch, hunk_idx);
      throw_if(ret);
      return {*hunk_ptr, line_num};
    }

    auto num_lines_in_hunk(const git_patch &patch, std::size_t hunk_idx) -> std::size_t {
      return ::git_patch_num_lines_in_hunk(&patch, hunk_idx);
    }

    auto get_line_in_hunk(git_patch &patch, std::size_t hunk_idx, std::size_t line_idx)
      -> git_diff_line {
      const auto *line_ptr = static_cast<git_diff_line *>(nullptr);
      auto ret             = ::git_patch_get_line_in_hunk(&line_ptr, &patch, hunk_idx, line_idx);
      throw_if(ret);
      return *line_ptr;
    }

    auto get_lines_in_hunk(git_patch &patch, std::size_t hunk_idx) -> std::vector<std::string> {
      auto ret         = std::vector<std::string>{};
      size_t num_lines = patch::num_lines_in_hunk(patch, hunk_idx);
      for (size_t i = 0; i < num_lines; i++) {
        auto line = get_line_in_hunk(patch, hunk_idx, i);
        ret.emplace_back(line.content, line.content_len);
      }
      return ret;
    }

    auto get_target_lines_in_hunk(git_patch &patch, std::size_t hunk_idx)
      -> std::vector<std::string> {
      auto ret         = std::vector<std::string>{};
      size_t num_lines = patch::num_lines_in_hunk(patch, hunk_idx);
      for (size_t i = 0; i < num_lines; i++) {
        auto line = get_line_in_hunk(patch, hunk_idx, i);
        if (line.origin == GIT_DIFF_LINE_CONTEXT || line.origin == GIT_DIFF_LINE_DELETION) {
          ret.emplace_back(line.content, line.content_len);
        }
      }
      return ret;
    }

    auto get_source_lines_in_hunk(git_patch &patch, std::size_t hunk_idx)
      -> std::vector<std::string> {
      auto ret         = std::vector<std::string>{};
      size_t num_lines = patch::num_lines_in_hunk(patch, hunk_idx);
      for (size_t i = 0; i < num_lines; i++) {
        auto line = get_line_in_hunk(patch, hunk_idx, i);
        if (line.origin == GIT_DIFF_LINE_CONTEXT || line.origin == GIT_DIFF_LINE_ADDITION) {
          ret.emplace_back(line.content, line.content_len);
        }
      }
      return ret;
    }

  } // namespace patch

  namespace hunk {
    auto is_old_line(const git_diff_line &line) -> bool {
      return line.origin == GIT_DIFF_LINE_DELETION;
    }

    auto is_new_line(const git_diff_line &line) -> bool {
      return line.origin == GIT_DIFF_LINE_ADDITION;
    }

    auto get_line_content(const git_diff_line &line) -> std::string {
      return {line.content, line.content_len};
    }

    auto get_old_line_number(const git_diff_line &line) -> std::optional<std::size_t> {
      if (line.old_lineno == -1) {
        return std::nullopt;
      }
      return static_cast<std::size_t>(line.old_lineno);
    }

    auto get_new_line_number(const git_diff_line &line) -> std::optional<std::size_t> {
      if (line.new_lineno == -1) {
        return std::nullopt;
      }
      return static_cast<std::size_t>(line.new_lineno);
    }

    bool is_row_in_hunk(const git_diff_hunk &hunk, int row_number) noexcept {
      return row_number >= hunk.new_start && row_number <= hunk.new_start + hunk.new_lines;
    }
  } // namespace hunk

  namespace blob {
    auto lookup(git_repository &repo, const git_oid &oid) -> blob_ptr {
      auto *blob = static_cast<git_blob *>(nullptr);
      auto ret   = ::git_blob_lookup(&blob, &repo, &oid);
      throw_if(ret);
      return {blob, ::git_blob_free};
    }

    auto get_raw_content(const git_blob &blob) -> std::string {
      const auto *ret = ::git_blob_rawcontent(&blob);
      throw_if(ret == nullptr, "get raw content by blob error");
      return static_cast<const char *>(ret);
    }

    auto get_raw_content(git_repository &repo, const git_tree &tree, const std::string &file_name)
      -> std::string {
      throw_if(file_name.empty(), "failed to get raw content sicne file name is empty");
      auto [entry_id, entry] = tree::entry_byname(tree, file_name);
      if (entry == nullptr) {
        return "";
      }
      auto blob = lookup(repo, entry_id);
      return get_raw_content(*blob);
    }

    auto get_raw_content(git_repository &repo,
                         const git_commit &commit,
                         const std::string &file_name) -> std::string {
      auto tree = commit::tree(commit);
      return get_raw_content(repo, *tree, file_name);
    }

  } // namespace blob

} // namespace lint::git
