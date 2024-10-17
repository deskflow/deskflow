/*
 * Deskflow -- mouse and keyboard sharing utility
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

#include "ExitTimeout.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <string_view>
#include <thread>

namespace deskflow::test {

const auto checkMilliseconds = std::chrono::milliseconds(100);

using std::chrono::duration_cast;
using std::chrono::steady_clock;

bool timeoutReached(const steady_clock::time_point &start, const int minutes)
{
  auto now = steady_clock::now();
  auto duration = duration_cast<std::chrono::minutes>(now - start);
  return duration.count() >= minutes;
}

ExitTimeout::ExitTimeout(const int minutes, const std::string_view &name)
    : m_minutes(minutes),
      m_name(name),
      m_thread(std::make_unique<std::thread>([this]() { run(); }))
{
}

ExitTimeout::~ExitTimeout()
{
  m_running = false;
  m_thread->join();
}

void ExitTimeout::run() const
{
  auto start = steady_clock::now();
  while (m_running) {
    std::this_thread::sleep_for(checkMilliseconds);
    if (timeoutReached(start, m_minutes)) {
      std::cerr << m_name << " timed out after " << m_minutes << " minute(s)" << std::endl;
      std::exit(EXIT_FAILURE);
    }
  }
}

} // namespace deskflow::test
