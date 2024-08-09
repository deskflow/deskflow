/*
 * synergy -- mouse and keyboard sharing utility
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

#include <cstring>
#include <string>
#include <vector>

namespace synergy {

const auto kCopyrightFormat = //
    "Copyright (C) 2012-%s Symless Ltd.\n"
    "Copyright (C) 2009-2012 Nick Bolton\n"
    "Copyright (C) 2002-2009 Chris Schoeneman";

inline std::string copyright() {
  const std::string date = __DATE__;
  const auto year = date.substr(date.size() - 4);
  const auto kBufferLength = 256;
  std::vector<char> buffer(kBufferLength);
  std::snprintf( // NOSONAR
      buffer.data(), kBufferLength, kCopyrightFormat, year.c_str());
  return std::string(buffer.data());
}

} // namespace synergy
