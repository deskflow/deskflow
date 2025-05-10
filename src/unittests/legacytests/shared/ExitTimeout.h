/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <string_view>
#include <thread>

namespace deskflow::test {

/**
 * @brief Exits the program after a specified timeout, unless destroyed.
 *
 * The `std::thread` class is used instead of `std::jthread` as Apple Clang has
 * no `std::jthread` support: https://en.cppreference.com/w/cpp/compiler_support
 *
 * TODO: Switch to regular Clang instead of Apple Clang:
 * https://symless.atlassian.net/browse/S1-1754
 */
class ExitTimeout
{
public:
  ExitTimeout(const int minutes, const std::string_view &name);
  ~ExitTimeout();
  void run() const;

private:
  bool m_running = true;
  int m_minutes = 0;
  std::string_view m_name;
  std::unique_ptr<std::thread> m_thread;
};

} // namespace deskflow::test
