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

#include <string_view>
#include <thread>

namespace synergy::test {

/// @brief Exits the program after a specified timeout, unless cancelled.
class ExitTimeout {
public:
  ExitTimeout(const int minutes, const std::string_view &name);
  ~ExitTimeout() = default;
  void run() const;
  void cancel();

private:
  bool m_running = true;
  int m_minutes = 0;
  std::string_view m_name;
  std::unique_ptr<std::jthread> m_thread;
};

} // namespace synergy::test
