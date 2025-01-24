/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2011 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/dfwhook.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

//! Loads and provides functions for the Windows hook
class MSWindowsHook
{
public:
  MSWindowsHook();
  virtual ~MSWindowsHook();

  void loadLibrary();

  int init(DWORD threadID);

  int cleanup();

  void setSides(uint32_t sides);

  void setZone(int32_t x, int32_t y, int32_t w, int32_t h, int32_t jumpZoneSize);

  void setMode(EHookMode mode);

  static EHookResult install();

  static int uninstall();

  static int installScreenSaver();

  static int uninstallScreenSaver();
};
