/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2021 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "OSXPowerManager.h"
#include "base/Log.h"

OSXPowerManager::OSXPowerManager()
{
}

OSXPowerManager::~OSXPowerManager()
{
  enableSleep();
}

void OSXPowerManager::disableSleep()
{
  if (!m_sleepPreventionAssertionID) {
    CFStringRef reasonForActivity = CFSTR("Deskflow application");
    IOReturn result = IOPMAssertionCreateWithName(
        kIOPMAssertPreventUserIdleDisplaySleep, kIOPMAssertionLevelOn, reasonForActivity, &m_sleepPreventionAssertionID
    );
    if (result != kIOReturnSuccess) {
      m_sleepPreventionAssertionID = 0;
      LOG((CLOG_ERR "failed to disable system idle sleep"));
    }
  }
}

void OSXPowerManager::enableSleep()
{
  if (m_sleepPreventionAssertionID) {
    IOPMAssertionRelease(m_sleepPreventionAssertionID);
  }
}
