/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchLog.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define ARCH_LOG ArchLogWindows

//! Win32 implementation of IArchLog
class ArchLogWindows : public IArchLog
{
public:
  ArchLogWindows();
  virtual ~ArchLogWindows();

  // IArchLog overrides
  virtual void openLog(const char *name);
  virtual void closeLog();
  virtual void showLog(bool showIfEmpty);
  virtual void writeLog(ELevel, const char *);

private:
  HANDLE m_eventLog;
};
