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

#include <stdexcept>
#include <string>
#include <vector>

namespace synergy {

class Config {
  class ParseError : public std::runtime_error {
  public:
    explicit ParseError() : std::runtime_error("failed to parse config file") {}
  };

public:
  explicit Config(const std::string &filename, const std::string &section);
  bool load(const std::string &firstArg);
  const char *const *argv() const;
  int argc() const;

private:
  std::string m_filename;
  std::string m_section;
  std::vector<std::string> m_args;
  std::vector<const char *> m_argv;
};

} // namespace synergy
