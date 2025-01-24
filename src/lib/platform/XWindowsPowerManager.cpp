/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "XWindowsPowerManager.h"
#include "arch/Arch.h"
#include "base/Log.h"

namespace {

bool sleepInhibitCall(bool state, ArchSystemUnix::InhibitScreenServices serviceID)
{
  std::string error;

  if (!ArchSystemUnix::DBusInhibitScreenCall(serviceID, state, error)) {
    LOG((CLOG_DEBUG "dbus inhibit error %s", error.c_str()));
    return false;
  }

  return true;
}

} // namespace

XWindowsPowerManager::~XWindowsPowerManager()
{
  enableSleep();
}

void XWindowsPowerManager::disableSleep() const
{
  if (!sleepInhibitCall(true, ArchSystemUnix::InhibitScreenServices::kScreenSaver) &&
      !sleepInhibitCall(true, ArchSystemUnix::InhibitScreenServices::kSessionManager)) {
    LOG((CLOG_WARN "failed to prevent system from going to sleep"));
  }
}

void XWindowsPowerManager::enableSleep() const
{
  if (!sleepInhibitCall(false, ArchSystemUnix::InhibitScreenServices::kScreenSaver) &&
      !sleepInhibitCall(false, ArchSystemUnix::InhibitScreenServices::kSessionManager)) {
    LOG((CLOG_WARN "failed to enable system idle sleep"));
  }
}
