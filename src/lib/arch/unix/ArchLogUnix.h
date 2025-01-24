/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchLog.h"

#define ARCH_LOG ArchLogUnix

//! Unix implementation of IArchLog
class ArchLogUnix : public IArchLog
{
public:
  ArchLogUnix();
  virtual ~ArchLogUnix();

  // IArchLog overrides
  virtual void openLog(const char *name);
  virtual void closeLog();
  virtual void showLog(bool);
  virtual void writeLog(ELevel, const char *);
};
