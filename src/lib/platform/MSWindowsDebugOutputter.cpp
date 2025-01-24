/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2012 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/MSWindowsDebugOutputter.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>

MSWindowsDebugOutputter::MSWindowsDebugOutputter()
{
}

MSWindowsDebugOutputter::~MSWindowsDebugOutputter()
{
}

void MSWindowsDebugOutputter::open(const char *title)
{
}

void MSWindowsDebugOutputter::close()
{
}

void MSWindowsDebugOutputter::show(bool showIfEmpty)
{
}

bool MSWindowsDebugOutputter::write(ELevel level, const char *msg)
{
  OutputDebugString((std::string(msg) + "\n").c_str());
  return true;
}

void MSWindowsDebugOutputter::flush()
{
}
