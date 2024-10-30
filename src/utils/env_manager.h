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

#include <cstdlib>
#include <mutex>
#include <string>
#include <unordered_map>

namespace lint::env {

  class thread_safe_env_manager {
  public:
    static thread_safe_env_manager &get_instance() noexcept;

    auto get(const std::string &name) -> std::string;
    void set_cache(const std::string &name, const std::string &value);
    void set_cache(std::unordered_map<std::string, std::string> data);

    thread_safe_env_manager(const thread_safe_env_manager &)            = delete;
    thread_safe_env_manager &operator=(const thread_safe_env_manager &) = delete;

  private:
    thread_safe_env_manager()  = default;
    ~thread_safe_env_manager() = default;

    std::mutex mutex_;
    std::unordered_map<std::string, std::string> cache_;
  };

  [[nodiscard]] auto get(const std::string &name) -> std::string;
  void set_cache(const std::string &name, const std::string &value);
  void set_cache(std::unordered_map<std::string, std::string> data);

} // namespace lint::env
