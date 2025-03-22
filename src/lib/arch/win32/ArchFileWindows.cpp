/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/win32/ArchFileWindows.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shlobj.h>
#include <string.h>
#include <tchar.h>

//
// ArchFileWindows
//

ArchFileWindows::ArchFileWindows()
{
  // do nothing
}

ArchFileWindows::~ArchFileWindows()
{
  // do nothing
}

std::string ArchFileWindows::getInstalledDirectory()
{
  char fileNameBuffer[MAX_PATH];
  GetModuleFileName(NULL, fileNameBuffer, MAX_PATH);
  std::string fileName(fileNameBuffer);
  size_t lastSlash = fileName.find_last_of("\\");
  fileName = fileName.substr(0, lastSlash);

  return fileName;
}
