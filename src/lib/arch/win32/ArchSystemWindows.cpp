/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/win32/ArchSystemWindows.h"
#include "arch/win32/ArchMiscWindows.h"

#include "arch/XArch.h"
#include "base/Log.h"
#include "common/constants.h"

#include "tchar.h"
#include <string>

#include <psapi.h>
#include <windows.h>

static const TCHAR *s_settingsKeyNames[] = {_T("SOFTWARE"), _T(kAppName), NULL};

//
// ArchSystemWindows
//

ArchSystemWindows::ArchSystemWindows()
{
  // do nothing
}

ArchSystemWindows::~ArchSystemWindows()
{
  // do nothing
}

std::string ArchSystemWindows::setting(const std::string &valueName) const
{
  HKEY key = ArchMiscWindows::openKey(HKEY_LOCAL_MACHINE, s_settingsKeyNames);
  if (key == NULL)
    return "";

  return ArchMiscWindows::readValueString(key, valueName.c_str());
}

void ArchSystemWindows::setting(const std::string &valueName, const std::string &valueString) const
{
  HKEY key = ArchMiscWindows::addKey(HKEY_LOCAL_MACHINE, s_settingsKeyNames);
  if (key == NULL)
    throw XArch(std::string("could not access registry key: ") + valueName);
  ArchMiscWindows::setValue(key, valueName.c_str(), valueString.c_str());
}

void ArchSystemWindows::clearSettings() const
{
  ArchMiscWindows::deleteKeyTree(HKEY_LOCAL_MACHINE, kWindowsRegistryKey);
}
