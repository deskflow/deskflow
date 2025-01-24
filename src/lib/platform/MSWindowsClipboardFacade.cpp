/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/MSWindowsClipboardFacade.h"

#include "platform/MSWindowsClipboard.h"

void MSWindowsClipboardFacade::write(HANDLE win32Data, UINT win32Format)
{
  if (SetClipboardData(win32Format, win32Data) == NULL) {
    // free converted data if we couldn't put it on
    // the clipboard.
    // nb: couldn't cause this in integ tests.
    GlobalFree(win32Data);
  }
}
