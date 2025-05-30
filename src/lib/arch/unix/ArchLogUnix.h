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
  ArchLogUnix() = default;
  ~ArchLogUnix() override = default;

  // IArchLog overrides
  void openLog(const QString &name) override;
  void closeLog() override;
  void showLog(bool) override;
  void writeLog(LogLevel, const QString &) override;
};
