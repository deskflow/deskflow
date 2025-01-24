/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/ArchConsoleStd.h"

#define ARCH_CONSOLE ArchConsoleWindows

class ArchConsoleWindows : public ArchConsoleStd
{
public:
  ArchConsoleWindows();
  virtual ~ArchConsoleWindows();
};
