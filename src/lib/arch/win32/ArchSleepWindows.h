/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchSleep.h"

#define ARCH_SLEEP ArchSleepWindows

//! Win32 implementation of IArchSleep
class ArchSleepWindows : public IArchSleep
{
public:
  ArchSleepWindows();
  virtual ~ArchSleepWindows();

  // IArchSleep overrides
  virtual void sleep(double timeout);
};
