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
#include "tools/clang_format/general/impl.h"

#include <cctype>
#include <cstdint>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include <boost/regex.hpp>
#include <spdlog/spdlog.h>
#include <tinyxml2.h>

#include "context.h"
#include "tools/clang_format/general/reporter.h"
#include "utils/common.h"
#include "utils/shell.h"

namespace lint::tool::clang_format {
  namespace {

    // Read a file and get this
    auto get_line_lens(std::string_view file_path) -> std::vector<uint32_t> {
      spdlog::trace("Enter clang_format::get_line_lens()");
      constexpr auto line_feed_len = 1;

      auto lines = std::vector<uint32_t>{};
      auto file  = std::ifstream{file_path.data()};
      throw_unless(file.is_open(), fmt::format("open file {} error", file_path));

      auto temp = std::string{};
      while (std::getline(file, temp)) {
        lines.emplace_back(temp.size() + line_feed_len);
      }
      return lines;
    }

    // offset starts from 0 while row/col starts from 1
    auto get_position(const std::vector<uint32_t> &lens, int offset)
      -> std::tuple<int32_t, int32_t> {
      spdlog::trace("Enter clang_format::get_position()");

      auto cur_offset = uint32_t{0};
      for (int i = 0; i < lens.size(); ++i) {
        auto len = lens[i];
        if (offset >= cur_offset && offset < cur_offset + len) {
          return {i + 1, offset - cur_offset + 1};
        }
        cur_offset += len;
      }
      return {-1, -1};
    }

    inline auto xml_error(tinyxml2::XMLError err) -> std::string_view {
      return tinyxml2::XMLDocument::ErrorIDToName(err);
    }

    inline auto xml_has_error(tinyxml2::XMLError err) -> bool {
      return err != tinyxml2::XMLError::XML_SUCCESS;
    }

    auto parse_replacements_xml(
      const runtime_context &ctx,
      std::string_view data,
      std::string_view file) -> replacements_t {
      spdlog::trace("Enter clang_format_general::parse_replacements_xml()");

      // Names in replacements xml file.
      static constexpr auto offset_str       = "offset";
      static constexpr auto length_str       = "length";
      static constexpr auto replacements_str = "replacements";
      static constexpr auto replacement_str  = "replacement";

      // Start to parse given data to xml tree.
      auto doc = tinyxml2::XMLDocument{true, tinyxml2::PEDANTIC_WHITESPACE};
      auto err = doc.Parse(data.data());
      throw_if(xml_has_error(err),
               fmt::format("Parse replacements xml failed since: {}", xml_error(err)));
      throw_if(doc.NoChildren(),
               "Parse replacements xml failed since no children in replacements xml");

      // Find <replacements><replacement offset="xxx"
      // length="xxx">text</replacement></replacements>
      auto *replacements_ele = doc.FirstChildElement(replacements_str);
      throw_if(replacements_ele == nullptr,
               "Parse replacements xml failed since no child names 'replacements'");
      auto replacements = replacements_t{};

      const auto lens = get_line_lens(fmt::format("{}/{}", ctx.repo_path, file));

      // Empty replacement node is allowd here.
      auto *replacement_ele = replacements_ele->FirstChildElement(replacement_str);
      while (replacement_ele != nullptr) {
        auto replacement = replacement_t{};
        replacement_ele->QueryIntAttribute(offset_str, &replacement.offset);
        replacement_ele->QueryIntAttribute(length_str, &replacement.length);
        const auto *text = replacement_ele->GetText();
        if (text != nullptr) {
          replacement.data = text;
        }

        auto [row, col] = get_position(lens, replacement.offset);
        replacement.row = row;
        replacement.col = col;
        if (!replacements.contains(row)) {
          replacements[row] = std::vector<replacement_t>{};
        }
        replacements[row].emplace_back(replacement);
        replacement_ele = replacement_ele->NextSiblingElement(replacement_str);
      }
      return replacements;
    }

    auto make_replacements_options(std::string_view file) -> std::vector<std::string> {
      spdlog::trace("Enter clang_format::make_replacements_options()");
      auto tool_opt = std::vector<std::string>{};
      tool_opt.emplace_back("--output-replacements-xml");
      tool_opt.emplace_back(file);
      return tool_opt;
    }

    auto execute(const option_t &opt, std::string_view repo, std::string_view file)
      -> std::tuple<shell::result, std::string> {
      spdlog::trace("Enter clang_format_general::execute()");
      auto tool_opt     = make_replacements_options(file);
      auto tool_opt_str = concat(tool_opt, ' ');
      spdlog::info("Running command: {} {}", opt.binary, tool_opt_str);

      return {shell::execute(opt.binary, tool_opt, repo), tool_opt_str};
    }

  } // namespace

  auto clang_format_general::check_single_file(
    const runtime_context &context,
    const std::string &root_dir,
    const std::string &file) const -> per_file_result {
    spdlog::trace("Enter clang_format_general::check_single_file()");

    auto [xml_res, file_opt] = execute(option, root_dir, file);
    auto result              = per_file_result{};
    result.file_path         = file;
    result.tool_stdout       = xml_res.std_out;
    result.tool_stderr       = xml_res.std_err;
    result.file_option       = file_opt;
    if (xml_res.exit_code != 0) {
      result.passed = false;
      return result;
    }

    auto replacements   = parse_replacements_xml(context, xml_res.std_out, file);
    result.passed       = replacements.empty();
    result.replacements = std::move(replacements);
    return result;
  }

  void clang_format_general::check(const runtime_context &context) {
    assert(!option.binary.empty() && "clang-format binary is empty");
    assert(!context.repo_path.empty() && "the repo_path of context is empty");

    const auto root_dir = context.repo_path;
    const auto files    = context.changed_files;
    for (const auto &file: files) {
      const auto &delta = context.deltas.at(file);
      if (delta.status == GIT_DELTA_DELETED) {
        continue;
      }
      if (filter_file(option.file_filter_iregex, file)) {
        result.ignored.push_back(file);
        spdlog::debug("file {} is ignored by {}", file, option.binary);
        continue;
      }

      auto per_file_result = check_single_file(context, root_dir, file);
      if (per_file_result.passed) {
        spdlog::info("file: {} passes {} check.", file, option.binary);
        result.passes[file] = std::move(per_file_result);
        continue;
      }

      spdlog::error("file: {} doesn't pass {} check.", file, option.binary);
      result.failed_commands.emplace_back(
        fmt::format("clang-format {}", per_file_result.file_option));
      result.fails[file] = std::move(per_file_result);

      if (option.enabled_fastly_exit) {
        spdlog::info("{} fastly exit since check failed", option.binary);
        result.final_passed  = false;
        result.fastly_exited = true;
        return;
      }
    }

    result.final_passed = result.fails.empty();
  }

  auto clang_format_general::get_reporter() -> reporter_base_ptr {
    return std::make_unique<reporter_t>(option, result);
  }

} // namespace lint::tool::clang_format
