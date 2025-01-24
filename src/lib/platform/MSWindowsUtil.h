/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2004 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <string>

#define WINDOWS_LEAN_AND_MEAN
#include <Windows.h>

class MSWindowsUtil
{
public:
  //! Get message string
  /*!
  Gets a string for \p id from the string table of \p instance.
  */
  static std::string getString(HINSTANCE instance, DWORD id);

  //! Get error string
  /*!
  Gets a system error message for \p error.  If the error cannot be
  found return the string for \p id, replacing ${1} with \p error.
  */
  static std::string getErrorString(HINSTANCE, DWORD error, DWORD id);
};
