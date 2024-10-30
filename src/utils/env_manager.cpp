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
#include "env_manager.h"

/// INFO: There is no standard std::setenv

namespace lint::env {
  auto thread_safe_env_manager::get_instance() noexcept -> thread_safe_env_manager & {
    static auto env_manager = thread_safe_env_manager{};
    return env_manager;
  }

  auto thread_safe_env_manager::get(const std::string &name) -> std::string {
    auto lg = std::lock_guard(mutex_);
    if (cache_.contains(name)) {
      return cache_[name];
    }
    auto ret = std::getenv(name.data()); // NOLINT
    if (ret == nullptr) {
      return "";
    }
    cache_[name] = ret;
    return cache_[name];
  }

  void thread_safe_env_manager::set_cache(const std::string &name, const std::string &value) {
    auto lg      = std::lock_guard(mutex_);
    cache_[name] = value;
  }

  void thread_safe_env_manager::set_cache(std::unordered_map<std::string, std::string> data) {
    auto lg = std::lock_guard(mutex_);
    cache_  = std::move(data);
  }

  [[nodiscard]] auto get(const std::string &name) -> std::string {
    auto &env_manager = thread_safe_env_manager::get_instance();
    return env_manager.get(name);
  }

  void set_cache(const std::string &name, const std::string &value) {
    auto &env_manager = thread_safe_env_manager::get_instance();
    env_manager.set_cache(name, value);
  }

  void set_cache(std::unordered_map<std::string, std::string> data) {
    auto &env_manager = thread_safe_env_manager::get_instance();
    env_manager.set_cache(std::move(data));
  }

} // namespace lint::env
