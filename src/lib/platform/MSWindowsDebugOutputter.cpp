/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2012 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/MSWindowsDebugOutputter.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <QString>

void MSWindowsDebugOutputter::open(const QString &title)
{
  // do nothing
}

void MSWindowsDebugOutputter::close()
{
  // do nothing
}

void MSWindowsDebugOutputter::show(bool showIfEmpty)
{
  // do nothing
}

bool MSWindowsDebugOutputter::write(LogLevel level, const QString &msg)
{
  std::wstring out = msg.toStdWString() + L"\n";
  OutputDebugString(out.c_str());
  return true;
}

void MSWindowsDebugOutputter::flush()
{
  // do nothing
}
