/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchSystem.h"

#define ARCH_SYSTEM ArchSystemWindows

//! Win32 implementation of IArchString
class ArchSystemWindows : public IArchSystem
{
public:
  ArchSystemWindows();
  virtual ~ArchSystemWindows();

  // IArchSystem overrides
  virtual std::string getOSName() const;
  virtual std::string getPlatformName() const;
  virtual std::string setting(const std::string &valueName) const;
  virtual void setting(const std::string &valueName, const std::string &valueString) const;

  bool isWOW64() const;
};
