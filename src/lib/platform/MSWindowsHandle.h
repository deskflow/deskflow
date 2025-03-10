/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/Log.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

/**
 * @brief RAII pattern for a Windows handle (ensures handle is closed when out of scope).
 */
class MSWindowsHandle
{
public:
  explicit MSWindowsHandle(HANDLE handle) : m_handle(handle)
  {
  }

  MSWindowsHandle(const MSWindowsHandle &) = delete;
  MSWindowsHandle &operator=(const MSWindowsHandle &) = delete;
  MSWindowsHandle(MSWindowsHandle &&) = delete;
  MSWindowsHandle &operator=(MSWindowsHandle &&) = delete;

  ~MSWindowsHandle()
  {
    if (m_handle == nullptr || m_handle == INVALID_HANDLE_VALUE) {
      return;
    }

    if (!CloseHandle(m_handle)) {
      LOG_WARN("failed to close handle");
    }
  }

  HANDLE
  get() const
  {
    return m_handle;
  }

private:
  HANDLE m_handle = nullptr;
};
