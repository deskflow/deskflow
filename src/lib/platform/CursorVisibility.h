/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <mutex>

namespace deskflow {

// Keep the native hide/show count and our state together across input threads.
class CursorVisibility
{
public:
  template <typename Apply> bool setHidden(bool hidden, Apply apply)
  {
    std::lock_guard lock(m_mutex);
    if (m_hidden == hidden) {
      return true;
    }
    if (!apply()) {
      return false;
    }
    m_hidden = hidden;
    return true;
  }

private:
  std::mutex m_mutex;
  bool m_hidden = false;
};

} // namespace deskflow
