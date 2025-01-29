/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/win32/ArchStringWindows.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdio.h>

//
// ArchStringWindows
//

ArchStringWindows::ArchStringWindows()
{
}

ArchStringWindows::~ArchStringWindows()
{
}

IArchString::EWideCharEncoding ArchStringWindows::getWideCharEncoding()
{
  return kUTF16;
}
