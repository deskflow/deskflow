/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/win32/ArchSystemWindows.h"
#include "arch/win32/ArchMiscWindows.h"

#include "arch/XArch.h"
#include "common/constants.h"

#include "tchar.h"
#include <string>

#include <psapi.h>
#include <windows.h>

static const char *s_settingsKeyNames[] = {_T("SOFTWARE"), _T(kAppName), NULL};

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

std::string ArchSystemWindows::getOSName() const
{
  std::string osName("Microsoft Windows <unknown>");
  static const TCHAR *const windowsVersionKeyNames[] = {
      _T("SOFTWARE"), _T("Microsoft"), _T("Windows NT"), _T("CurrentVersion"), NULL
  };

  HKEY key = ArchMiscWindows::openKey(HKEY_LOCAL_MACHINE, windowsVersionKeyNames);
  if (key == NULL) {
    return osName;
  }

  std::string productName = ArchMiscWindows::readValueString(key, "ProductName");
  if (osName.empty()) {
    return osName;
  }

  return "Microsoft " + productName;
}

std::string ArchSystemWindows::getPlatformName() const
{
#ifdef _X86_
  if (isWOW64())
    return "x86 (WOW64)";
  else
    return "x86";
#else
#ifdef _AMD64_
  return "x64";
#else
  return "Unknown";
#endif
#endif
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

bool ArchSystemWindows::isWOW64() const
{
#if WINVER >= _WIN32_WINNT_WINXP
  typedef BOOL(WINAPI * LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
  HMODULE hModule = GetModuleHandle(TEXT("kernel32"));
  if (!hModule)
    return FALSE;

  LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(hModule, "IsWow64Process");

  BOOL bIsWow64 = FALSE;
  if (NULL != fnIsWow64Process && fnIsWow64Process(GetCurrentProcess(), &bIsWow64) && bIsWow64) {
    return true;
  }
#endif
  return false;
}
