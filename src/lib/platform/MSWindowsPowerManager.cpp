/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2021 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "MSWindowsPowerManager.h"
#include "arch/win32/ArchMiscWindows.h"

MSWindowsPowerManager::~MSWindowsPowerManager()
{
  enableSleep();
}

void MSWindowsPowerManager::disableSleep()
{
  ArchMiscWindows::addBusyState(ArchMiscWindows::kSYSTEM);
  ArchMiscWindows::addBusyState(ArchMiscWindows::kDISPLAY);
}

void MSWindowsPowerManager::enableSleep()
{
  // allow the system to enter power saving mode
  ArchMiscWindows::removeBusyState(ArchMiscWindows::kSYSTEM);
  ArchMiscWindows::removeBusyState(ArchMiscWindows::kDISPLAY);
}
