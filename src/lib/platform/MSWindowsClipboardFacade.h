/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/IMSWindowsClipboardFacade.h"

#include "deskflow/IClipboard.h"

class MSWindowsClipboardFacade : public IMSWindowsClipboardFacade
{
public:
  virtual void write(HANDLE win32Data, UINT win32Format);
};
