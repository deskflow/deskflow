/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/IInterface.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class IMSWindowsClipboardConverter;

class IMSWindowsClipboardFacade : public IInterface
{
public:
  virtual void write(HANDLE win32Data, UINT win32Format) = 0;
  virtual ~IMSWindowsClipboardFacade()
  {
  }
};
