/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
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
