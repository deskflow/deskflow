/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <Tlhelp32.h>

class MSWindowsSession
{
public:
  MSWindowsSession();
  ~MSWindowsSession() = default;

  /*!
  Returns true if the session ID has changed since updateActiveSession was
  called.
  */
  BOOL hasChanged();

  bool isProcessInSession(const wchar_t *name, PHANDLE process);
  HANDLE getUserToken(LPSECURITY_ATTRIBUTES security);
  void updateActiveSession();

  DWORD getActiveSessionId()
  {
    return m_activeSessionId;
  }

private:
  BOOL nextProcessEntry(HANDLE snapshot, LPPROCESSENTRY32 entry);

private:
  DWORD m_activeSessionId;
};
