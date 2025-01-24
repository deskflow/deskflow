/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/ArchConsoleStd.h"
#include "base/Log.h"

#include <iostream>

void ArchConsoleStd::writeConsole(ELevel level, const char *str)
{
  if ((level >= kFATAL) && (level <= kWARNING))
    std::cerr << str << std::endl;
  else
    std::cout << str << std::endl;

  std::cout.flush();
}
