/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/IArchSystem.h"

#define ARCH_SYSTEM ArchSystemUnix

//! Unix implementation of IArchString
class ArchSystemUnix : public IArchSystem
{
public:
  ArchSystemUnix();
  virtual ~ArchSystemUnix();

  // IArchSystem overrides
  virtual std::string getOSName() const;
  virtual std::string getPlatformName() const;
  virtual std::string setting(const std::string &) const;
  virtual void setting(const std::string &, const std::string &) const;
  virtual std::string getLibsUsed(void) const;

#ifndef __APPLE__
  enum class InhibitScreenServices
  {
    kScreenSaver,
    kSessionManager
  };
  static bool DBusInhibitScreenCall(InhibitScreenServices serviceID, bool state, std::string &error);
#endif
};
