/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2013 - 2016 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <Tlhelp32.h>

class MSWindowsSession
{
public:
  MSWindowsSession();
  ~MSWindowsSession();

  /*!
  Returns true if the session ID has changed since updateActiveSession was
  called.
  */
  BOOL hasChanged();

  bool isProcessInSession(const char *name, PHANDLE process);
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
