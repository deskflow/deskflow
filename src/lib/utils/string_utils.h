/*
 * deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <ranges>
#include <string>

const auto isNotSpace = [](unsigned char ch) { return !std::isspace(ch); };

namespace deskflow::utils {

/**
 * @brief A _very_ basic whitespace trimer.
 *
 * Ideally C++20 would have a std::trim function, but until then, this will do.
 */
inline std::string trim(const std::string &str) {
  auto front = std::ranges::find_if(str, isNotSpace);
  auto back =
      std::ranges::find_if(str | std::views::reverse, isNotSpace).base();
  return (front < back ? std::string(front, back) : std::string{});
}

} // namespace deskflow::utils
