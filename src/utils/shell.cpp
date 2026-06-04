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
#include "shell.h"

#include <spdlog/spdlog.h>
#include <string>
#include <string_view>

#include <boost/asio/error.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/readable_pipe.hpp>
#include <boost/process/v2/environment.hpp>
#include <boost/process/v2/process.hpp>
#include <boost/process/v2/start_dir.hpp>
#include <boost/process/v2/stdio.hpp>

#include "utils/error.h"
#include "utils/common.h"

namespace lint::shell {
  namespace bp = boost::process::v2;

  namespace {
    void read_pipe(boost::asio::readable_pipe &pipe,
                   std::string &output,
                   std::string_view stream_name,
                   std::string_view command) {
      auto ec = boost::system::error_code{};
      do {
        ec.clear();
        boost::asio::read(pipe, boost::asio::dynamic_buffer(output), ec);
      } while (ec == boost::asio::error::interrupted);

      throw_if(ec && ec != boost::asio::error::eof,
               fmt::format("Read {} message of {} failed since {}",
                           stream_name,
                           command,
                           ec.message()));
    }
  } // namespace

  auto execute(std::string_view command, const options &opts) -> result {
    auto context = boost::asio::io_context{};
    auto rp_out  = boost::asio::readable_pipe{context};
    auto rp_err  = boost::asio::readable_pipe{context};

    auto proc = bp::process(
      context,
      command,
      opts,
      bp::process_stdio{.in = {}, .out = rp_out, .err = rp_err});
    auto res = result{};
    read_pipe(rp_out, res.std_out, "stdout", command);
    read_pipe(rp_err, res.std_err, "stderr", command);

    res.exit_code = proc.wait();
    return res;
  }

  auto execute(std::string_view command, const options &opts, std::string_view start_dir)
    -> result {
    auto context = boost::asio::io_context{};
    auto rp_out  = boost::asio::readable_pipe{context};
    auto rp_err  = boost::asio::readable_pipe{context};

    auto proc = bp::process(
      context,
      command,
      opts,
      bp::process_stdio{.in = {}, .out = rp_out, .err = rp_err},
      bp::process_start_dir{start_dir});
    auto res = result{};
    read_pipe(rp_out, res.std_out, "stdout", command);
    read_pipe(rp_err, res.std_err, "stderr", command);

    res.exit_code = proc.wait();
    return res;
  }

  auto execute(std::string_view command, const options &opts, const envrionment &env) -> result {
    auto context = boost::asio::io_context{};
    auto rp_out  = boost::asio::readable_pipe{context};
    auto rp_err  = boost::asio::readable_pipe{context};

    auto proc = bp::process(
      context,
      command,
      opts,
      bp::process_stdio{.in = {}, .out = rp_out, .err = rp_err},
      bp::process_environment{env});
    auto res = result{};
    read_pipe(rp_out, res.std_out, "stdout", command);
    read_pipe(rp_err, res.std_err, "stderr", command);

    res.exit_code = proc.wait();
    return res;
  }

  auto execute(std::string_view command,
               const options &opts,
               const envrionment &env,
               std::string_view start_dir) -> result {
    auto context = boost::asio::io_context{};
    auto rp_out  = boost::asio::readable_pipe{context};
    auto rp_err  = boost::asio::readable_pipe{context};

    auto proc = bp::process(
      context,
      command,
      opts,
      bp::process_stdio{.in = {}, .out = rp_out, .err = rp_err},
      bp::process_environment{env},
      bp::process_start_dir(start_dir));
    auto res = result{};
    read_pipe(rp_out, res.std_out, "stdout", command);
    read_pipe(rp_err, res.std_err, "stderr", command);

    res.exit_code = proc.wait();
    return res;
  }

  auto which(std::string command) -> result {
    constexpr auto which = std::string_view{"/usr/bin/which"};
    auto res             = execute(which, {std::move(command)});
    res.std_out          = trim(res.std_out);
    return res;
  }

} // namespace lint::shell
