/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2002 Chris Schoeneman
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/ArchConsoleStd.h"

#define ARCH_CONSOLE ArchConsoleUnix

class ArchConsoleUnix : public ArchConsoleStd
{
public:
  ArchConsoleUnix();
  virtual ~ArchConsoleUnix();
};
